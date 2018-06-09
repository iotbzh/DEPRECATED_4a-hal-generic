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

// TBD JAI : to implement
void HalMgrDispatchApiEvent(AFB_ApiT apiHandle, const char *evtLabel, json_object *eventJ)
{
	AFB_ApiWarning(apiHandle, "%s not implemented yet", __func__);
	// Use "4a-hal-manager-events.h" to handle events
}

/*******************************************************************************
 *		HAL Manager verbs functions				       *
 ******************************************************************************/

void HalMgrPing(AFB_ReqT request)
{
	static int count = 0;

	count++;

	if(request->dynapi)
		AFB_ReqNotice(request, "%s (%s): ping count = %d", request->api, request->dynapi->apiname, count);
	else
		AFB_ReqNotice(request, "%s: ping count = %d", request->api, count);

	AFB_ReqSuccess(request, json_object_new_int(count), NULL);

	return;
}

void HalMgrLoaded(AFB_ReqT request)
{
	int requestJsonErr, requestOptionValue;
	uint64_t cpt, numberOfLoadedApi;

	char cardIdString[10];

	AFB_ApiT apiHandle;
	struct HalMgrData *HalMgrGlobalData;
	struct SpecificHalData *currentHalData;

	json_object *requestJson, *requestAnswer, *apiObject;

	apiHandle = (AFB_ApiT ) afb_request_get_dynapi(request);
	if(! apiHandle) {
		AFB_ReqFail(request, "api_handle", "Can't get hal manager api handle");
		return;
	}

	HalMgrGlobalData = (struct HalMgrData *) afb_dynapi_get_userdata(apiHandle);
	if(! HalMgrGlobalData) {
		AFB_ReqFail(request, "hal_manager_data", "Can't get hal manager data");
		return;
	}

	requestJson = AFB_ReqJson(request);
	if(! requestJson) {
		AFB_ReqFail(request, "request_json", "Can't get request json");
		return;
	}

	numberOfLoadedApi = HalUtlGetNumberOfHalInList(HalMgrGlobalData);
	if(! numberOfLoadedApi) {
		AFB_ReqSuccess(request, NULL, "No Hal Api loaded");
		return;
	}

	requestAnswer = json_object_new_array();
	if(! requestAnswer) {
		AFB_ReqFail(request, "json_answer", "Can't generate json answer");
		return;
	}

	currentHalData = HalMgrGlobalData->first;

	// Get request option
	requestJsonErr = wrap_json_unpack(requestJson, "{s:i}", "verbose", &requestOptionValue);

	// Case if request key is 'verbose' and value is bigger than 0
	if(! requestJsonErr && requestOptionValue > 0) {
		for(cpt = 0; cpt < numberOfLoadedApi; cpt++) {
			if(currentHalData->sndCardId >= 0)
				snprintf(cardIdString, 6, "hw:%i", currentHalData->sndCardId);
			else
				snprintf(cardIdString, 10, "not-found");

			wrap_json_pack(&apiObject,
				       "{s:s s:i s:s s:i s:s s:s s:s s:s s:s}",
				       "api", currentHalData->apiName,
				       "status", (int) currentHalData->status,
				       "sndcard", currentHalData->sndCardPath,
				       "internal", (int) currentHalData->internal,
				       "info", currentHalData->info ? currentHalData->info : "",
				       "author", currentHalData->author ? currentHalData->author : "",
				       "version", currentHalData->version ? currentHalData->version : "",
				       "date", currentHalData->date ? currentHalData->date : "",
				       "snd-dev-id", cardIdString);
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

	AFB_ReqSuccess(request, requestAnswer, "Requested data");
}

void HalMgrLoad(AFB_ReqT request)
{
	int cardId = -1;

	char *apiName, *sndCardPath, *info = NULL, *author = NULL, *version = NULL, *date = NULL;

	AFB_ApiT apiHandle;
	struct HalMgrData *HalMgrGlobalData;
	struct SpecificHalData *addedHal;

	struct json_object *requestJson, *apiReceviedMetadata;

	apiHandle = (AFB_ApiT) afb_request_get_dynapi(request);
	if(! apiHandle) {
		AFB_ReqFail(request, "api_handle", "Can't get hal manager api handle");
		return;
	}

	HalMgrGlobalData = (struct HalMgrData *) afb_dynapi_get_userdata(apiHandle);
	if(! HalMgrGlobalData) {
		AFB_ReqFail(request, "hal_manager_data", "Can't get hal manager data");
		return;
	}

	requestJson = AFB_ReqJson(request);
	if(! requestJson) {
		AFB_ReqFail(request, "request_json", "Can't get request json");
		return;
	}

	if(! json_object_object_get_ex(requestJson, "metadata", &apiReceviedMetadata)) {
		AFB_ReqFail(request, "api_metadata", "Can't get api to register metadata");
		return;
	}

	if(wrap_json_unpack(apiReceviedMetadata,
			    "{s:s s:s s?:s s?:s s?:s s?:s s?:i}",
			    "api", &apiName,
			    "uid", &sndCardPath,
			    "info", &info,
			    "author", &author,
			    "version", &version,
			    "date", &date,
			    "snd-dev-id", &cardId)) {
		AFB_ReqFail(request, "api_metadata", "Can't metadata of api to register");
		return;
	}

	addedHal = HalUtlAddHalApiToHalList(HalMgrGlobalData);

	addedHal->internal = false;
	// TBD JAI : initialize external to unavailable once event from external hal will be handled
	addedHal->status = HAL_STATUS_READY;

	addedHal->apiName = strdup(apiName);
	addedHal->sndCardPath = strdup(sndCardPath);

	if(info)
		addedHal->info = strdup(info);

	if(author)
		addedHal->author = strdup(author);

	if(version)
		addedHal->version = strdup(version);

	if(date)
		addedHal->date = strdup(date);

	addedHal->sndCardId = cardId;

	// TBD JAI: add subscription to this api status events, if subscription fails, remove hal from list

	AFB_ReqSuccess(request, NULL, "Api successfully registered");
}

void HalMgrUnload(AFB_ReqT request)
{
	char *apiName;

	AFB_ApiT apiHandle;
	struct HalMgrData *HalMgrGlobalData;
	struct SpecificHalData *HalToRemove;

	struct json_object *requestJson;

	apiHandle = (AFB_ApiT) afb_request_get_dynapi(request);
	if(! apiHandle) {
		AFB_ReqFail(request, "api_handle", "Can't get hal manager api handle");
		return;
	}

	HalMgrGlobalData = (struct HalMgrData *) afb_dynapi_get_userdata(apiHandle);
	if(! HalMgrGlobalData) {
		AFB_ReqFail(request, "hal_manager_data", "Can't get hal manager data");
		return;
	}

	requestJson = AFB_ReqJson(request);
	if(! requestJson) {
		AFB_ReqFail(request, "request_json", "Can't get request json");
		return;
	}

	if(wrap_json_unpack(requestJson, "{s:s}", "api", &apiName)) {
		AFB_ReqFail(request, "requested_api", "Can't get api to remove");
		return;
	}

	HalToRemove = HalUtlSearchHalDataByApiName(HalMgrGlobalData, apiName);
	if(! HalToRemove) {
		AFB_ReqFail(request, "requested_api", "Can't find api to remove");
		return;
	}

	if(HalToRemove->internal) {
		AFB_ReqFail(request, "requested_api", "Can't remove an internal controller api");
		return;
	}

	if(HalUtlRemoveSelectedHalFromList(HalMgrGlobalData, HalToRemove)) {
		AFB_ReqFail(request, "unregister_error", "Didn't succeed to remove specified api");
		return;
	}

	// TBD JAI: remove subscription to this api status events

	AFB_ReqSuccess(request, NULL, "Api successfully unregistered");
}

// TBD JAI : to implement
void HalMgrSubscribeEvent(AFB_ReqT request)
{
	AFB_ReqWarning(request, "%s not implemented yet", __func__);

	AFB_ReqSuccess(request, json_object_new_boolean(false), NULL);
}

// TBD JAI : to implement
void HalMgrUnsubscribeEvent(AFB_ReqT request)
{
	AFB_ReqWarning(request, "%s not implemented yet", __func__);

	AFB_ReqSuccess(request, json_object_new_boolean(false), NULL);
}