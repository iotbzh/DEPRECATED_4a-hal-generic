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

#include "../4a-hal-utilities/4a-hal-utilities-appfw-responses-handler.h"
#include "../4a-hal-utilities/4a-hal-utilities-data.h"
#include "../4a-hal-utilities/4a-hal-utilities-verbs-loader.h"

#include "../4a-hal-manager/4a-hal-manager.h"

#include "4a-hal-controllers-mixer-handler.h"
#include "4a-hal-controllers-cb.h"

/*******************************************************************************
 *		HAL controllers handle mixer calls functions		       *
 ******************************************************************************/

int HalCtlsHandleMixerAttachResponse(AFB_ApiT apiHandle, struct CtlHalStreamsDataT *currentHalStreamsData, json_object *mixerResponseJ)
{
	int err = (int) MIXER_NO_ERROR;
	unsigned int idx;

	char *currentStreamName, *currentStreamCardId;

	struct HalUtlApiVerb *CtlHalDynApiStreamVerbs;

	json_object *currentStreamJ;

	if(! apiHandle) {
		AFB_ApiError(apiHandle, "%s: Can't get current hal api handle", __func__);
		return (int) MIXER_ERROR_API_UNAVAILABLE;
	}

	switch(json_object_get_type(mixerResponseJ)) {
		case json_type_object:
			currentHalStreamsData->count = 1;
			break;
		case json_type_array:
			currentHalStreamsData->count = json_object_array_length(mixerResponseJ);
			break;
		default:
			currentHalStreamsData->count = 0;
			AFB_ApiWarning(apiHandle, "%s: no streams returned", __func__);
			return (int) MIXER_ERROR_NO_STREAMS;
	}

	currentHalStreamsData->data = (struct CtlHalStreamData *) calloc(currentHalStreamsData->count, sizeof(struct CtlHalStreamData));

	CtlHalDynApiStreamVerbs = alloca((currentHalStreamsData->count + 1) * sizeof(struct HalUtlApiVerb));
	memset(CtlHalDynApiStreamVerbs, '\0', (currentHalStreamsData->count + 1) * sizeof(struct HalUtlApiVerb));
	CtlHalDynApiStreamVerbs[currentHalStreamsData->count].verb = NULL;

	for(idx = 0; idx < currentHalStreamsData->count; idx++) {
		if(currentHalStreamsData->count > 1)
			currentStreamJ = json_object_array_get_idx(mixerResponseJ, (int) idx);
		else
			currentStreamJ = mixerResponseJ;

		if(wrap_json_unpack(currentStreamJ, "{s:s}", "uid", &currentStreamName)) {
			AFB_ApiError(apiHandle, "%s: can't find name in current stream object", __func__);
			err += (int) MIXER_ERROR_STREAM_NAME_UNAVAILABLE;
		}
		else if(wrap_json_unpack(currentStreamJ, "{s:s}", "alsa", &currentStreamCardId)) {
			AFB_ApiError(apiHandle, "%s: can't find card id in current stream object", __func__);
			err += (int) MIXER_ERROR_STREAM_CARDID_UNAVAILABLE;
		}
		else {
			currentHalStreamsData->data[idx].name = strdup(currentStreamName);
			currentHalStreamsData->data[idx].streamCardId = strdup(currentStreamCardId);

			CtlHalDynApiStreamVerbs[idx].verb = currentStreamName;
			CtlHalDynApiStreamVerbs[idx].callback = HalCtlsActionOnStream;
			CtlHalDynApiStreamVerbs[idx].info = "Peform action on this stream";
		}
	}

	if(HalUtlLoadVerbs(apiHandle, CtlHalDynApiStreamVerbs)) {
		AFB_ApiError(apiHandle, "%s: error while creating verbs for streams", __func__);
		err += (int) MIXER_ERROR_COULDNT_ADD_STREAMS_AS_VERB;
	}

	return err;
}

int HalCtlsAttachToMixer(AFB_ApiT apiHandle)
{
	unsigned int err;

	char *apiToCall, *returnedStatus = NULL, *returnedInfo = NULL;

	enum CallError returnedError;

	CtlConfigT *ctrlConfig;

	struct SpecificHalData *currentCtlHalData, *concurentHalData = NULL;
	struct SpecificHalData **firstHalData;

	json_object *returnJ, *toReturnJ;

	ctrlConfig = (CtlConfigT *) afb_dynapi_get_userdata(apiHandle);
	if(! ctrlConfig) {
		AFB_ApiError(apiHandle, "%s: Can't get current hal controller config", __func__);
		return -1;
	}

	currentCtlHalData = (struct SpecificHalData *) ctrlConfig->external;
	if(! currentCtlHalData) {
		AFB_ApiError(apiHandle, "%s: Can't get current hal controller data", __func__);
		return -2;
	}

	// TODO JAI : add a test to see if the hal card id is already used
	firstHalData = HalMngGetFirstHalData();
	if((concurentHalData = HalUtlSearchReadyHalDataByCarId(firstHalData, currentCtlHalData->sndCardId))) {
		AFB_ApiError(apiHandle,
			     "%s: trying to attach mixer for hal '%s' but the alsa device %i is already in use with mixer by hal '%s'",
			     __func__,
			     currentCtlHalData->apiName,
			     currentCtlHalData->sndCardId,
			     concurentHalData->apiName);
		return -3;
	}

	apiToCall = currentCtlHalData->ctlHalSpecificData->mixerApiName;
	if(! apiToCall) {
		AFB_ApiError(apiHandle, "%s: Can't get mixer api", __func__);
		return -4;
	}

	switch(currentCtlHalData->status) {
		case HAL_STATUS_UNAVAILABLE:
			AFB_ApiError(apiHandle, "%s: Seems that the hal corresponding card was not found by alsacore at startup", __func__);
			return -5;

		case HAL_STATUS_READY:
			AFB_ApiNotice(apiHandle, "%s: Seems that the hal mixer is already initialized", __func__);
			return 1;

		case HAL_STATUS_AVAILABLE:
			break;
	}

	if(AFB_ServiceSync(apiHandle, apiToCall, MIXER_ATTACH_VERB, json_object_get(currentCtlHalData->ctlHalSpecificData->halMixerJ), &returnJ)) {
		returnedError = HalUtlHandleAppFwCallError(apiHandle, apiToCall, MIXER_ATTACH_VERB, returnJ, &returnedStatus, &returnedInfo);
		AFB_ApiWarning(apiHandle,
			       "Error %i during call to verb %s of %s api with status '%s' and info '%s'",
			       (int) returnedError,
			       apiToCall,
			       MIXER_ATTACH_VERB,
			       returnedStatus ? returnedStatus : "not returned",
			       returnedInfo ? returnedInfo : "not returned");
		return -6;
	}
	else if(json_object_object_get_ex(returnJ, "response", &toReturnJ)) {
		err = HalCtlsHandleMixerAttachResponse(apiHandle, &currentCtlHalData->ctlHalSpecificData->ctlHalStreamsData, toReturnJ);
		if(err != (int) MIXER_NO_ERROR) {
			AFB_ApiError(apiHandle,
				     "%s: Seems that %s call to api %s succeed but this warning was risen by response decoder : %i '%s'",
				     __func__,
				     MIXER_ATTACH_VERB,
				     apiToCall,
				     err,
				     json_object_get_string(toReturnJ));
			return -7;
		}

		AFB_ApiNotice(apiHandle,
			      "%s: Seems that %s call to api %s succeed with no warning raised : '%s'",
			      __func__,
			      MIXER_ATTACH_VERB,
			      apiToCall,
			      json_object_get_string(toReturnJ));

		currentCtlHalData->status = HAL_STATUS_READY;
	}
	else {
		AFB_ApiError(apiHandle,
			     "%s: Seems that %s call to api %s succeed, but response is not valid : '%s'",
			     __func__,
			     MIXER_ATTACH_VERB,
			     apiToCall,
			     json_object_get_string(returnJ));
		return -8;
	}

	return 0;
}