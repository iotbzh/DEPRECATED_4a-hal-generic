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

#ifndef _HAL_BT_DATA_INCLUDE_
#define _HAL_BT_DATA_INCLUDE_

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <json-c/json.h>

#include <4a-hal-utilities-data.h>

// Structure to store bluetooth device data
struct HalBtDeviceData {
	char *uid;

	char *hci;
	char *name;
	char *address;

	unsigned int a2dp;

	struct HalBtDeviceData *next;
};

// Structure to store hal bluetooth plugin data
struct HalBtPluginData {
	unsigned int halBtPluginEnabled;

	struct SpecificHalData *currentHalData;

	unsigned int btStreamEnabled;

	struct HalBtDeviceData *selectedBtDevice;
	struct HalBtDeviceData *first;
};

// Exported verbs for 'struct HalBtDeviceData' list (available in 'struct HalBtPluginData') handling
int HalBtDataGetNumberOfBtDeviceInList(struct HalBtDeviceData **firstBtDeviceData);
struct HalBtDeviceData *HalBtDataSearchBtDeviceByAddress(struct HalBtDeviceData **firstBtDeviceData, char *btAddress);
int HalBtDataHandleReceivedSingleBtDeviceData(struct HalBtPluginData *halBtPluginData, json_object *currentSingleBtDeviceDataJ);
int HalBtDataHandleReceivedMutlipleBtDeviceData(struct HalBtPluginData *halBtPluginData, json_object *currentMultipleBtDeviceDataJ);

#endif /* _HAL_BT_DATA_INCLUDE_ */