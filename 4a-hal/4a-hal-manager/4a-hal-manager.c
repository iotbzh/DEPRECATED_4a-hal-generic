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

#include "../4a-hal-utilities/4a-hal-utilities-data.h"
#include "../4a-hal-utilities/4a-hal-utilities-verbs-loader.h"
#include "../4a-hal-controllers/4a-hal-controllers-api-loader.h"

#include "4a-hal-manager.h"
#include "4a-hal-manager-cb.h"

// Default api to print log when apihandle not available
AFB_ApiT AFB_default;

// Local (static) Hal manager data structure
static struct HalMgrData localHalMgrGlobalData;

/*******************************************************************************
 *		HAL Manager verbs table					       *
 ******************************************************************************/

// Hal manager exported verbs
struct HalUtlApiVerb HalManagerApiStaticVerbs[] =
{
	/* VERB'S NAME			FUNCTION TO CALL			SHORT DESCRIPTION */
	{ .verb = "ping",		.callback = HalMgrPing,			.info = "Ping test for DynApi"},
	{ .verb = "loaded",		.callback = HalMgrLoaded,		.info = "Show loaded HAL"},
	{ .verb = "load",		.callback = HalMgrLoad,			.info = "Load an external HAL to HAL Manager"},
	{ .verb = "unload",		.callback = HalMgrUnload,		.info = "Unload an external HAL to HAL Manager"},
	{ .verb = "subscribe",		.callback = HalMgrSubscribeEvent,	.info = "Subscribe to an event"},
	{ .verb = "unsuscribe",		.callback = HalMgrUnsubscribeEvent,	.info = "Unsubscribe to an event"},
	{ .verb = NULL }		// Marker for end of the array
};

/*******************************************************************************
 *		HAL Manager get first 'SpecificHalData' structure	       *
		from HAL list function					       *
 ******************************************************************************/

struct SpecificHalData **HalMngGetFirstHalData(void)
{
	return &localHalMgrGlobalData.first;
}

/*******************************************************************************
 *		Dynamic API functions for hal manager			       *
 *		TBD JAI : Use API-V3 instead of API-PRE-V3		       *
 ******************************************************************************/

static int HalMgrInitApi(AFB_ApiT apiHandle)
{
	struct HalMgrData *HalMgrGlobalData;

	if(! apiHandle)
		return -1;

	// Hugely hack to make all V2 AFB_DEBUG to work in fileutils
	AFB_default = apiHandle;

	// Retrieve section config from api handle
	HalMgrGlobalData = (struct HalMgrData *) afb_dynapi_get_userdata(apiHandle);
	if(! HalMgrGlobalData)
		return -2;

	if(HalUtlInitializeHalMgrData(apiHandle, HalMgrGlobalData, HAL_MANAGER_API_NAME, HAL_MANAGER_API_INFO))
		return -3;

	return 0;
}

static int HalMgrLoadApi(void *cbdata, AFB_ApiT apiHandle)
{
	struct HalMgrData *HalMgrGlobalData;

	if(! cbdata || ! apiHandle)
		return -1;

	HalMgrGlobalData = (struct HalMgrData *) cbdata;

	// Save closure as api's data context
	afb_dynapi_set_userdata(apiHandle, HalMgrGlobalData);

	// Add static controls verbs
	if(HalUtlLoadVerbs(apiHandle, HalManagerApiStaticVerbs)) {
		AFB_ApiError(apiHandle, "%s : load section : fail to register static V2 verbs", __func__);
		return 1;
	}

	// Declare an event manager for Hal Manager
	afb_dynapi_on_event(apiHandle, HalMgrDispatchApiEvent);

	// Init Api function (does not receive user closure ???)
	afb_dynapi_on_init(apiHandle, HalMgrInitApi);

	afb_dynapi_seal(apiHandle);

	return 0;
}

int HalMgrCreateApi(AFB_ApiT apiHandle, struct HalMgrData *HalMgrGlobalData)
{
	if(! apiHandle || ! HalMgrGlobalData)
		return -1;

	// Create one API (Pre-V3 return code ToBeChanged)
	return afb_dynapi_new_api(apiHandle, HAL_MANAGER_API_NAME, HAL_MANAGER_API_INFO, 1, HalMgrLoadApi, HalMgrGlobalData);
}

/*******************************************************************************
 *		Startup function					       *
 *		TBD JAI : Use API-V3 instead of API-PRE-V3		       *
 ******************************************************************************/

int afbBindingVdyn(AFB_ApiT apiHandle)
{
	int status = 0, rc;

	if(! apiHandle)
		return -1;

	// Hugely hack to make all V2 AFB_DEBUG to work in fileutils
	AFB_default = apiHandle;

	AFB_ApiNotice(apiHandle, "In %s", __func__);

	// Load Hal-Manager using DynApi
	rc = HalMgrCreateApi(apiHandle, &localHalMgrGlobalData);
	if(rc < 0)
		status--;

	// Load Hal-Ctls using DynApi
	rc = HalCtlsCreateAllApi(apiHandle, &localHalMgrGlobalData);
	if(rc < 0)
		status -= rc;

	return status;
}