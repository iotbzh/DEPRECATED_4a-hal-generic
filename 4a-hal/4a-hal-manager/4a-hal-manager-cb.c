/*
 * Copyright (C) 2016 "IoT.bzh"
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

#include "../4a-hal-utilities/4a-hal-utilities-data.h"

#include "4a-hal-manager-cb.h"
#include "4a-hal-manager-events.h"

/*******************************************************************************
 *		HAL Manager event handler function			       *
 ******************************************************************************/

// TODO JAI : to implement
void HalMgrDispatchApiEvent(afb_dynapi *apiHandle, const char *evtLabel, struct json_object *eventJ)
{
	AFB_DYNAPI_WARNING(apiHandle, "JAI :%s not implemented yet", __func__);
	// Use "4a-hal-manager-events.h" to handle events
}

/*******************************************************************************
 *	TODO JAI : Present for test, delete this function when validated       *
 ******************************************************************************/

void HalMgrPing(afb_request *request)
{
	static int count = 0;

	count++;

	if(request->dynapi)
		AFB_REQUEST_NOTICE(request, "JAI :%s (%s): ping count = %d", request->api, request->dynapi->apiname, count);
	else
		AFB_REQUEST_NOTICE(request, "JAI: %s: ping count = %d", request->api, count);

	afb_request_success(request, json_object_new_int(count), NULL);

	return;
}

/*******************************************************************************
 *		HAL Manager verbs functions				       *
 ******************************************************************************/

void HalMgrLoaded(afb_request *request)
{
	int requestJsonErr;
	uint64_t cpt, numberOfLoadedApi;
	char *requestOption;

	afb_dynapi *apiHandle;
	struct HalMgrData *HalMgrGlobalData;
	struct SpecificHalData *currentHalData;

	struct json_object *requestJson;
	struct json_object *requestAnswer;
	struct json_object *apiObject;

	AFB_REQUEST_WARNING(request, "JAI :%s not completelly implemented yet", __func__);

	apiHandle = (afb_dynapi *) afb_request_get_dynapi(request);
	if(! apiHandle) {
		afb_request_fail(request, "ERROR", "Can't get HalManager Api handle");
		return;
	}

	HalMgrGlobalData = (struct HalMgrData *) afb_dynapi_get_userdata(apiHandle);
	if(! HalMgrGlobalData) {
		afb_request_fail(request, "ERROR", "Can't get HalManager data");
		return;
	}

	requestJson = afb_request_json(request);
	if(! requestJson) {
		afb_request_fail(request, "ERROR", "Can't get request json");
		return;
	}

	numberOfLoadedApi = HalUtlGetNumberOfHalInList(HalMgrGlobalData);
	if(! numberOfLoadedApi) {
		afb_request_success(request, NULL, "No Hal Api loaded");
		return;
	}

	requestAnswer = json_object_new_array();
	if(! requestAnswer) {
		afb_request_fail(request, "ERROR", "Can't generate json answer");
		return;
	}

	currentHalData = HalMgrGlobalData->first;

	// Get request option
	requestJsonErr = wrap_json_unpack(requestJson, "{s:s}", "option", &requestOption);

	if(! requestJsonErr && ! strcmp(requestOption, "status")) {		// Case if request option is 'status'
		for(cpt = 0; cpt < numberOfLoadedApi; cpt++) {
			wrap_json_pack(&apiObject,
				       "{s:i}",
				       currentHalData->apiName,
				       (unsigned int) currentHalData->status);
			json_object_array_add(requestAnswer, apiObject);

			currentHalData = currentHalData->next;
		}
	}
	else if(! requestJsonErr && ! strcmp(requestOption, "metadata")) {	// Case if request option is 'metadata'
		for(cpt = 0; cpt < numberOfLoadedApi; cpt++) {
			wrap_json_pack(&apiObject,
				       "{s:s s:i s:s s:i s:s s:s s:s}",
				       "api", currentHalData->apiName,
				       "status", (int) currentHalData->status,
				       "sndcard", currentHalData->sndCard,
				       "internal", (int) currentHalData->internal,
				       "author", currentHalData->author,
				       "version", currentHalData->version,
				       "date", currentHalData->date);
			json_object_array_add(requestAnswer, apiObject);

			currentHalData = currentHalData->next;
		}
	}
	else {									// Case if request option is empty or not valid
		for(cpt = 0; cpt < numberOfLoadedApi; cpt++) {
			json_object_array_add(requestAnswer, json_object_new_string(currentHalData->apiName));

			currentHalData = currentHalData->next;
		}
	}

	afb_request_success(request, requestAnswer, "Requested data");
}

// TODO JAI : to implement
void HalMgrLoad(afb_request *request)
{
	AFB_REQUEST_WARNING(request, "JAI :%s not implemented yet", __func__);

	afb_request_success(request, json_object_new_boolean(false), NULL);
}

// TODO JAI : to implement
void HalMgrUnload(afb_request *request)
{
	AFB_REQUEST_WARNING(request, "JAI :%s not implemented yet", __func__);

	afb_request_success(request, json_object_new_boolean(false), NULL);
}

// TODO JAI : to implement
void HalMgrSubscribeEvent(afb_request *request)
{
	AFB_REQUEST_WARNING(request, "JAI :%s not implemented yet", __func__);

	afb_request_success(request, json_object_new_boolean(false), NULL);
}

// TODO JAI : to implement
void HalMgrUnsubscribeEvent(afb_request *request)
{
	AFB_REQUEST_WARNING(request, "JAI :%s not implemented yet", __func__);

	afb_request_success(request, json_object_new_boolean(false), NULL);
}