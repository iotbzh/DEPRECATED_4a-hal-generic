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

#include "../4a-hal-utilities/4a-hal-utilities-data.h"

#include "4a-hal-manager-cb.h"
#include "4a-hal-manager-events.h"

/*******************************************************************************
 *		HAL Manager event handler function			       *
 ******************************************************************************/

// TODO JAI : to implement
void HalMgrDispatchApiEvent(afb_dynapi *apiHandle, const char *evtLabel, json_object *eventJ)
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
	int requestJsonErr, requestOptionValue;
	uint64_t cpt, numberOfLoadedApi;

	afb_dynapi *apiHandle;
	struct HalMgrData *HalMgrGlobalData;
	struct SpecificHalData *currentHalData;

	json_object *requestJson, *requestAnswer, *apiObject;

	apiHandle = (afb_dynapi *) afb_request_get_dynapi(request);
	if(! apiHandle) {
		afb_request_fail(request, "api_handle", "Can't get hal manager api handle");
		return;
	}

	HalMgrGlobalData = (struct HalMgrData *) afb_dynapi_get_userdata(apiHandle);
	if(! HalMgrGlobalData) {
		afb_request_fail(request, "hal_manager_data", "Can't get hal manager data");
		return;
	}

	requestJson = afb_request_json(request);
	if(! requestJson) {
		afb_request_fail(request, "request_json", "Can't get request json");
		return;
	}

	numberOfLoadedApi = HalUtlGetNumberOfHalInList(HalMgrGlobalData);
	if(! numberOfLoadedApi) {
		afb_request_success(request, NULL, "No Hal Api loaded");
		return;
	}

	requestAnswer = json_object_new_array();
	if(! requestAnswer) {
		afb_request_fail(request, "json_answer", "Can't generate json answer");
		return;
	}

	currentHalData = HalMgrGlobalData->first;

	// Get request option
	requestJsonErr = wrap_json_unpack(requestJson, "{s:i}", "verbose", &requestOptionValue);

	// Case if request key is 'verbose' and value is bigger than 0
	if(! requestJsonErr && requestOptionValue > 0) {
		for(cpt = 0; cpt < numberOfLoadedApi; cpt++) {
			wrap_json_pack(&apiObject,
				       "{s:s s:i s:s s:i s:s s:s s:s s:s}",
				       "api", currentHalData->apiName,
				       "status", (int) currentHalData->status,
				       "sndcard", currentHalData->sndCard,
				       "internal", (int) currentHalData->internal,
				       "info", currentHalData->info ? currentHalData->info : "",
				       "author", currentHalData->author ? currentHalData->author : "",
				       "version", currentHalData->version ? currentHalData->version : "",
				       "date", currentHalData->date ? currentHalData->date : "");
			json_object_array_add(requestAnswer, apiObject);

			currentHalData = currentHalData->next;
		}
	}
	// Case if request is empty or not handled
	else {
		for(cpt = 0; cpt < numberOfLoadedApi; cpt++) {
			json_object_array_add(requestAnswer, json_object_new_string(currentHalData->apiName));

			currentHalData = currentHalData->next;
		}
	}

	afb_request_success(request, requestAnswer, "Requested data");
}

void HalMgrLoad(afb_request *request)
{
	char *apiName, *sndCard, *info = NULL, *author = NULL, *version = NULL, *date = NULL;

	afb_dynapi *apiHandle;
	struct HalMgrData *HalMgrGlobalData;
	struct SpecificHalData *addedHal;

	struct json_object *requestJson, *apiReceviedMetadata;

	apiHandle = (afb_dynapi *) afb_request_get_dynapi(request);
	if(! apiHandle) {
		afb_request_fail(request, "api_handle", "Can't get hal manager api handle");
		return;
	}

	HalMgrGlobalData = (struct HalMgrData *) afb_dynapi_get_userdata(apiHandle);
	if(! HalMgrGlobalData) {
		afb_request_fail(request, "hal_manager_data", "Can't get hal manager data");
		return;
	}

	requestJson = afb_request_json(request);
	if(! requestJson) {
		afb_request_fail(request, "request_json", "Can't get request json");
		return;
	}

	if(! json_object_object_get_ex(requestJson, "metadata", &apiReceviedMetadata)) {
		afb_request_fail(request, "api_metadata", "Can't get api to register metadata");
		return;
	}

	if(wrap_json_unpack(apiReceviedMetadata,
			    "{s:s s:s s?:s s?:s s?:s s?:s}",
			    "api", &apiName,
			    "uid", &sndCard,
			    "info", &info,
			    "author", &author,
			    "version", &version,
			    "date", &date)) {
		afb_request_fail(request, "api_metadata", "Can't metadata of api to register");
		return;
	}

	// TODO JAI: try connect to the api to test if it really exists

	addedHal = HalUtlAddHalApiToHalList(HalMgrGlobalData);

	addedHal->internal = false;
	addedHal->status = HAL_STATUS_UNAVAILABLE;

	addedHal->apiName = strdup(apiName);
	addedHal->sndCard = strdup(sndCard);

	if(info)
		addedHal->info = strdup(info);

	if(author)
		addedHal->author = strdup(author);

	if(version)
		addedHal->version = strdup(version);

	if(date)
		addedHal->date = strdup(date);

	// TODO JAI: add subscription to this api status events

	afb_request_success(request, NULL, "Api successfully registered");
}

void HalMgrUnload(afb_request *request)
{
	char *apiName;

	afb_dynapi *apiHandle;
	struct HalMgrData *HalMgrGlobalData;
	struct SpecificHalData *HalToRemove;

	struct json_object *requestJson;

	apiHandle = (afb_dynapi *) afb_request_get_dynapi(request);
	if(! apiHandle) {
		afb_request_fail(request, "api_handle", "Can't get hal manager api handle");
		return;
	}

	HalMgrGlobalData = (struct HalMgrData *) afb_dynapi_get_userdata(apiHandle);
	if(! HalMgrGlobalData) {
		afb_request_fail(request, "hal_manager_data", "Can't get hal manager data");
		return;
	}

	requestJson = afb_request_json(request);
	if(! requestJson) {
		afb_request_fail(request, "request_json", "Can't get request json");
		return;
	}

	if(wrap_json_unpack(requestJson, "{s:s}", "api", &apiName)) {
		afb_request_fail(request, "requested_api", "Can't get api to remove");
		return;
	}

	HalToRemove = HalUtlSearchHalDataByApiName(HalMgrGlobalData, apiName);
	if(! HalToRemove) {
		afb_request_fail(request, "requested_api", "Can't find api to remove");
		return;
	}

	if(HalToRemove->internal) {
		afb_request_fail(request, "requested_api", "Can't remove an internal controller api");
		return;
	}

	if(HalUtlRemoveSelectedHalFromList(HalMgrGlobalData, HalToRemove)) {
		afb_request_fail(request, "unregister_error", "Didn't succeed to remove specified api");
		return;
	}

	// TODO JAI: remove subscription to this api status events

	afb_request_success(request, NULL, "Api successfully unregistered");
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