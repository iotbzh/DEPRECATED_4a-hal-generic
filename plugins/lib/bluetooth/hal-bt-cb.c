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

#include "hal-bt-data.h"

/*******************************************************************************
 *		HAL Bluetooth plugin verbs functions			       *
 ******************************************************************************/

void HalBtGetStreamingStatus(AFB_ReqT request)
{
	struct HalBtPluginData *localHalBtPluginData;

	if(! (localHalBtPluginData = (struct HalBtPluginData *) afb_request_get_vcbdata(request))) {
		AFB_ReqFail(request, "bt_plugin_data", "Can't get bluetooth plugin data");
		return;
	}

	AFB_ReqSuccess(request, json_object_new_string(localHalBtPluginData->btStreamEnabled ? "enabled" : "disabled"), "Bluetooth streaming status");
}

void HalBtSetStreamingStatus(AFB_ReqT request)
{
	unsigned int requestedBtStreamingStatus;

	struct HalBtPluginData *localHalBtPluginData;

	json_object *requestJson;

	if(! (localHalBtPluginData = (struct HalBtPluginData *) afb_request_get_vcbdata(request))) {
		AFB_ReqFail(request, "bt_plugin_data", "Can't get bluetooth plugin data");
		return;
	}

	if(! (requestJson = AFB_ReqJson(request))) {
		AFB_ReqFail(request, "request_json", "Can't get request json");
		return;
	}

	if(wrap_json_unpack(requestJson, "{s:b}", "status", &requestedBtStreamingStatus)) {
		AFB_ReqFail(request, "requested_status", "Can't get requested bluetooth streaming status");
		return;
	}

	localHalBtPluginData->btStreamEnabled = requestedBtStreamingStatus;
	
	// TODO JAI : Enable capture stream to playback using 'uid' used at 'set-capture' call

	AFB_ReqSuccess(request, NULL, "Bluetooth streaming status successfully set");
}

void HalBtGetConnectedBluetoothDevices(AFB_ReqT request)
{
	struct HalBtPluginData *localHalBtPluginData;
	struct HalBtDeviceData *currentBtDeviceData;

	json_object *requestAnswer, *currentBtDeviceObjectJ;

	if(! (localHalBtPluginData = (struct HalBtPluginData *) afb_request_get_vcbdata(request))) {
		AFB_ReqFail(request, "bt_plugin_data", "Can't get bluetooth plugin data");
		return;
	}

	if(! (currentBtDeviceData = localHalBtPluginData->first)) {
		AFB_ReqSuccess(request, NULL, "No bluetooth device connected");
		return;
	}

	requestAnswer = json_object_new_array();
	if(! requestAnswer) {
		AFB_ReqFail(request, "json_answer", "Can't generate json answer");
		return;
	}

	while(currentBtDeviceData) {
		wrap_json_pack(&currentBtDeviceObjectJ,
			       "{s:s s:s}",
			       "Name", currentBtDeviceData->name,
			       "Address", currentBtDeviceData->address);
		json_object_array_add(requestAnswer, currentBtDeviceObjectJ);

		currentBtDeviceData = currentBtDeviceData->next;
	}

	AFB_ReqSuccess(request, requestAnswer, "Connected bluetooth devices list");
}

void HalBtGetSelectedBluetoothDevice(AFB_ReqT request)
{
	struct HalBtPluginData *localHalBtPluginData;

	json_object *selectedBtDeviceObject;

	if(! (localHalBtPluginData = (struct HalBtPluginData *) afb_request_get_vcbdata(request))) {
		AFB_ReqFail(request, "bt_plugin_data", "Can't get bluetooth plugin data");
		return;
	}

	if(! localHalBtPluginData->selectedBtDevice) {
		AFB_ReqSuccess(request, NULL, "No bluetooth device selected");
		return;
	}

	wrap_json_pack(&selectedBtDeviceObject,
		       "{s:s s:s}",
		       "Name", localHalBtPluginData->selectedBtDevice->name,
		       "Address", localHalBtPluginData->selectedBtDevice->address);
	
	AFB_ReqSuccess(request, selectedBtDeviceObject, "Selected Bluetooth device");
}

void HalBtSetSelectedBluetoothDevice(AFB_ReqT request)
{
	char *requestedBtDeviceToSelect;

	struct HalBtPluginData *localHalBtPluginData;
	struct HalBtDeviceData *selectedBtDeviceData;

	json_object *requestJson;

	if(! (localHalBtPluginData = (struct HalBtPluginData *) afb_request_get_vcbdata(request))) {
		AFB_ReqFail(request, "bt_plugin_data", "Can't get bluetooth plugin data");
		return;
	}

	if(! (requestJson = AFB_ReqJson(request))) {
		AFB_ReqFail(request, "request_json", "Can't get request json");
		return;
	}

	if(wrap_json_unpack(requestJson, "{s:s}", "Address", &requestedBtDeviceToSelect)) {
		AFB_ReqFail(request, "requested_device_to_select", "Can't get requested bluetooth device to select");
		return;
	}

	if(! (selectedBtDeviceData = HalBtDataSearchBtDeviceByAddress(&localHalBtPluginData->first, requestedBtDeviceToSelect))) {
		AFB_ReqFail(request, "requested_device_to_select", "Requested bluetooth device to select is not currently connected");
		return;
	}

	localHalBtPluginData->selectedBtDevice = selectedBtDeviceData;

	// TODO JAI : Tell the softmixer that we want it as an input using 'bluealsa:HCI=hci0,DEV=F6:32:15:2A:80:70,PROFILE=a2dp' 
	//	      string as capture associated with an 'uid' using smixer verb 'set-capture'

	AFB_ReqSuccess(request, NULL, "Selected bluetooth device successfully set");
}