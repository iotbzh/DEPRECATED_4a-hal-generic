/*
 * Copyright (C) 2018 "IoT.bzh"
 * Author Jonathan Aillet <jonathan.aillet@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <wrap-json.h>

#include <ctl-config.h>

#include "hal-bt.h"
#include "hal-bt-cb.h"
#include "hal-bt-data.h"
#include "hal-bt-mixer-link.h"

// Local (static) Hal manager data structure
static struct HalBtPluginData localHalBtPluginData;

CTLP_CAPI_REGISTER(HAL_BT_PLUGIN_NAME)

// Call at initialisation time
CTLP_ONLOAD(plugin, callbacks)
{
	CtlConfigT *ctrlConfig;

	AFB_ApiInfo(plugin->api, "Hal-Bt Plugin Registering: uid='%s' 'info='%s'", plugin->uid, plugin->info);

	memset(&localHalBtPluginData, '\0', sizeof(localHalBtPluginData));

	localHalBtPluginData.currentHalApiHandle = plugin->api;

	if(AFB_RequireApi(plugin->api, BT_MANAGER_API, 1)) {
		AFB_ApiWarning(plugin->api, "Didn't succeed to require %s api, bluetooth is disable because not reachable", BT_MANAGER_API);
		return 0;
	}

	if(! (ctrlConfig = (CtlConfigT *) afb_dynapi_get_userdata(plugin->api))) {
		AFB_ApiError(plugin->api, "Can't get current hal controller config");
		return -1;
	}

	if(! (localHalBtPluginData.currentHalData = (struct SpecificHalData *) ctrlConfig->external)) {
		AFB_ApiError(plugin->api, "%s: Can't get current hal controller data", __func__);
		return -2;
	}

	localHalBtPluginData.halBtPluginEnabled = 1;

	AFB_ApiNotice(plugin->api, "Hal-Bt Plugin Registered correctly: uid='%s' 'info='%s'", plugin->uid, plugin->info);

	/* TDB JAI :
		- Register 'init' plugin function (HAL_BT_PLUGIN_NAME#init) as onload action here (to avoid adding it in json)
		- Register 'event' plugin function (HAL_BT_PLUGIN_NAME#event) as action for BT_MANAGER_API#BT_MANAGER_DEVICE_UPDATE_EVENT event here (to avoid adding it in json)
	*/

	return 0;
}

// Call at contoller onload time
CTLP_CAPI(init, source, argsJ, queryJ)
{
	unsigned int err;

	char *returnedInfo;

	struct json_object *toSendJ, *returnedJ, *returnedBtList = NULL;

	if(! localHalBtPluginData.halBtPluginEnabled) {
		AFB_ApiWarning(source->api, "Controller onload initialization of HAL-BT plugin cannot be done because bluetooth is not reachable");
		return 0;
	}

	AFB_ApiInfo(source->api, "Controller onload initialization of HAL-BT plugin");

	// Loading hal BT plugin specific verbs
	if(afb_dynapi_add_verb(source->api,
			       HAL_BT_GET_STREAMING_STATUS_VERB,
			       "Get Bluetooth streaming status",
			       HalBtGetStreamingStatus,
			       (void *) &localHalBtPluginData,
			       NULL,
			       0)) {
		AFB_ApiError(source->api, "%s: error while creating verb for bluetooth plugin : '%s'", __func__, HAL_BT_GET_STREAMING_STATUS_VERB);
		return -1;
	}

	if(afb_dynapi_add_verb(source->api,
			       HAL_BT_SET_STREAMING_STATUS_VERB,
			       "Set Bluetooth streaming status",
			       HalBtSetStreamingStatus,
			       (void *) &localHalBtPluginData,
			       NULL,
			       0)) {
		AFB_ApiError(source->api, "%s: error while creating verb for bluetooth plugin : '%s'", __func__, HAL_BT_SET_STREAMING_STATUS_VERB);
		return -2;
	}

	if(afb_dynapi_add_verb(source->api, 
			       HAL_BT_GET_CONNECTED_A2DP_DEVICES_VERB,
			       "Get connected Bluetooth A2DP devices list",
			       HalBtGetA2DPBluetoothDevices,
			       (void *) &localHalBtPluginData,
			       NULL,
			       0)) {
		AFB_ApiError(source->api, "%s: error while creating verb for bluetooth plugin : '%s'", __func__, HAL_BT_GET_CONNECTED_A2DP_DEVICES_VERB);
		return -3;
	}

	if(afb_dynapi_add_verb(source->api,
			       HAL_BT_GET_SELECTED_A2DP_DEVICE_VERB,
			       "Get selected Bluetooth A2DP device",
			       HalBtGetSelectedA2DPBluetoothDevice,
			       (void *) &localHalBtPluginData,
			       NULL,
			       0)) {
		AFB_ApiError(source->api, "%s: error while creating verb for bluetooth plugin : '%s'", __func__, HAL_BT_GET_SELECTED_A2DP_DEVICE_VERB);
		return -4;
	}

	if(afb_dynapi_add_verb(source->api,
			       HAL_BT_SET_SELECTED_A2DP_DEVICE_VERB,
			       "Set selected Bluetooth A2DP device",
			       HalBtSetSelectedA2DPBluetoothDevice,
			       (void *) &localHalBtPluginData,
			       NULL,
			       0)) {
		AFB_ApiError(source->api, "%s: error while creating verb for bluetooth plugin : '%s'", __func__, HAL_BT_SET_SELECTED_A2DP_DEVICE_VERB);
		return -5;
	}

	// Register to Bluetooth manager
	wrap_json_pack(&toSendJ, "{s:s}", "value", BT_MANAGER_DEVICE_UPDATE_EVENT);
	if(AFB_ServiceSync(source->api, BT_MANAGER_API, BT_MANAGER_SUBSCRIBE_VERB, toSendJ, &returnedJ)) {
		AFB_ApiError(source->api,
			     "Error during call to verb '%s' of '%s' api (%s)",
			     BT_MANAGER_SUBSCRIBE_VERB,
			     BT_MANAGER_API,
			     json_object_get_string(returnedJ));

		return -6;
	}
	else if(! wrap_json_unpack(returnedJ, "{s:{s:s}}", "request", "info", &returnedInfo)) {
		AFB_ApiError(source->api,
			     "Couldn't subscribe to event '%s' during call to verb '%s' of api '%s' (error '%s')",
			     BT_MANAGER_DEVICE_UPDATE_EVENT,
			     BT_MANAGER_SUBSCRIBE_VERB,
			     BT_MANAGER_API,
			     returnedInfo);
		return -6;
	}

	if(AFB_ServiceSync(source->api, BT_MANAGER_API, BT_MANAGER_GET_DEVICES_VERB, NULL, &returnedJ)) {
		AFB_ApiError(source->api,
			     "Error during call to verb '%s' of '%s' api (%s)",
			     BT_MANAGER_GET_DEVICES_VERB,
			     BT_MANAGER_API,
			     json_object_get_string(returnedJ));

		return -7;
	}
	else if(wrap_json_unpack(returnedJ, "{s:{s:o}}", "response", "list", &returnedBtList)) {
		AFB_ApiError(source->api,
			     "Couldn't get bluetooth device list during call to verb '%s' of api '%s'",
			     BT_MANAGER_GET_DEVICES_VERB,
			     BT_MANAGER_API);
		return -7;
	}

	if((err = HalBtDataHandleReceivedMutlipleBtDeviceData(&localHalBtPluginData, returnedBtList)))
		return (10 * err);

	if(localHalBtPluginData.selectedBtDevice) {
		localHalBtPluginData.btStreamEnabled = 1;

		AFB_ApiInfo(source->api, "Useable bluetooth device detected at initialization, will try to use it");

		if(HalBtMixerLinkSetBtStreamingSettings(source->api,
							localHalBtPluginData.currentHalData->ctlHalSpecificData->mixerApiName,
							localHalBtPluginData.btStreamEnabled,
							localHalBtPluginData.selectedBtDevice->hci,
							localHalBtPluginData.selectedBtDevice->address)) {
			AFB_ApiError(source->api,
				     "Couldn't set bluetooth streaming settings during call to verb '%s' of api '%s'",
				     MIXER_SET_STREAMED_BT_DEVICE_VERB,
				     localHalBtPluginData.currentHalData->ctlHalSpecificData->mixerApiName);
			return -8;
		}
	}

	AFB_ApiNotice(source->api, "Controller onload initialization of HAL-BT plugin correctly done");

	return 0;
}

// This receive Hal bluetooth plugin events
CTLP_CAPI(events, source, argsJ, queryJ)
{
	struct HalBtDeviceData *previouslySelectedBtDevice = localHalBtPluginData.selectedBtDevice;

	if(! localHalBtPluginData.halBtPluginEnabled) {
		AFB_ApiWarning(source->api, "Bluetooth event received but cannot be handled because bluetooth is not reachable");
		return 0;
	}

	if(HalBtDataHandleReceivedSingleBtDeviceData(&localHalBtPluginData, queryJ)) {
		AFB_ApiError(source->api, "Error while decoding bluetooth event received json (%s)", json_object_get_string(queryJ));
		return -1;
	}

	if(localHalBtPluginData.selectedBtDevice == previouslySelectedBtDevice) {
		AFB_ApiDebug(source->api, "Bluetooth event received but device didn't change");
		return 0;
	}

	if(localHalBtPluginData.selectedBtDevice)
		localHalBtPluginData.btStreamEnabled = 1;
	else
		localHalBtPluginData.btStreamEnabled = 0;

	if(HalBtMixerLinkSetBtStreamingSettings(source->api,
						localHalBtPluginData.currentHalData->ctlHalSpecificData->mixerApiName,
						localHalBtPluginData.btStreamEnabled,
						localHalBtPluginData.selectedBtDevice ? localHalBtPluginData.selectedBtDevice->hci : NULL,
						localHalBtPluginData.selectedBtDevice ? localHalBtPluginData.selectedBtDevice->address : NULL)) {
		AFB_ApiError(source->api,
			     "Couldn't set bluetooth streaming settings during call to verb '%s' of api '%s'",
			     MIXER_SET_STREAMED_BT_DEVICE_VERB,
			     localHalBtPluginData.currentHalData->ctlHalSpecificData->mixerApiName);
		return -2;
	}

	return 0;
}