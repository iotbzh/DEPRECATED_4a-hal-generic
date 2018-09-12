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
	char *currentBtDeviceName, *currentBtDeviceAddress, *currentBtDevicePath, *currentBtDeviceHci;

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

	if(wrap_json_unpack(currentSingleBtDeviceDataJ,
			    "{s:s s:s s:s}",
			    "Name", &currentBtDeviceName,
			    "Address", &currentBtDeviceAddress,
			    "Path", &currentBtDevicePath)) {
		HalBtDataRemoveSelectedBtDeviceFromList(firstBtDeviceData, currentBtDeviceData);
		return NULL;
	}

	if(asprintf(&currentBtDeviceData->uid, "BT#%s", currentBtDeviceAddress) == -1) {
		HalBtDataRemoveSelectedBtDeviceFromList(firstBtDeviceData, currentBtDeviceData);
		return NULL;
	}

	currentBtDeviceHci = strtok(currentBtDevicePath, "/");
	while(currentBtDeviceHci && strncmp(currentBtDeviceHci, "hci", 3))
		currentBtDeviceHci = strtok(NULL, "/");

	if((! currentBtDeviceHci) || (! (currentBtDeviceData->hci = strdup(currentBtDeviceHci)))) {
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

	return currentBtDeviceData;
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
	int btProfilesCount;

	unsigned int idx = 0, currentBtDeviceIsConnected, currentBtDeviceIsA2DP;

	char *currentBtDeviceAddress, *currentBtDeviceIsConnectedString, *currentBtDeviceIsAVPConnectedString;

	json_object *currentBtAllProfilesJ = NULL, *currentBtCurrentProfileJ;

	struct HalBtDeviceData *currentBtDevice;
	if(! halBtPluginData || ! currentSingleBtDeviceDataJ)
		return -1;

	if(wrap_json_unpack(currentSingleBtDeviceDataJ,
			    "{s:s s:s s:s s?:o}",
			    "Address", &currentBtDeviceAddress,
			    "Connected", &currentBtDeviceIsConnectedString,
			    "AVPConnected", &currentBtDeviceIsAVPConnectedString,
			    "UUIDs", &currentBtAllProfilesJ)) {
		return -2;
	}

	if(! currentBtAllProfilesJ) {
		AFB_ApiInfo(halBtPluginData->currentHalApiHandle, "Bluetooth device (address = %s) has no specified profiles, ignore it", currentBtDeviceAddress);
		return 0;
	}

	if(json_object_is_type(currentBtAllProfilesJ, json_type_array)) {
		btProfilesCount = json_object_array_length(currentBtAllProfilesJ);

		while(idx < btProfilesCount) {
			currentBtCurrentProfileJ = json_object_array_get_idx(currentBtAllProfilesJ, idx);

			if(json_object_is_type(currentBtCurrentProfileJ, json_type_string) &&
			   ! strncasecmp(json_object_get_string(currentBtCurrentProfileJ), A2DP_AUDIOSOURCE_UUID, sizeof(A2DP_AUDIOSOURCE_UUID))) {
				currentBtDeviceIsA2DP = 1;
				break;
			}

			idx++;
		}
	}
	else if(json_object_is_type(currentBtAllProfilesJ, json_type_string) &&
		! strncasecmp(json_object_get_string(currentBtAllProfilesJ), A2DP_AUDIOSOURCE_UUID, sizeof(A2DP_AUDIOSOURCE_UUID))) {
		currentBtDeviceIsA2DP = 1;
	}

	if(! currentBtDeviceIsA2DP)
		return 0;

	currentBtDeviceIsConnected = ((! strncmp(currentBtDeviceIsConnectedString, "True", strlen(currentBtDeviceIsConnectedString))) &&
				      (! strncmp(currentBtDeviceIsAVPConnectedString, "True", strlen(currentBtDeviceIsAVPConnectedString))));

	currentBtDevice = HalBtDataSearchBtDeviceByAddress(&halBtPluginData->first, currentBtDeviceAddress);

	if(currentBtDevice && ! currentBtDeviceIsConnected) {
		if(HalBtDataRemoveSelectedBtDeviceFromList(&halBtPluginData->first, currentBtDevice))
			return -3;

		AFB_ApiInfo(halBtPluginData->currentHalApiHandle, "Bluetooth device (address = %s) successfully removed from list", currentBtDeviceAddress);

		if(halBtPluginData->selectedBtDevice == currentBtDevice) {
			halBtPluginData->selectedBtDevice = halBtPluginData->first;
			AFB_ApiDebug(halBtPluginData->currentHalApiHandle,
				     "Bluetooth selected device changed to '%s'",
				     halBtPluginData->selectedBtDevice ? halBtPluginData->selectedBtDevice->address : "none");
		}
	}
	else if(! currentBtDevice && currentBtDeviceIsConnected) {
		if(! HalBtDataAddBtDeviceToBtDeviceList(&halBtPluginData->first, currentSingleBtDeviceDataJ))
			return -4;

		AFB_ApiInfo(halBtPluginData->currentHalApiHandle, "Bluetooth device (address = %s) successfully added to list", currentBtDeviceAddress);

		if(! halBtPluginData->selectedBtDevice) {
			halBtPluginData->selectedBtDevice = halBtPluginData->first;
			AFB_ApiDebug(halBtPluginData->currentHalApiHandle,
				     "Bluetooth selected device changed to '%s'",
				     halBtPluginData->selectedBtDevice ? halBtPluginData->selectedBtDevice->address : "none");
		}
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
		if((err = HalBtDataHandleReceivedSingleBtDeviceData(halBtPluginData, json_object_array_get_idx(currentMultipleBtDeviceDataJ, (unsigned int) idx)))) {
			return ((int) idx * err * 10);
		}
	}

	return 0;
}
