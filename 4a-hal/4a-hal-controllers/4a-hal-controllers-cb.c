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
#include "../4a-hal-utilities/4a-hal-utilities-verbs-loader.h"

#include "4a-hal-controllers-cb.h"
#include "4a-hal-controllers-mixer-handler.h"

/*******************************************************************************
 *		HAL controllers sections parsing functions		       *
 ******************************************************************************/

int HalCtlsHalMixerConfig(afb_dynapi *apiHandle, CtlSectionT *section, json_object *MixerJ)
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
int HalCtlsHalMapConfig(afb_dynapi *apiHandle, CtlSectionT *section, json_object *StreamControlsJ)
{
	AFB_DYNAPI_WARNING(apiHandle, "JAI :%s not implemented yet", __func__);

	return 0;
}

/*******************************************************************************
 *		HAL controllers verbs functions				       *
 ******************************************************************************/

void HalCtlsActionOnStream(afb_request *request)
{
	int verbToCallSize;

	char *apiToCall, *mixerVerbName, *verbToCall;

	afb_dynapi *apiHandle;
	CtlConfigT *ctrlConfig;

	struct SpecificHalData *currentCtlHalData;

	json_object *requestJson, *returnJ, *toReturnJ;

	apiHandle = (afb_dynapi *) afb_request_get_dynapi(request);
	if(! apiHandle) {
		afb_request_fail(request, "api_handle", "Can't get current hal controller api handle");
		return;
	}

	ctrlConfig = (CtlConfigT *) afb_dynapi_get_userdata(apiHandle);
	if(! ctrlConfig) {
		afb_request_fail(request, "hal_controller_config", "Can't get current hal controller config");
		return;
	}

	currentCtlHalData = ctrlConfig->external;
	if(! currentCtlHalData) {
		afb_request_fail(request, "hal_controller_data", "Can't get current hal controller data");
		return;
	}

	requestJson = afb_request_json(request);
	if(! requestJson) {
		afb_request_fail(request, "request_json", "Can't get request json");
		return;
	}

	apiToCall = currentCtlHalData->ctlHalSpecificData->mixerApiName;
	if(! apiToCall) {
		afb_request_fail(request, "mixer_api", "Can't get mixer api");
		return;
	}

	mixerVerbName = currentCtlHalData->ctlHalSpecificData->mixerVerbName;
	if(! mixerVerbName) {
		afb_request_fail(request, "hal_softmixer_verb", "Can't get hal mixer verb prefix");
		return;
	}

	// TODO JAI: check status of hal before doing anything

	// TODO JAI : remove verb to call prefix, each hal should have its own api in softmixer, and each streams should be created as verb by mixer
	verbToCallSize = (int) strlen(mixerVerbName) + (int) strlen(request->verb) + 2;
	verbToCall = (char *) alloca(verbToCallSize * sizeof(char));
	verbToCall[0] = '\0';
	verbToCall[verbToCallSize - 1] = '\0';

	strcat(verbToCall, mixerVerbName);
	strcat(verbToCall, "/");
	strcat(verbToCall, request->verb);

	if(afb_dynapi_call_sync(apiHandle, apiToCall, verbToCall, json_object_get(requestJson), &returnJ)) {
		HalCtlsHandleMixerCallError(request, apiToCall, verbToCall, returnJ, "stream_action");
	}
	else if(json_object_object_get_ex(returnJ, "response", &toReturnJ)){
		afb_request_success_f(request, toReturnJ, "Action %s correctly transfered to %s without any error raised",
								verbToCall,
								apiToCall);
	}
	else {
		afb_request_fail_f(request, "invalid_response", "Action %s correctly transfered to %s, but response is not valid",
							verbToCall,
							apiToCall);
	}
}

void HalCtlsListVerbs(afb_request *request)
{
	unsigned int idx;

	afb_dynapi *apiHandle;
	CtlConfigT *ctrlConfig;

	struct SpecificHalData *currentCtlHalData;

	json_object *requestJson, *requestAnswer, *streamsArray, *currentStream;

	apiHandle = (afb_dynapi *) afb_request_get_dynapi(request);
	if(! apiHandle) {
		afb_request_fail(request, "api_handle", "Can't get current hal controller api handle");
		return;
	}

	ctrlConfig = (CtlConfigT *) afb_dynapi_get_userdata(apiHandle);
	if(! ctrlConfig) {
		afb_request_fail(request, "hal_controller_config", "Can't get current hal controller config");
		return;
	}

	currentCtlHalData = ctrlConfig->external;
	if(! currentCtlHalData) {
		afb_request_fail(request, "hal_controller_data", "Can't get current hal controller data");
		return;
	}

	if(! currentCtlHalData->ctlHalSpecificData->ctlHalStreamsData.count) {
		afb_request_success(request, NULL, "No data to answer, no streams found");
		return;
	}

	requestJson = afb_request_json(request);
	if(! requestJson) {
		afb_request_fail(request, "request_json", "Can't get request json");
		return;
	}

	streamsArray = json_object_new_array();
	if(! streamsArray) {
		afb_request_fail(request, "json_answer", "Can't generate par of json answer");
		return;
	}

	for(idx = 0; idx < currentCtlHalData->ctlHalSpecificData->ctlHalStreamsData.count; idx++) {
		wrap_json_pack(&currentStream,
			       "{s:s s:s}",
			       "name", currentCtlHalData->ctlHalSpecificData->ctlHalStreamsData.data[idx].name,
			       "cardId", currentCtlHalData->ctlHalSpecificData->ctlHalStreamsData.data[idx].streamCardId);
		json_object_array_add(streamsArray, currentStream);
	}

	// TODO JAI : Check if there is some halmap config controls and add them to the reponse

	wrap_json_pack(&requestAnswer, "{s:o}", "streams", streamsArray);

	afb_request_success(request, requestAnswer, "Requested data");
}

void HalCtlsInitMixer(afb_request *request)
{
	unsigned int err;

	char *apiToCall;

	afb_dynapi *apiHandle;
	CtlConfigT *ctrlConfig;

	struct SpecificHalData *currentCtlHalData;

	json_object *returnJ, *toReturnJ;

	apiHandle = (afb_dynapi *) afb_request_get_dynapi(request);
	if(! apiHandle) {
		afb_request_fail(request, "api_handle", "Can't get hal manager api handle");
		return;
	}

	ctrlConfig = (CtlConfigT *) afb_dynapi_get_userdata(apiHandle);
	if(! ctrlConfig) {
		afb_request_fail(request, "hal_controller_config", "Can't get current hal controller config");
		return;
	}

	currentCtlHalData = ctrlConfig->external;
	if(! currentCtlHalData) {
		afb_request_fail(request, "hal_controller_data", "Can't get current hal controller data");
		return;
	}

	apiToCall = currentCtlHalData->ctlHalSpecificData->mixerApiName;
	if(! apiToCall) {
		afb_request_fail(request, "mixer_api", "Can't get mixer api");
		return;
	}

	// TODO JAI: test hal status (card is detected)

	if(afb_dynapi_call_sync(apiHandle, apiToCall, "create", json_object_get(currentCtlHalData->ctlHalSpecificData->halMixerJ), &returnJ)) {
		HalCtlsHandleMixerCallError(request, apiToCall, "create", returnJ, "mixer_create");
	}
	else if(json_object_object_get_ex(returnJ, "response", &toReturnJ)) {
		err = HalCtlsHandleMixerAttachResponse(request, &currentCtlHalData->ctlHalSpecificData->ctlHalStreamsData, toReturnJ);
		if(err) {
			afb_request_success_f(request,
					      toReturnJ,
					      "Seems that create call to api %s succeed but this warning was rised by response decoder : %i",
					      apiToCall,
					      err);
			return;
		}

		afb_request_success_f(request,
				      toReturnJ,
				      "Seems that create call to api %s succeed with no warning raised",
				      apiToCall);
	}
	else {
		afb_request_fail_f(request,
				   "invalid_response",
				   "Seems that mix_new call to api %s succeed, but response is not valid",
				   apiToCall);
	}
}