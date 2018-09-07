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

#include "hal-bt-data.h"

/*******************************************************************************
 *		Bt device data handling functions			       *
 ******************************************************************************/

int HalBtDataRemoveSelectedBtDeviceFromList(struct HalBtDeviceData **firstBtDeviceData, struct HalBtDeviceData *btDeviceDataToRemove)
{
	struct HalBtDeviceData *currentBtDevice, *matchingBtDevice;

	if(! firstBtDeviceData || ! btDeviceDataToRemove)
		return -1;

	currentBtDevice = *firstBtDeviceData;

	if(currentBtDevice == btDeviceDataToRemove) {
		*firstBtDeviceData = currentBtDevice->next;
		matchingBtDevice = currentBtDevice;
	}
	else {
		while(currentBtDevice && currentBtDevice->next != btDeviceDataToRemove)
			currentBtDevice = currentBtDevice->next;

		if(currentBtDevice) {
			matchingBtDevice = currentBtDevice->next;
			currentBtDevice->next = currentBtDevice->next->next;
		}
		else {
			return -2;
		}
	}

	free(matchingBtDevice->hci);
	free(matchingBtDevice->uid);
	free(matchingBtDevice->name);
	free(matchingBtDevice->address);

	free(matchingBtDevice);

	return 0;
}

struct HalBtDeviceData *HalBtDataAddBtDeviceToBtDeviceList(struct HalBtDeviceData **firstBtDeviceData, json_object *currentSingleBtDeviceDataJ)
{
	char *currentBtDeviceName, *currentBtDeviceAddress;

	struct HalBtDeviceData *currentBtDeviceData;

	if(! firstBtDeviceData || ! currentSingleBtDeviceDataJ)
		return NULL;

	currentBtDeviceData = *firstBtDeviceData;

	if(! currentBtDeviceData) {
		currentBtDeviceData = (struct HalBtDeviceData *) calloc(1, sizeof(struct HalBtDeviceData));
		if(! currentBtDeviceData)
			return NULL;

		*firstBtDeviceData = currentBtDeviceData;
	}
	else {
		while(currentBtDeviceData->next)
			currentBtDeviceData = currentBtDeviceData->next;

		currentBtDeviceData->next = calloc(1, sizeof(struct HalBtDeviceData));
		if(! currentBtDeviceData)
			return NULL;

		currentBtDeviceData = currentBtDeviceData->next;
	}

	// TODO JAI : get hci device of bt device here
	// TODO JAI : check if a2dp profile is supported here
	if(wrap_json_unpack(currentSingleBtDeviceDataJ,
			 "{s:s s:s}",
			 "Name", &currentBtDeviceName,
			 "Address", &currentBtDeviceAddress)) {
		HalBtDataRemoveSelectedBtDeviceFromList(firstBtDeviceData, currentBtDeviceData);
		return NULL;
	}

	if(asprintf(&currentBtDeviceData->uid, "BT#%s", currentBtDeviceAddress) == -1) {
		HalBtDataRemoveSelectedBtDeviceFromList(firstBtDeviceData, currentBtDeviceData);
		return NULL;
	}

	// JAI : WARNING : hci bt device is currently harcoded
	if(! (currentBtDeviceData->hci = strdup("hci0"))) {
		HalBtDataRemoveSelectedBtDeviceFromList(firstBtDeviceData, currentBtDeviceData);
		return NULL;
	}

	if(! (currentBtDeviceData->name = strdup(currentBtDeviceName))) {
		HalBtDataRemoveSelectedBtDeviceFromList(firstBtDeviceData, currentBtDeviceData);
		return NULL;
	}

	if(! (currentBtDeviceData->address = strdup(currentBtDeviceAddress))) {
		HalBtDataRemoveSelectedBtDeviceFromList(firstBtDeviceData, currentBtDeviceData);
		return NULL;
	}

	// JAI : WARNING : a2dp profile is currently harcoded
	currentBtDeviceData->a2dp = 1;

	return currentBtDeviceData;
}

struct HalBtDeviceData *HalBtDataGetFirstA2dpBtDeviceAvailable(struct HalBtDeviceData **firstBtDeviceData)
{
	struct HalBtDeviceData *currentBtDeviceData;

	if(! firstBtDeviceData)
		return NULL;

	currentBtDeviceData = *firstBtDeviceData;

	while(currentBtDeviceData) {
		if(currentBtDeviceData->a2dp)
			return currentBtDeviceData;

		currentBtDeviceData = currentBtDeviceData->next;
	}

	return NULL;
}

int HalBtDataGetNumberOfBtDeviceInList(struct HalBtDeviceData **firstBtDeviceData)
{
	unsigned int btDeviceNb = 0;

	struct HalBtDeviceData *currentBtDeviceData;

	if(! firstBtDeviceData)
		return -1;

	currentBtDeviceData = *firstBtDeviceData;

	while(currentBtDeviceData) {
		currentBtDeviceData = currentBtDeviceData->next;
		btDeviceNb++;
	}

	return btDeviceNb;
}

struct HalBtDeviceData *HalBtDataSearchBtDeviceByAddress(struct HalBtDeviceData **firstBtDeviceData, char *btAddress)
{
	struct HalBtDeviceData *currentBtDevice;

	if(! firstBtDeviceData || ! btAddress)
		return NULL;

	currentBtDevice = *firstBtDeviceData;

	while(currentBtDevice) {
		if(! strcmp(btAddress, currentBtDevice->address))
			return currentBtDevice;

		currentBtDevice = currentBtDevice->next;
	}

	return NULL;
}

int HalBtDataHandleReceivedSingleBtDeviceData(struct HalBtPluginData *halBtPluginData, json_object *currentSingleBtDeviceDataJ)
{
	unsigned int currentBtDeviceIsConnected;

	char *currentBtDeviceAddress, *currentBtDeviceIsConnectedString;

	struct HalBtDeviceData *currentBtDevice;
	if(! halBtPluginData || ! currentSingleBtDeviceDataJ)
		return -1;

	if(wrap_json_unpack(currentSingleBtDeviceDataJ,
			    "{s:s s:s}",
			    "Address", &currentBtDeviceAddress,
			    "Connected", &currentBtDeviceIsConnectedString)) {
		return -2;
	}

	currentBtDeviceIsConnected = ! strncmp(currentBtDeviceIsConnectedString, "True", strlen(currentBtDeviceIsConnectedString));
	currentBtDevice = HalBtDataSearchBtDeviceByAddress(&halBtPluginData->first, currentBtDeviceAddress);

	if(currentBtDevice && ! currentBtDeviceIsConnected) {
		if(HalBtDataRemoveSelectedBtDeviceFromList(&halBtPluginData->first, currentBtDevice))
			return -3;

		if(halBtPluginData->selectedBtDevice == currentBtDevice)
			halBtPluginData->selectedBtDevice = HalBtDataGetFirstA2dpBtDeviceAvailable(&halBtPluginData->first);
	}
	else if(! currentBtDevice && currentBtDeviceIsConnected) {
		if(! HalBtDataAddBtDeviceToBtDeviceList(&halBtPluginData->first, currentSingleBtDeviceDataJ))
			return -4;

		if(! halBtPluginData->selectedBtDevice)
			halBtPluginData->selectedBtDevice = HalBtDataGetFirstA2dpBtDeviceAvailable(&halBtPluginData->first);
	}

	return 0;
}

int HalBtDataHandleReceivedMutlipleBtDeviceData(struct HalBtPluginData *halBtPluginData, json_object *currentMultipleBtDeviceDataJ)
{
	int err = 0;
	size_t idx, btDeviceNumber;

	if(! halBtPluginData || ! currentMultipleBtDeviceDataJ)
		return -1;

	if(! json_object_is_type(currentMultipleBtDeviceDataJ, json_type_array))
		return -2;

	btDeviceNumber = json_object_array_length(currentMultipleBtDeviceDataJ);

	for(idx = 0; idx < btDeviceNumber; idx++) {
		if((err = HalBtDataHandleReceivedSingleBtDeviceData(halBtPluginData, json_object_array_get_idx(currentMultipleBtDeviceDataJ, (unsigned int) idx))))
			return ((int) idx * err * 10);
	}

	return 0;
}