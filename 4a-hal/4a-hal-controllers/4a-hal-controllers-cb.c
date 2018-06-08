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
#include "../4a-hal-utilities/4a-hal-utilities-appfw-responses-handler.h"

#include "4a-hal-controllers-cb.h"
#include "4a-hal-controllers-mixer-handler.h"
#include "4a-hal-controllers-alsacore-link.h"

/*******************************************************************************
 *		HAL controllers sections parsing functions		       *
 ******************************************************************************/

int HalCtlsHalMixerConfig(AFB_ApiT apiHandle, CtlSectionT *section, json_object *MixerJ)
{
	CtlConfigT *ctrlConfig;
	struct SpecificHalData *currentHalData;

	if(! apiHandle || ! section)
		return -1;

	if(MixerJ) {
		ctrlConfig = (CtlConfigT *) afb_dynapi_get_userdata(apiHandle);
		if(! ctrlConfig)
			return -2;

		currentHalData = (struct SpecificHalData *) ctrlConfig->external;
		if(! currentHalData)
			return -3;

		if(json_object_is_type(MixerJ, json_type_object))
			currentHalData->ctlHalSpecificData->halMixerJ = MixerJ;
		else
			return -4;

		if(wrap_json_unpack(MixerJ, "{s:s}", "mixerapi", &currentHalData->ctlHalSpecificData->mixerApiName))
			return -5;

		if(wrap_json_unpack(MixerJ, "{s:s}", "uid", &currentHalData->ctlHalSpecificData->mixerVerbName))
			return -6;
	}

	return 0;
}

// TODO JAI : to implement
int HalCtlsHalMapConfig(AFB_ApiT apiHandle, CtlSectionT *section, json_object *StreamControlsJ)
{
	AFB_ApiWarning(apiHandle, "JAI :%s not implemented yet", __func__);

	return 0;
}

/*******************************************************************************
 *		HAL controllers verbs functions				       *
 ******************************************************************************/

void HalCtlsActionOnStream(AFB_ReqT request)
{
	int verbToCallSize;

	char *apiToCall, *mixerVerbName, *verbToCall;

	AFB_ApiT apiHandle;
	CtlConfigT *ctrlConfig;

	struct SpecificHalData *currentCtlHalData;

	json_object *requestJson, *returnJ, *toReturnJ;

	apiHandle = (AFB_ApiT) afb_request_get_dynapi(request);
	if(! apiHandle) {
		AFB_ReqFail(request, "api_handle", "Can't get current hal controller api handle");
		return;
	}

	ctrlConfig = (CtlConfigT *) afb_dynapi_get_userdata(apiHandle);
	if(! ctrlConfig) {
		AFB_ReqFail(request, "hal_controller_config", "Can't get current hal controller config");
		return;
	}

	currentCtlHalData = (struct SpecificHalData *) ctrlConfig->external;
	if(! currentCtlHalData) {
		AFB_ReqFail(request, "hal_controller_data", "Can't get current hal controller data");
		return;
	}

	requestJson = AFB_ReqJson(request);
	if(! requestJson) {
		AFB_ReqFail(request, "request_json", "Can't get request json");
		return;
	}

	apiToCall = currentCtlHalData->ctlHalSpecificData->mixerApiName;
	if(! apiToCall) {
		AFB_ReqFail(request, "mixer_api", "Can't get mixer api");
		return;
	}

	mixerVerbName = currentCtlHalData->ctlHalSpecificData->mixerVerbName;
	if(! mixerVerbName) {
		AFB_ReqFail(request, "hal_softmixer_verb", "Can't get hal mixer verb prefix");
		return;
	}

	if(currentCtlHalData->status != HAL_STATUS_READY) {
		AFB_ReqFail(request, "hal_not_ready", "Seems that hal is not ready");
		return;
	}

	// TODO JAI : remove verb to call prefix, each hal should have its own api in softmixer, and each streams should be created as verb by mixer
	verbToCallSize = (int) strlen(mixerVerbName) + (int) strlen(request->verb) + 2;
	verbToCall = (char *) alloca(verbToCallSize * sizeof(char));
	verbToCall[0] = '\0';
	verbToCall[verbToCallSize - 1] = '\0';

	strcat(verbToCall, mixerVerbName);
	strcat(verbToCall, "/");
	strcat(verbToCall, request->verb);

	if(AFB_ServiceSync(apiHandle, apiToCall, verbToCall, json_object_get(requestJson), &returnJ)) {
		HalUtlHandleAppFwCallErrorInRequest(request, apiToCall, verbToCall, returnJ, "stream_action");
	}
	else if(json_object_object_get_ex(returnJ, "response", &toReturnJ)){
		AFB_ReqSucessF(request,
			       toReturnJ,
			       "Action %s correctly transferred to %s without any error raised",
			       verbToCall,
			       apiToCall);
	}
	else {
		AFB_ReqFailF(request, "invalid_response", "Action %s correctly transferred to %s, but response is not valid",
							verbToCall,
							apiToCall);
	}
}

void HalCtlsListVerbs(AFB_ReqT request)
{
	unsigned int idx;

	AFB_ApiT apiHandle;
	CtlConfigT *ctrlConfig;

	struct SpecificHalData *currentCtlHalData;

	json_object *requestJson, *requestAnswer, *streamsArray, *currentStream;

	apiHandle = (AFB_ApiT) afb_request_get_dynapi(request);
	if(! apiHandle) {
		AFB_ReqFail(request, "api_handle", "Can't get current hal controller api handle");
		return;
	}

	ctrlConfig = (CtlConfigT *) afb_dynapi_get_userdata(apiHandle);
	if(! ctrlConfig) {
		AFB_ReqFail(request, "hal_controller_config", "Can't get current hal controller config");
		return;
	}

	currentCtlHalData = (struct SpecificHalData *) ctrlConfig->external;
	if(! currentCtlHalData) {
		AFB_ReqFail(request, "hal_controller_data", "Can't get current hal controller data");
		return;
	}

	if(! currentCtlHalData->ctlHalSpecificData->ctlHalStreamsData.count) {
		AFB_ReqSucess(request, NULL, "No data to answer, no streams found");
		return;
	}

	requestJson = AFB_ReqJson(request);
	if(! requestJson) {
		AFB_ReqFail(request, "request_json", "Can't get request json");
		return;
	}

	streamsArray = json_object_new_array();
	if(! streamsArray) {
		AFB_ReqFail(request, "json_answer", "Can't generate par of json answer");
		return;
	}

	for(idx = 0; idx < currentCtlHalData->ctlHalSpecificData->ctlHalStreamsData.count; idx++) {
		wrap_json_pack(&currentStream,
			       "{s:s s:s}",
			       "name", currentCtlHalData->ctlHalSpecificData->ctlHalStreamsData.data[idx].name,
			       "cardId", currentCtlHalData->ctlHalSpecificData->ctlHalStreamsData.data[idx].streamCardId);
		json_object_array_add(streamsArray, currentStream);
	}

	// TODO JAI : Add playback and capture with their card id to the response

	wrap_json_pack(&requestAnswer, "{s:o}", "streams", streamsArray);

	AFB_ReqSucess(request, requestAnswer, "Requested data");
}

void HalCtlsInitMixer(AFB_ReqT request)
{
	unsigned int err;

	char *apiToCall;

	AFB_ApiT apiHandle;
	CtlConfigT *ctrlConfig;

	struct SpecificHalData *currentCtlHalData;

	json_object *returnJ, *toReturnJ;

	apiHandle = (AFB_ApiT ) afb_request_get_dynapi(request);
	if(! apiHandle) {
		AFB_ReqFail(request, "api_handle", "Can't get current hal api handle");
		return;
	}

	ctrlConfig = (CtlConfigT *) afb_dynapi_get_userdata(apiHandle);
	if(! ctrlConfig) {
		AFB_ReqFail(request, "hal_controller_config", "Can't get current hal controller config");
		return;
	}

	currentCtlHalData = (struct SpecificHalData *) ctrlConfig->external;
	if(! currentCtlHalData) {
		AFB_ReqFail(request, "hal_controller_data", "Can't get current hal controller data");
		return;
	}

	apiToCall = currentCtlHalData->ctlHalSpecificData->mixerApiName;
	if(! apiToCall) {
		AFB_ReqFail(request, "mixer_api", "Can't get mixer api");
		return;
	}

	switch(currentCtlHalData->status) {
		case HAL_STATUS_UNAVAILABLE:
			AFB_ReqFail(request, "hal_unavailable", "Seems that the hal corresponding card was not found by alsacore at startup");
			return;

		case HAL_STATUS_READY:
			AFB_ReqSucess(request, NULL, "Seems that the hal mixer is already initialized");
			return;

		case HAL_STATUS_AVAILABLE:
			break;
	}

	if(AFB_ServiceSync(apiHandle, apiToCall, "create", json_object_get(currentCtlHalData->ctlHalSpecificData->halMixerJ), &returnJ)) {
		HalUtlHandleAppFwCallErrorInRequest(request, apiToCall, "create", returnJ, "mixer_create");
	}
	else if(json_object_object_get_ex(returnJ, "response", &toReturnJ)) {
		err = HalCtlsHandleMixerAttachResponse(request, &currentCtlHalData->ctlHalSpecificData->ctlHalStreamsData, toReturnJ);
		if(err != (int) MIXER_NO_ERROR) {
			AFB_ReqFailF(request,
					   "handler_mixer_response",
					   "Seems that create call to api %s succeed but this warning was risen by response decoder : %i",
					   apiToCall,
					   err);
			return;
		}

		AFB_ReqSucessF(request,
			       toReturnJ,
			       "Seems that create call to api %s succeed with no warning raised",
			       apiToCall);

		currentCtlHalData->status = HAL_STATUS_READY;
	}
	else {
		AFB_ReqFailF(request,
				   "invalid_response",
				   "Seems that mix_new call to api %s succeed, but response is not valid",
				   apiToCall);
	}
}