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

#include <ctl-plugin.h>

#include "hal-bt.h"
#include "hal-bt-cb.h"
#include "hal-bt-data.h"

// Local (static) Hal manager data structure
static struct HalBtPluginData localHalBtPluginData;

CTLP_CAPI_REGISTER(HAL_BT_PLUGIN_NAME)

// Call at initialisation time
CTLP_ONLOAD(plugin, callbacks)
{
	AFB_ApiNotice(plugin->api, "Hal-Bt Plugin Registered: uid='%s' 'info='%s'", plugin->uid, plugin->info);

	memset(&localHalBtPluginData, '\0', sizeof(localHalBtPluginData));

	AFB_RequireApi(plugin->api, BT_MANAGER_API, 1);

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

	struct json_object *toSendJ, *returnedJ, *returnedBtList = NULL;

	AFB_ApiNotice(source->api, "Initializing HAL-BT plugin");

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
			       HAL_BT_GET_CONNECTED_DEVICES_VERB,
			       "Get connected Bluetooth devices list",
			       HalBtGetConnectedBluetoothDevices,
			       (void *) &localHalBtPluginData,
			       NULL,
			       0)) {
		AFB_ApiError(source->api, "%s: error while creating verb for bluetooth plugin : '%s'", __func__, HAL_BT_GET_CONNECTED_DEVICES_VERB);
		return -3;
	}

	if(afb_dynapi_add_verb(source->api,
			       HAL_BT_GET_SELECTED_DEVICE_VERB,
			       "Get selected Bluetooth device",
			       HalBtGetSelectedBluetoothDevice,
			       (void *) &localHalBtPluginData,
			       NULL,
			       0)) {
		AFB_ApiError(source->api, "%s: error while creating verb for bluetooth plugin : '%s'", __func__, HAL_BT_GET_SELECTED_DEVICE_VERB);
		return -4;
	}

	if(afb_dynapi_add_verb(source->api,
			       HAL_BT_SET_SELECTED_DEVICE_VERB,
			       "Set selected Bluetooth device",
			       HalBtSetSelectedBluetoothDevice,
			       (void *) &localHalBtPluginData,
			       NULL,
			       0)) {
		AFB_ApiError(source->api, "%s: error while creating verb for bluetooth plugin : '%s'", __func__, HAL_BT_SET_SELECTED_DEVICE_VERB);
		return -5;
	}

	// Register to Bluetooth manager
	wrap_json_pack(&toSendJ, "{s:s}", "value", BT_MANAGER_DEVICE_UPDATE_EVENT);
	if(AFB_ServiceSync(source->api, BT_MANAGER_API, BT_MANAGER_SUBSCRIBE_VERB, toSendJ, &returnedJ)) {
		AFB_ApiError(source->api,
			     "Error during call to verb '%s' of '%s' api",
			     BT_MANAGER_SUBSCRIBE_VERB,
			     BT_MANAGER_API);

		return -6;
	}
	else if(! wrap_json_unpack(returnedJ, "{s:{s:s}}", "request", "info", NULL)) {
		AFB_ApiError(source->api,
			     "Couldn't subscribe to event '%s' during call to verb '%s' of api '%s'",
			     BT_MANAGER_DEVICE_UPDATE_EVENT,
			     BT_MANAGER_SUBSCRIBE_VERB,
			     BT_MANAGER_API);
		return -6;
	}

	if(AFB_ServiceSync(source->api, BT_MANAGER_API, BT_MANAGER_GET_DEVICES_VERB, NULL, &returnedJ)) {
		AFB_ApiError(source->api,
			     "Error during call to verb '%s' of '%s' api",
			     BT_MANAGER_GET_DEVICES_VERB,
			     BT_MANAGER_API);

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
		/* TODO JAI : send selected device to softmixer (and enable stream here for now)
					* Tell the softmixer that we want it as an input using 'bluealsa:HCI=hci0,DEV=F6:32:15:2A:80:70,PROFILE=a2dp'
					  string as capture associated with an 'uid' using smixer verb 'set-capture'
					* Enable capture stream to playback using 'uid' used at 'set-capture' call
		*/
	}

	return 0;
}

// This receive Hal bluetooth plugin events
CTLP_CAPI(events, source, argsJ, queryJ)
{
	struct HalBtDeviceData *previouslySelectedBtDevice = localHalBtPluginData.selectedBtDevice;

	if(HalBtDataHandleReceivedSingleBtDeviceData(&localHalBtPluginData, queryJ)) {
		AFB_ApiError(source->api, "Error while decoding bluetooth event received json (%s)", json_object_get_string(queryJ));
		return -1;
	}

	if(localHalBtPluginData.selectedBtDevice && localHalBtPluginData.selectedBtDevice != previouslySelectedBtDevice) {
		localHalBtPluginData.btStreamEnabled = 1;
		/* TODO JAI : send selected device to softmixer (and enable stream here for now)
					* Tell the softmixer that we want it as an input using 'bluealsa:HCI=hci0,DEV=F6:32:15:2A:80:70,PROFILE=a2dp'
					  string as capture associated with an 'uid' using smixer verb 'set-capture'
					* Enable capture stream to playback using 'uid' used at 'set-capture' call
		*/
	}
	else if (! localHalBtPluginData.selectedBtDevice) {
		localHalBtPluginData.btStreamEnabled = 0;
		// TODO JAI: Disable capture stream to playback using 'uid' used at 'set-capture' call
	}

	return 0;
}