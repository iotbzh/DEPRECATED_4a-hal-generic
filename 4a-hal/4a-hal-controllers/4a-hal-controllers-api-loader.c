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
#include <stdbool.h>
#include <string.h>

#include <filescan-utils.h>
#include <wrap-json.h>

#include <ctl-config.h>

#include "../4a-hal-utilities/4a-hal-utilities-verbs-loader.h"

#include "4a-hal-controllers-api-loader.h"
#include "4a-hal-controllers-alsacore-link.h"
#include "4a-hal-controllers-cb.h"
#include "4a-hal-controllers-mixer-handler.h"

// Default api to print log when apihandle not available
AFB_ApiT AFB_default;

/*******************************************************************************
 *		Json parsing functions using app controller		       *
 ******************************************************************************/

// Config Section definition
static CtlSectionT ctrlSections[] =
{
	{.key="plugins" , .loadCB= PluginConfig},
	{.key="onload"  , .loadCB= OnloadConfig},
	{.key="controls", .loadCB= ControlConfig},
	{.key="events"  , .loadCB= EventConfig},
	{.key="halmap", .loadCB= HalCtlsHalMapConfig},
	{.key="halmixer", .loadCB= HalCtlsHalMixerConfig},
	{.key=NULL}
};

/*******************************************************************************
 *		Dynamic HAL verbs' functions				       *
 ******************************************************************************/

// Every HAL export the same API & Interface Mapping from SndCard to AudioLogic is done through alsaHalSndCardT
static struct HalUtlApiVerb CtlHalDynApiStaticVerbs[] =
{
	/* VERB'S NAME			FUNCTION TO CALL		SHORT DESCRIPTION */
	{ .verb = "info",		.callback = HalCtlsInfo,	.info = "List available streams/playbacks/... for this api"},
	{ .verb = NULL }		// Marker for end of the array
};

/*******************************************************************************
 *		Dynamic API functions for app controller		       *
 *		TBD JAI : Use API-V3 instead of API-PRE-V3		       *
 ******************************************************************************/

static int HalCtlsInitOneApi(AFB_ApiT apiHandle)
{
	int err;

	CtlConfigT *ctrlConfig;
	struct SpecificHalData *currentCtlHalData;

	if(! apiHandle)
		return -1;

	// Hugely hack to make all V2 AFB_DEBUG to work in fileutils
	AFB_default = apiHandle;

	// Retrieve section config from api handle
	ctrlConfig = (CtlConfigT *) afb_dynapi_get_userdata(apiHandle);
	if(! ctrlConfig)
		return -2;

	currentCtlHalData = (struct SpecificHalData *) ctrlConfig->external;
	if(! currentCtlHalData)
		return -3;

	// Fill SpecificHalDatadata structure
	currentCtlHalData->internal = true;

	currentCtlHalData->apiName = (char *) ctrlConfig->api;
	currentCtlHalData->sndCardPath = (char *) ctrlConfig->uid;
	currentCtlHalData->info = (char *) ctrlConfig->info;

	currentCtlHalData->author = (char *) ctrlConfig->author;
	currentCtlHalData->version = (char *) ctrlConfig->version;
	currentCtlHalData->date = (char *) ctrlConfig->date;

	currentCtlHalData->ctlHalSpecificData->apiHandle = apiHandle;
	currentCtlHalData->ctlHalSpecificData->ctrlConfig = ctrlConfig;

	currentCtlHalData->ctlHalSpecificData->ctlHalStreamsData.count = 0;

	currentCtlHalData->sndCardId = HalCtlsGetCardIdByCardPath(apiHandle, currentCtlHalData->sndCardPath);

	if(currentCtlHalData->sndCardId < 0)
		currentCtlHalData->status = HAL_STATUS_UNAVAILABLE;
	else {
		currentCtlHalData->status = HAL_STATUS_AVAILABLE;
		if((err = HalCtlsAttachToMixer(apiHandle))) {
			AFB_ApiError(apiHandle, "%s: Error %i while attaching to mixer", __func__, err);
			return -4;
		}
	}

	// TBD JAI: handle refresh of hal status for dynamic card (/dev/by-id)

	return 0;
}

static int HalCtlsLoadOneApi(void *cbdata, AFB_ApiT apiHandle)
{
	int err = 0;
	CtlConfigT *ctrlConfig;

	if(! cbdata || ! apiHandle )
		return -1;

	ctrlConfig = (CtlConfigT*) cbdata;

	// Save closure as api's data context
	afb_dynapi_set_userdata(apiHandle, ctrlConfig);

	// Add static controls verbs
	if(HalUtlLoadVerbs(apiHandle, CtlHalDynApiStaticVerbs)) {
		AFB_ApiError(apiHandle, "%s : load Section : fail to register static V2 verbs", __func__);
		return 1;
	}

	// Load section for corresponding Api
	err += CtlLoadSections(apiHandle, ctrlConfig, ctrlSections);

	// Declare an event manager for this Api
	afb_dynapi_on_event(apiHandle, HalCtlsDispatchApiEvent);

	// Init Api function (does not receive user closure ???)
	afb_dynapi_on_init(apiHandle, NULL);

	err += HalCtlsInitOneApi(apiHandle);

	err += CtlConfigExec(apiHandle, ctrlConfig);

	return err;
}

int HalCtlsCreateApi(AFB_ApiT apiHandle, char *path, struct HalMgrData *HalMgrGlobalData)
{
	CtlConfigT *ctrlConfig;
	struct SpecificHalData *currentCtlHalData;

	if(! apiHandle || ! path || ! HalMgrGlobalData)
		return -1;

	// Create one Api per file
	ctrlConfig = CtlLoadMetaData(apiHandle, path);
	if(! ctrlConfig) {
		AFB_ApiError(apiHandle, "%s: No valid control config file in:\n-- %s", __func__, path);
		return -2;
	}

	if(! ctrlConfig->api) {
		AFB_ApiError(apiHandle, "%s: API Missing from metadata in:\n-- %s", __func__, path);
		return -3;
	}

	// Allocation of current hal controller data
	currentCtlHalData = HalUtlAddHalApiToHalList(&HalMgrGlobalData->first);
	if(! currentCtlHalData)
		return -4;

	// Stores current hal controller data in controller config
	ctrlConfig->external = (void *) currentCtlHalData;

	// Allocation of the structure that will be used to store specific hal controller data
	currentCtlHalData->ctlHalSpecificData = calloc(1, sizeof(struct CtlHalSpecificData));

	// Create one API (Pre-V3 return code ToBeChanged)
	return afb_dynapi_new_api(apiHandle, ctrlConfig->api, ctrlConfig->info, 1, HalCtlsLoadOneApi, ctrlConfig);
}

int HalCtlsCreateAllApi(AFB_ApiT apiHandle, struct HalMgrData *HalMgrGlobalData)
{
	int index, status = 0;
	char *dirList, *fileName, *fullPath;
	char filePath[CONTROL_MAXPATH_LEN];

	filePath[CONTROL_MAXPATH_LEN - 1] = '\0';

	json_object *configJ, *entryJ;

	if(! apiHandle || ! HalMgrGlobalData)
		return -1;

	// Hugely hack to make all V2 AFB_DEBUG to work in fileutils
	AFB_default = apiHandle;

	AFB_ApiNotice(apiHandle, "In %s", __func__);

	dirList = getenv("CONTROL_CONFIG_PATH");
	if(! dirList)
		dirList = CONTROL_CONFIG_PATH;

	configJ = CtlConfigScan(dirList, "hal");
	if(! configJ) {
		AFB_ApiError(apiHandle, "%s: No hal-(binder-middle-name)*.json config file(s) found in %s ", __func__, dirList);
		return -2;
	}

	// We load 1st file others are just warnings
	for(index = 0; index < json_object_array_length(configJ); index++) {
		entryJ = json_object_array_get_idx(configJ, index);

		if(wrap_json_unpack(entryJ, "{s:s, s:s !}", "fullpath", &fullPath, "filename", &fileName)) {
			AFB_ApiError(apiHandle, "%s: HOOPs invalid JSON entry = %s", __func__, json_object_get_string(entryJ));
			return -1;
		}

		strncpy(filePath, fullPath, sizeof(filePath) - 1);
		strncat(filePath, "/", sizeof(filePath));
		strncat(filePath, fileName, sizeof(filePath));

		if(HalCtlsCreateApi(apiHandle, filePath, HalMgrGlobalData) < 0)
			status--;
	}

	return status;
}