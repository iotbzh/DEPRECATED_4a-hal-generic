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

#ifndef _HAL_UTILITIES_DATA_INCLUDE_
#define _HAL_UTILITIES_DATA_INCLUDE_

#include <stdio.h>
#include <stdbool.h>

#include <wrap-json.h>

#include <afb-definitions.h>

#include <ctl-config.h>

#include "../4a-hal-controllers/4a-hal-controllers-alsacore-link.h"

#define ALSA_MAX_CARD 32

// Enum for sharing hal (controller or external) status
enum HalStatus {
	HAL_STATUS_UNAVAILABLE=0,
	HAL_STATUS_AVAILABLE=1,
	HAL_STATUS_READY=2
};

// Structure to store stream data
struct CtlHalStreamData {
	char *name;
	char *streamCardId;
};

// Structure to store stream data table
struct CtlHalStreamsDataT {
	struct CtlHalStreamData *data;
	unsigned int count;
};

// Structure to store specific controller hal  data
struct CtlHalSpecificData {
	char *mixerApiName;
	char *mixerVerbName;
	json_object *halMixerJ;

	struct CtlHalStreamsDataT ctlHalStreamsData;
	struct CtlHalAlsaMapT *ctlHalAlsaMapT;

	AFB_ApiT apiHandle;
	CtlConfigT *ctrlConfig;
};

// Structure to store specific hal (controller or external) data
struct SpecificHalData {
	char *apiName;
	enum HalStatus status;
	char *sndCardPath;
	int sndCardId;
	char *info;
	bool internal;

	char *author;
	char *version;
	char *date;
	// Can be beefed up if needed

	struct CtlHalSpecificData *ctlHalSpecificData;		// Can be NULL if external api

	struct SpecificHalData *next;
};

// Structure to store hal manager data
struct HalMgrData {
	char *apiName;
	char *info;

	struct SpecificHalData usedCardId[ALSA_MAX_CARD];

	AFB_ApiT apiHandle;

	struct SpecificHalData *first;
};

// Exported verbs for 'struct SpecificHalData' handling
struct SpecificHalData *HalUtlAddHalApiToHalList(struct HalMgrData *HalMgrGlobalData);
uint8_t HalUtlRemoveSelectedHalFromList(struct HalMgrData *HalMgrGlobalData, struct SpecificHalData *ApiToRemove);
uint64_t HalUtlRemoveAllHalFromList(struct HalMgrData *HalMgrGlobalData);
uint64_t HalUtlGetNumberOfHalInList(struct HalMgrData *HalMgrGlobalData);
struct SpecificHalData *HalUtlSearchHalDataByApiName(struct HalMgrData *HalMgrGlobalData, char *apiName);

// Exported verbs for 'struct HalMgrData' handling
uint8_t HalUtlInitializeHalMgrData(AFB_ApiT apiHandle, struct HalMgrData *HalMgrGlobalData, char *apiName, char *info);
void HalUtlRemoveHalMgrData(struct HalMgrData *HalMgrGlobalData);

#endif /* _HAL_UTILITIES_DATA_INCLUDE_ */