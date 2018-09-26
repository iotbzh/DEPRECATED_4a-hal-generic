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
	AFB_ApiNotice(plugin->api, "%s Plugin Registered correctly: uid='%s' 'info='%s'", HAL_BT_PLUGIN_NAME, plugin->uid, plugin->info);

	return 0;
}

CTLP_INIT(plugin, callbacks)
{
	int btChannelsNumber;

	unsigned int idx;

	char *btStreamZone, *returnedInfo;

	CtlConfigT *ctrlConfig;

	struct SpecificHalData *currentHalData;

	json_object *actionsToAdd, *returnedJ, *halMixerJ, *halOrigCaptureJ, *halNewCaptureJ, *halOrigStreamJ, *halNewStreamJ, *btCaptureJ, *btCaptureParamsJ, *btStreamJ;

	AFB_ApiInfo(plugin->api, "Plugin initialization of %s plugin", HAL_BT_PLUGIN_NAME);

	if(AFB_RequireApi(plugin->api, BT_MANAGER_API, 1)) {
		AFB_ApiWarning(plugin->api, "Didn't succeed to require %s api, bluetooth is disable because not reachable", BT_MANAGER_API);
		return 0;
	}

	if(! (ctrlConfig = (CtlConfigT *) afb_dynapi_get_userdata(plugin->api))) {
		AFB_ApiError(plugin->api, "Can't get current hal controller config");
		return -1;
	}

	if(! (currentHalData = (struct SpecificHalData *) ctrlConfig->external)) {
		AFB_ApiError(plugin->api, "Can't get current hal controller data");
		return -2;
	}

	if((! currentHalData->ctlHalSpecificData) ||
	   (! (halMixerJ = currentHalData->ctlHalSpecificData->halMixerJ))) {
		AFB_ApiError(plugin->api, "Can't get current hal mixer json section");
		return -3;
	}

	if(AFB_ServiceSync(plugin->api, BT_MANAGER_API, BT_MANAGER_GET_POWER_INFO, NULL, &returnedJ)) {
		if((! wrap_json_unpack(returnedJ, "{s:{s:s}}", "request", "info", &returnedInfo)) &&
		   (! strncmp(returnedInfo, "Unable to get power status", strlen(returnedInfo)))) {
			AFB_ApiWarning(plugin->api,
				       "No bluetooth receiver detected when calling verb '%s' of '%s' api, bluetooth is disable because not reachable",
				       BT_MANAGER_GET_POWER_INFO,
				       BT_MANAGER_API);
			return 0;
		}
		else {
			AFB_ApiError(plugin->api,
				     "Error during call to verb '%s' of '%s' api (%s)",
				     BT_MANAGER_GET_POWER_INFO,
				     BT_MANAGER_API,
				     json_object_get_string(returnedJ));
			return -4;
		}
	}

	if((! json_object_is_type(plugin->paramsJ, json_type_object)) ||
	   (wrap_json_unpack(plugin->paramsJ, "{s:i, s:s}", "channels", &btChannelsNumber, "zone", &btStreamZone))) {
		AFB_ApiError(plugin->api, "Can't get %s plugin parameters from json ('%s')", HAL_BT_PLUGIN_NAME, json_object_get_string(plugin->paramsJ));
		return -5;
	}

	wrap_json_pack(&actionsToAdd, "{s:s s:s}",
				      "uid", "Bluetooth-Manager/device_updated",
				      "action", "plugin://hal-bt#events");

	if(currentHalData->status != HAL_STATUS_AVAILABLE) {
		AFB_ApiWarning(plugin->api, "Controller initialization of %s plugin cannot be done because hal is not ready to be used", HAL_BT_PLUGIN_NAME);
		return 0;
	}

	memset(&localHalBtPluginData, '\0', sizeof(localHalBtPluginData));

	localHalBtPluginData.currentHalApiHandle = plugin->api;

	localHalBtPluginData.currentHalData = currentHalData;

	idx = 0;
	while(ctrlConfig->sections[idx].key && strcasecmp(ctrlConfig->sections[idx].key, "events"))
		idx++;

	if(! ctrlConfig->sections[idx].key) {
		AFB_ApiError(plugin->api, "Wasn't able to add '%s' as a new event, 'events' section not found", json_object_get_string(actionsToAdd));
		json_object_put(actionsToAdd);
		return -6;
	}

	if(AddActionsToSectionFromPlugin(plugin->api, *ctrlConfig->ctlPlugins, &ctrlConfig->sections[idx], actionsToAdd, 0)) {
		AFB_ApiError(plugin->api, "Wasn't able to add '%s' as a new event to %s", json_object_get_string(actionsToAdd), ctrlConfig->sections[idx].key);
		json_object_put(actionsToAdd);
		return -7;
	}

	wrap_json_pack(&actionsToAdd, "{s:s s:s s:s}",
				      "uid", "init-bt-plugin",
				      "info", "Init Bluetooth hal plugin",
				      "action", "plugin://hal-bt#init");

	idx = 0;
	while(ctrlConfig->sections[idx].key && strcasecmp(ctrlConfig->sections[idx].key, "onload"))
		idx++;

	if(! ctrlConfig->sections[idx].key) {
		AFB_ApiError(plugin->api, "Wasn't able to add '%s' as a new onload, 'onload' section not found", json_object_get_string(actionsToAdd));
		json_object_put(actionsToAdd);
		return -8;
	}

	if(AddActionsToSectionFromPlugin(plugin->api, *ctrlConfig->ctlPlugins, &ctrlConfig->sections[idx], actionsToAdd, 0)) {
		AFB_ApiError(plugin->api, "Wasn't able to add '%s' as a new onload to %s", json_object_get_string(actionsToAdd), ctrlConfig->sections[idx].uid);
		json_object_put(actionsToAdd);
		return -9;
	}

	btCaptureJ = json_tokener_parse(MIXER_BT_CAPTURE_JSON_SECTION);
	wrap_json_pack(&btCaptureParamsJ, "{s:i}", "channels", btChannelsNumber);
	json_object_object_add(btCaptureJ, "params", btCaptureParamsJ);

	if(! json_object_object_get_ex(halMixerJ, "captures", &halOrigCaptureJ)) {
		halNewCaptureJ = json_object_new_array();
		json_object_array_add(halNewCaptureJ, btCaptureJ);
		json_object_object_add(halMixerJ, "captures", halNewCaptureJ);
	}
	else if(json_object_is_type(halOrigCaptureJ, json_type_array)) {
		halNewCaptureJ = halOrigCaptureJ;
		json_object_array_add(halNewCaptureJ, btCaptureJ);
	}
	else if(json_object_is_type(halOrigCaptureJ, json_type_object)) {
		json_object_get(halOrigCaptureJ);
		json_object_object_del(halMixerJ, "captures");
		halNewCaptureJ = json_object_new_array();
		json_object_array_add(halNewCaptureJ, halOrigCaptureJ);
		json_object_array_add(halNewCaptureJ, btCaptureJ);
		json_object_object_add(halMixerJ, "captures", halNewCaptureJ);
	}
	else {
		AFB_ApiError(plugin->api, "Unrecognized 'halmixer' captures format");
		return -10;
	}

	btStreamJ = json_tokener_parse(MIXER_BT_STREAM_JSON_SECTION);
	json_object_object_add(btStreamJ, "zone", json_object_new_string(btStreamZone));

	if(! json_object_object_get_ex(halMixerJ, "streams", &halOrigStreamJ)) {
		halNewStreamJ = json_object_new_array();
		json_object_array_add(halNewStreamJ, btStreamJ);
		json_object_object_add(halMixerJ, "streams", halNewStreamJ);
	}
	else if(json_object_is_type(halOrigStreamJ, json_type_array)) {
		halNewStreamJ = halOrigStreamJ;
		json_object_array_add(halNewStreamJ, btStreamJ);
	}
	else if(json_object_is_type(halOrigStreamJ, json_type_object)) {
		json_object_get(halOrigStreamJ);
		json_object_object_del(halMixerJ, "streams");
		halNewStreamJ = json_object_new_array();
		json_object_array_add(halNewStreamJ, halOrigStreamJ);
		json_object_array_add(halNewStreamJ, btStreamJ);
		json_object_object_add(halMixerJ, "streams", halNewStreamJ);
	}
	else {
		AFB_ApiError(plugin->api, "Unrecognized 'halmixer' streams format");
		return -11;
	}

	localHalBtPluginData.halBtPluginEnabled = 1;

	AFB_ApiNotice(plugin->api, "Plugin initialization of %s plugin correctly done", HAL_BT_PLUGIN_NAME);

	return 0;
}

// Call at contoller onload time
CTLP_CAPI(init, source, argsJ, queryJ)
{
	int err;

	char *returnedInfo;

	json_object *toSendJ, *returnedJ, *returnedBtList = NULL;

	if(! localHalBtPluginData.halBtPluginEnabled) {
		AFB_ApiWarning(source->api, "Controller onload initialization of %s plugin cannot be done because bluetooth is not reachable", HAL_BT_PLUGIN_NAME);
		return 0;
	}

	AFB_ApiInfo(source->api, "Controller onload initialization of %s plugin", HAL_BT_PLUGIN_NAME);

	// Loading hal BT plugin specific verbs
	if(afb_dynapi_add_verb(source->api,
			       HAL_BT_GET_STREAMING_STATUS_VERB,
			       "Get Bluetooth streaming status",
			       HalBtGetStreamingStatus,
			       (void *) &localHalBtPluginData,
			       NULL,
			       0)) {
		AFB_ApiError(source->api, "Error while creating verb for bluetooth plugin : '%s'", HAL_BT_GET_STREAMING_STATUS_VERB);
		return -1;
	}

	if(afb_dynapi_add_verb(source->api,
			       HAL_BT_SET_STREAMING_STATUS_VERB,
			       "Set Bluetooth streaming status",
			       HalBtSetStreamingStatus,
			       (void *) &localHalBtPluginData,
			       NULL,
			       0)) {
		AFB_ApiError(source->api, "Error while creating verb for bluetooth plugin : '%s'", HAL_BT_SET_STREAMING_STATUS_VERB);
		return -2;
	}

	if(afb_dynapi_add_verb(source->api, 
			       HAL_BT_GET_CONNECTED_A2DP_DEVICES_VERB,
			       "Get connected Bluetooth A2DP devices list",
			       HalBtGetA2DPBluetoothDevices,
			       (void *) &localHalBtPluginData,
			       NULL,
			       0)) {
		AFB_ApiError(source->api, "Error while creating verb for bluetooth plugin : '%s'", HAL_BT_GET_CONNECTED_A2DP_DEVICES_VERB);
		return -3;
	}

	if(afb_dynapi_add_verb(source->api,
			       HAL_BT_GET_SELECTED_A2DP_DEVICE_VERB,
			       "Get selected Bluetooth A2DP device",
			       HalBtGetSelectedA2DPBluetoothDevice,
			       (void *) &localHalBtPluginData,
			       NULL,
			       0)) {
		AFB_ApiError(source->api, "Error while creating verb for bluetooth plugin : '%s'", HAL_BT_GET_SELECTED_A2DP_DEVICE_VERB);
		return -4;
	}

	if(afb_dynapi_add_verb(source->api,
			       HAL_BT_SET_SELECTED_A2DP_DEVICE_VERB,
			       "Set selected Bluetooth A2DP device",
			       HalBtSetSelectedA2DPBluetoothDevice,
			       (void *) &localHalBtPluginData,
			       NULL,
			       0)) {
		AFB_ApiError(source->api, "Error while creating verb for bluetooth plugin : '%s'", HAL_BT_SET_SELECTED_A2DP_DEVICE_VERB);
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
		json_object_put(returnedJ);
		return -6;
	}
	json_object_put(returnedJ);

	if(AFB_ServiceSync(source->api, BT_MANAGER_API, BT_MANAGER_GET_DEVICES_VERB, NULL, &returnedJ)) {
		if((! wrap_json_unpack(returnedJ, "{s:{s:s}}", "request", "info", &returnedInfo)) &&
		   (! strncmp(returnedInfo, "No find devices", strlen(returnedInfo)))) {
			AFB_ApiInfo(source->api,
				    "No bluetooth devices returned by call to verb '%s' of '%s' api",
				    BT_MANAGER_GET_DEVICES_VERB,
				    BT_MANAGER_API);
		}
		else {
			AFB_ApiError(source->api,
				     "Error during call to verb '%s' of '%s' api (%s)",
				     BT_MANAGER_GET_DEVICES_VERB,
				     BT_MANAGER_API,
				     json_object_get_string(returnedJ));

			return -7;
		}
	}
	else if(wrap_json_unpack(returnedJ, "{s:{s:o}}", "response", "list", &returnedBtList)) {
		AFB_ApiError(source->api,
			     "Couldn't get bluetooth device list during call to verb '%s' of api '%s'",
			     BT_MANAGER_GET_DEVICES_VERB,
			     BT_MANAGER_API);
		json_object_put(returnedJ);
		return -7;
	}

	if((returnedBtList) && (err = HalBtDataHandleReceivedMutlipleBtDeviceData(&localHalBtPluginData, returnedBtList)))
		return (10 * err);

	json_object_put(returnedJ);

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

	AFB_ApiNotice(source->api, "Controller onload initialization of %s plugin correctly done", HAL_BT_PLUGIN_NAME);

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