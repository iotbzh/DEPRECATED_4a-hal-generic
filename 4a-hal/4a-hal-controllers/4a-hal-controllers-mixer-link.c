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

#include "../4a-hal-utilities/4a-hal-utilities-appfw-responses-handler.h"
#include "../4a-hal-utilities/4a-hal-utilities-data.h"
#include "../4a-hal-utilities/4a-hal-utilities-verbs-loader.h"

#include "../4a-hal-manager/4a-hal-manager.h"

#include "4a-hal-controllers-mixer-link.h"
#include "4a-hal-controllers-cb.h"

/*******************************************************************************
 *		HAL controllers handle mixer calls functions		       *
 ******************************************************************************/

int HalCtlsHandleMixerData(AFB_ApiT apiHandle, struct CtlHalMixerDataT *currentMixerDataT, json_object *currentDataJ, enum MixerDataType dataType)
{
	int idx, verbStart, size;
	int err = (int) MIXER_NO_ERROR;

	char *currentDataVerbName, *currentStreamCardId;

	json_type currentDataType;
	json_object *currentJ;

	currentDataType = json_object_get_type(currentDataJ);
	switch(currentDataType) {
		case json_type_object:
			currentMixerDataT->count = 1;
			break;
		case json_type_array:
			currentMixerDataT->count = (unsigned int) json_object_array_length(currentDataJ);
			break;
		default:
			currentMixerDataT->count = 0;
			AFB_ApiError(apiHandle, "%s: no data returned", __func__);
			return (int) MIXER_ERROR_DATA_EMPTY;
	}

	currentMixerDataT->data = (struct CtlHalMixerData *) calloc(currentMixerDataT->count, sizeof(struct CtlHalMixerData));

	for(idx = 0; idx < currentMixerDataT->count; idx++) {
		if(currentDataType == json_type_array)
			currentJ = json_object_array_get_idx(currentDataJ, (int) idx);
		else
			currentJ = currentDataJ;

		if(wrap_json_unpack(currentJ, "{s:s}", "verb", &currentDataVerbName)) {
			AFB_ApiError(apiHandle, "%s: can't find verb in current data object", __func__);
			err += (int) MIXER_ERROR_DATA_NAME_UNAVAILABLE;
		}
		else if(dataType == MIXER_DATA_STREAMS && wrap_json_unpack(currentJ, "{s:s}", "alsa", &currentStreamCardId)) {
			AFB_ApiError(apiHandle, "%s: can't find card id in current data object", __func__);
			err += (int) MIXER_ERROR_DATA_CARDID_UNAVAILABLE;
		}
		else {
			currentMixerDataT->data[idx].verbToCall = strdup(currentDataVerbName);

			if(dataType == MIXER_DATA_STREAMS) {
				currentMixerDataT->data[idx].streamCardId = strdup(currentStreamCardId);

				size = (int) strlen(currentDataVerbName);
				for(verbStart = 0; verbStart < size; verbStart++) {
					if(currentDataVerbName[verbStart] == '#')
						break;
				}

				if(verbStart == size)
					verbStart = 0;
				else
					verbStart++;

				currentMixerDataT->data[idx].verb = strdup(&currentDataVerbName[verbStart]);

				if(afb_dynapi_add_verb(apiHandle,
						       currentMixerDataT->data[idx].verb,
						       "Stream action transferred to mixer",
						       HalCtlsActionOnCall,
						       (void *) &currentMixerDataT->data[idx],
						       NULL,
						       0)) {
					AFB_ApiError(apiHandle,
						     "%s: error while creating verbs for stream : '%s'",
						     __func__,
						     currentMixerDataT->data[idx].verb);
					err += (int) MIXER_ERROR_STREAM_VERB_NOT_CREATED;
				}
			}
			else if(dataType == MIXER_DATA_PLAYBACKS) {
				currentMixerDataT->data[idx].verb = strdup(HAL_PLAYBACK_VERB);
			}
			else if(dataType == MIXER_DATA_CAPTURES) {
				currentMixerDataT->data[idx].verb = strdup(HAL_CAPTURE_VERB);
			}
		}
	}

	if(dataType == MIXER_DATA_PLAYBACKS) {
		if(afb_dynapi_add_verb(apiHandle,
				       HAL_PLAYBACK_VERB,
				       "Playback action transferred to mixer",
				       HalCtlsActionOnCall,
				       (void *) currentMixerDataT->data,
				       NULL,
				       0)) {
			AFB_ApiError(apiHandle, "%s: error while creating verb for playbacks : '%s'", __func__, HAL_PLAYBACK_VERB);
			err += (int) MIXER_ERROR_PLAYBACK_VERB_NOT_CREATED;
		}
	}

	if(dataType == MIXER_DATA_CAPTURES) {
		if(afb_dynapi_add_verb(apiHandle,
				       HAL_CAPTURE_VERB,
				       "Capture action transferred to mixer",
				       HalCtlsActionOnCall,
				       (void *) currentMixerDataT->data,
				       NULL,
				       0)) {
			AFB_ApiError(apiHandle, "%s: error while creating verb for captures : '%s'", __func__, HAL_CAPTURE_VERB);
			err += (int) MIXER_ERROR_CAPTURE_VERB_NOT_CREATED;
		}
	}

	return err;
}

int HalCtlsHandleMixerAttachResponse(AFB_ApiT apiHandle, struct CtlHalSpecificData *currentHalSpecificData, json_object *mixerResponseJ)
{
	int err = (int) MIXER_NO_ERROR;

	json_object *mixerStreamsJ = NULL, *mixerPlaybacksJ = NULL, *mixerCapturesJ = NULL;

	if(! apiHandle) {
		AFB_ApiError(apiHandle, "%s: Can't get current hal api handle", __func__);
		return (int) MIXER_ERROR_API_UNAVAILABLE;
	}

	if(wrap_json_unpack(mixerResponseJ, "{s?:o s?:o s?:o}", "streams", &mixerStreamsJ, "playbacks", &mixerPlaybacksJ, "captures", &mixerCapturesJ)) {
		AFB_ApiError(apiHandle, "%s: Can't get streams|playbacks|captures object in '%s'", __func__, json_object_get_string(mixerResponseJ));
		return (int) MIXER_ERROR_DATA_UNAVAILABLE;
	}

	if(mixerStreamsJ && (err += HalCtlsHandleMixerData(apiHandle, &currentHalSpecificData->ctlHalStreamsData, mixerStreamsJ, MIXER_DATA_STREAMS)))
		AFB_ApiError(apiHandle, "%s: Error during handling response mixer streams data '%s'", __func__, json_object_get_string(mixerStreamsJ));

	if(mixerPlaybacksJ && (err += HalCtlsHandleMixerData(apiHandle, &currentHalSpecificData->ctlHalPlaybacksData, mixerPlaybacksJ, MIXER_DATA_PLAYBACKS)))
		AFB_ApiError(apiHandle, "%s: Error during handling response mixer playbacks data '%s'", __func__, json_object_get_string(mixerPlaybacksJ));

	if(mixerCapturesJ && (err += HalCtlsHandleMixerData(apiHandle, &currentHalSpecificData->ctlHalCapturesData, mixerCapturesJ, MIXER_DATA_CAPTURES)))
		AFB_ApiError(apiHandle, "%s: Error during handling response mixer captures data '%s'", __func__, json_object_get_string(mixerCapturesJ));

	return err;
}

int HalCtlsAttachToMixer(AFB_ApiT apiHandle)
{
	int err = 0, mixerError;

	char *apiToCall, *returnedStatus = NULL, *returnedInfo = NULL;

	enum CallError returnedError;

	CtlConfigT *ctrlConfig;

	struct SpecificHalData *currentCtlHalData, *concurentHalData = NULL;
	struct SpecificHalData **firstHalData;

	json_object *returnJ = NULL, *toReturnJ;

	if(! apiHandle) {
		AFB_ApiError(apiHandle, "%s: Can't get current hal api handle", __func__);
		return -1;
	}

	ctrlConfig = (CtlConfigT *) afb_dynapi_get_userdata(apiHandle);
	if(! ctrlConfig) {
		AFB_ApiError(apiHandle, "%s: Can't get current hal controller config", __func__);
		return -2;
	}

	currentCtlHalData = (struct SpecificHalData *) ctrlConfig->external;
	if(! currentCtlHalData) {
		AFB_ApiError(apiHandle, "%s: Can't get current hal controller data", __func__);
		return -3;
	}

	switch(currentCtlHalData->status) {
		case HAL_STATUS_UNAVAILABLE:
			AFB_ApiError(apiHandle, "%s: Seems that the hal corresponding card was not found by alsacore at startup", __func__);
			return -4;

		case HAL_STATUS_READY:
			AFB_ApiNotice(apiHandle, "%s: Seems that the hal mixer is already initialized", __func__);
			return 1;

		case HAL_STATUS_AVAILABLE:
			break;
	}

	firstHalData = HalMngGetFirstHalData();
	if((concurentHalData = HalUtlSearchReadyHalDataByCarId(firstHalData, currentCtlHalData->sndCardId))) {
		AFB_ApiError(apiHandle,
			     "%s: trying to attach mixer for hal '%s' but the alsa device %i is already in use with mixer by hal '%s'",
			     __func__,
			     currentCtlHalData->apiName,
			     currentCtlHalData->sndCardId,
			     concurentHalData->apiName);
		return -5;
	}

	apiToCall = currentCtlHalData->ctlHalSpecificData->mixerApiName;
	if(! apiToCall) {
		AFB_ApiError(apiHandle, "%s: Can't get mixer api", __func__);
		return -6;
	}

	if(AFB_ServiceSync(apiHandle, apiToCall, MIXER_ATTACH_VERB, json_object_get(currentCtlHalData->ctlHalSpecificData->halMixerJ), &returnJ)) {
		returnedError = HalUtlHandleAppFwCallError(apiHandle, apiToCall, MIXER_ATTACH_VERB, returnJ, &returnedStatus, &returnedInfo);
		AFB_ApiError(apiHandle,
			    "Error %i during call to verb %s of %s api with status '%s' and info '%s'",
			    (int) returnedError,
			    apiToCall,
			    MIXER_ATTACH_VERB,
			    returnedStatus ? returnedStatus : "not returned",
			    returnedInfo ? returnedInfo : "not returned");
		err = -7;
	}
	else if(! json_object_object_get_ex(returnJ, "response", &toReturnJ)) {
		AFB_ApiError(apiHandle,
			     "%s: Seems that %s call to api %s succeed, but response is not valid : '%s'",
			     __func__,
			     MIXER_ATTACH_VERB,
			     apiToCall,
			     json_object_get_string(returnJ));
		err = -8;
	}
	else if((mixerError = HalCtlsHandleMixerAttachResponse(apiHandle, currentCtlHalData->ctlHalSpecificData, toReturnJ)) != (int) MIXER_NO_ERROR) {
		AFB_ApiError(apiHandle,
			     "%s: Seems that %s call to api %s succeed but this warning was risen by response decoder : %i '%s'",
			     __func__,
			     MIXER_ATTACH_VERB,
			     apiToCall,
			     mixerError,
			     json_object_get_string(toReturnJ));
		err = -9;
	}
	else {
		AFB_ApiNotice(apiHandle,
			      "%s: Seems that %s call to api %s succeed with no warning raised : '%s'",
			      __func__,
			      MIXER_ATTACH_VERB,
			      apiToCall,
			      json_object_get_string(toReturnJ));

		currentCtlHalData->status = HAL_STATUS_READY;
	}

	if(returnJ)
		json_object_put(returnJ);

	return err;
}

int HalCtlsGetInfoFromMixer(AFB_ApiT apiHandle, char *apiToCall, json_object *requestJson, json_object **toReturnJ, char **returnedStatus, char **returnedInfo)
{
	int err = 0;

	enum CallError returnedError;

	json_object *returnJ, *responseJ;

	if(! apiHandle) {
		AFB_ApiError(apiHandle, "%s: Can't get current hal api handle", __func__);
		return -1;
	}

	if(! apiToCall) {
		AFB_ApiError(apiHandle, "Can't get mixer api");
		return -2;
	}

	if(! requestJson) {
		AFB_ApiError(apiHandle, "Can't get request json");
		return -3;
	}

	if(AFB_ServiceSync(apiHandle, apiToCall, MIXER_INFO_VERB, json_object_get(requestJson), &returnJ)) {
		returnedError = HalUtlHandleAppFwCallError(apiHandle, apiToCall, MIXER_INFO_VERB, returnJ, returnedStatus, returnedInfo);
		AFB_ApiError(apiHandle,
			     "Error %i during call to verb %s of %s api with status '%s' and info '%s'",
			     (int) returnedError,
			     apiToCall,
			     MIXER_INFO_VERB,
			     *returnedStatus ? *returnedStatus : "not returned",
			     *returnedInfo ? *returnedInfo : "not returned");
		err = -4;
	}
	else if(! json_object_object_get_ex(returnJ, "response", &responseJ)) {
		AFB_ApiError(apiHandle,
			     "%s: Seems that %s call to api %s succeed, but response is not valid : '%s'",
			     __func__,
			     MIXER_INFO_VERB,
			     apiToCall,
			     json_object_get_string(returnJ));
		err = -5;
	}
	else {
		AFB_ApiNotice(apiHandle,
			      "%s: Seems that %s call to api %s succeed with no warning raised : '%s'",
			      __func__,
			      MIXER_INFO_VERB,
			      apiToCall,
			      json_object_get_string(responseJ));

		*toReturnJ = json_object_get(responseJ);
	}

	json_object_put(returnJ);
	return err;
}