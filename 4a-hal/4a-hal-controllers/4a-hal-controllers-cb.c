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
#include "4a-hal-controllers-alsacore-link.h"
#include "4a-hal-controllers-mixer-handler.h"
#include "4a-hal-controllers-value-handler.h"

/*******************************************************************************
 *		HAL controller event handler function			       *
 ******************************************************************************/

void HalCtlsDispatchApiEvent(afb_dynapi *apiHandle, const char *evtLabel, json_object *eventJ)
{
	int numid, idx;

	char cardIdString[6];

	char *alsaCoreLabel;

	CtlConfigT *ctrlConfig;
	CtlSourceT source;

	struct SpecificHalData *currentHalData;
	struct CtlHalAlsaMapT *currentHalAlsaCtlsT;

	json_object *valuesJ;

	AFB_ApiDebug(apiHandle, "%s: evtname=%s [msg=%s]", __func__, evtLabel, json_object_get_string(eventJ));

	ctrlConfig = (CtlConfigT *) afb_dynapi_get_userdata(apiHandle);
	if(! ctrlConfig) {
		AFB_ApiError(apiHandle, "%s: Can't get current hal controller config", __func__);
		return;
	}

	currentHalData = (struct SpecificHalData *) ctrlConfig->external;
	if(! currentHalData) {
		AFB_ApiWarning(apiHandle, "%s: Can't get current hal controller data", __func__);
		return;
	}

	snprintf(cardIdString, 6, "hw:%i", currentHalData->sndCardId);
	currentHalAlsaCtlsT = currentHalData->ctlHalSpecificData->ctlHalAlsaMapT;

	asprintf(&alsaCoreLabel, "%s/%s", ALSACORE_API, cardIdString);

	if(strcmp(evtLabel, alsaCoreLabel) == 0 &&
	   ! wrap_json_unpack(eventJ, "{s:i s:o !}", "id", &numid, "val", &valuesJ)) {
		free(alsaCoreLabel);

		// Search for corresponding numid in halCtls, if found, launch callback (if available)
		for(idx = 0; idx < currentHalAlsaCtlsT->ctlsCount; idx++) {
			if(currentHalAlsaCtlsT->ctls[idx].ctl.numid == numid) {
				if(currentHalAlsaCtlsT->ctls[idx].action) {
					source.uid = currentHalAlsaCtlsT->ctls[idx].action->uid;
					source.api = currentHalAlsaCtlsT->ctls[idx].action->api;
					source.request = NULL;

					(void) ActionExecOne(&source, currentHalAlsaCtlsT->ctls[idx].action, valuesJ);
				}
				else {
					AFB_ApiNotice(apiHandle,
						      "%s: The alsa control id '%i' is corresponding to a known control but without any action registered",
						      __func__,
						      numid);
				}

				return;
			}
		}

		AFB_ApiWarning(apiHandle, "%s: alsacore event with an unrecognized numid: %i, evtname=%s [msg=%s]",
					  __func__,
					  numid,
					  evtLabel,
					  json_object_get_string(eventJ));

		return;
	}

	free(alsaCoreLabel);

	AFB_ApiNotice(apiHandle,
		      "%s: not an alsacore event '%s' [msg=%s]",
		      __func__,
		      evtLabel,
		      json_object_get_string(eventJ));

	CtrlDispatchApiEvent(apiHandle, evtLabel, eventJ);
}

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

int HalCtlsProcessOneHalMapObject(AFB_ApiT apiHandle, struct CtlHalAlsaMap *alsaMap, json_object *AlsaMapJ)
{
	char *action = NULL, *typename = NULL;

	json_object *alsaJ = NULL, *createAlsaCtlJ = NULL;

	AFB_ApiDebug(apiHandle, "%s: AlsaMapJ=%s", __func__, json_object_get_string(AlsaMapJ));

	if(wrap_json_unpack(AlsaMapJ, "{s:s s?:s s:o s?:s !}",
				      "uid", &alsaMap->uid,
				      "info", &alsaMap->info,
				      "alsa", &alsaJ,
				      "action", &action)) {
		AFB_ApiError(apiHandle, "%s: parsing error, map should only contains [label]|[uid]|[tag]|[info]|[alsa]|[action] in:\n-- %s", __func__, json_object_get_string(AlsaMapJ));
		return -1;
	}

	if(wrap_json_unpack(alsaJ, "{s?:s s?:i s?:i s?:o !}",
				   "name", &alsaMap->ctl.name,
				   "numid", &alsaMap->ctl.numid,
				   "value", &alsaMap->ctl.value,
				   "create", &createAlsaCtlJ)) {
		AFB_ApiError(apiHandle, "%s: parsing error, alsa json should only contains [name]|[numid]||[value]|[create] in:\n-- %s", __func__, json_object_get_string(alsaJ));
		return -2;
	}

	if(createAlsaCtlJ) {
		alsaMap->ctl.alsaCtlCreation = &alsaMap->ctl.alsaCtlProperties;

		if(wrap_json_unpack(createAlsaCtlJ,
				    "{s:s s:i s:i s:i s:i !}",
				    "type", &typename,
				    "count", &alsaMap->ctl.alsaCtlCreation->count,
				    "minval", &alsaMap->ctl.alsaCtlCreation->minval,
				    "maxval", &alsaMap->ctl.alsaCtlCreation->maxval,
				    "step", &alsaMap->ctl.alsaCtlCreation->step)) {
			AFB_ApiError(apiHandle, "%s: parsing error, alsa creation json should only contains [type]|[count]|[minval]|[maxval]|[step] in:\n-- %s", __func__, json_object_get_string(alsaJ));
			return -3;
		}

		if(typename && (alsaMap->ctl.alsaCtlCreation->type = HalCtlsMapsAlsaTypeToEnum(typename)) == SND_CTL_ELEM_TYPE_NONE) {
			AFB_ApiError(apiHandle, "%s: Couldn't get alsa type from string %s in:\n-- %s", __func__, typename, json_object_get_string(alsaJ));
			return -4;
		}

		if(! alsaMap->ctl.name)
			alsaMap->ctl.name = (char *) alsaMap->uid;
	}
	else if(alsaMap->ctl.name && alsaMap->ctl.numid > 0) {
		AFB_ApiError(apiHandle,
			     "%s: Can't have both a control name (%s) and a control uid (%i) in alsa object:\n-- %s",
			     __func__,
			     alsaMap->ctl.name,
			     alsaMap->ctl.numid,
			     json_object_get_string(alsaJ));
		return -5;
	}
	else if(! alsaMap->ctl.name && alsaMap->ctl.numid <= 0) {
		AFB_ApiError(apiHandle,
			     "%s: Need at least a control name or a control uid in alsa object:\n-- %s",
			     __func__,
			     json_object_get_string(alsaJ));
		return -6;
	}

	if(action)
		alsaMap->actionJ = AlsaMapJ;

	return 0;
}

int HalCtlsHandleOneHalMapObject(AFB_ApiT apiHandle, char *cardId, struct CtlHalAlsaMap *alsaMap)
{
	json_object *valueJ, *convertedValueJ;

	if(alsaMap->ctl.alsaCtlCreation) {
		if(HalCtlsCreateAlsaCtl(apiHandle, cardId, &alsaMap->ctl)) {
			AFB_ApiError(apiHandle,
				"%s: An error happened when trying to create a new alsa control",
				__func__);
			return -1;
		}
	}
	else if(HalCtlsGetAlsaCtlInfo(apiHandle, cardId, &alsaMap->ctl)) {
		AFB_ApiError(apiHandle,
			     "%s: An error happened when trying to get existing alsa control info",
			     __func__);
		return -2;
	}

	if(alsaMap->ctl.value) {
		// TBD JAI : handle alsa controls type
		valueJ = json_object_new_int(alsaMap->ctl.value);

		if(HalCtlsNormalizeJsonValues(apiHandle, &alsaMap->ctl.alsaCtlProperties, valueJ, &convertedValueJ)) {
			AFB_ApiError(apiHandle, "Error when trying to convert initiate value json '%s'", json_object_get_string(valueJ));
			return -3;
		}

		if(HalCtlsSetAlsaCtlValue(apiHandle, cardId, alsaMap->ctl.numid, convertedValueJ)) {
			AFB_ApiError(apiHandle,
				     "Error while trying to set initial value on alsa control %i, device '%s', value '%s'",
				     alsaMap->ctl.numid,
				     cardId,
				     json_object_get_string(valueJ));
			return -4;
		}
	}

	if(alsaMap->actionJ) {
		alsaMap->action = calloc(1, sizeof(CtlActionT));
		if(ActionLoadOne(apiHandle, alsaMap->action, alsaMap->actionJ, 0)) {
			AFB_ApiError(apiHandle,
				     "%s: Didn't succeed to load action using alsa object:\n-- %s",
				     __func__,
				     json_object_get_string(alsaMap->actionJ));
			return -5;
		}
	}

	if(afb_dynapi_add_verb(apiHandle, alsaMap->uid, alsaMap->info, HalCtlsActionOnAlsaCtl, (void *) alsaMap, NULL, 0)) {
		AFB_ApiError(apiHandle,
			     "%s: Didn't to create verb for current alsa control to load action using alsa object:\n-- %s",
			     __func__,
			     json_object_get_string(alsaMap->actionJ));
		return -6;
	}

	return 0;
}

int HalCtlsProcessAllHalMap(AFB_ApiT apiHandle, json_object *AlsaMapJ, struct CtlHalAlsaMapT *currentCtlHalAlsaMapT)
{
	int idx, err = 0;

	struct CtlHalAlsaMap *ctlMaps;

	switch(json_object_get_type(AlsaMapJ)) {
		case json_type_array:
			currentCtlHalAlsaMapT->ctlsCount = json_object_array_length(AlsaMapJ);
			break;

		case json_type_object:
			currentCtlHalAlsaMapT->ctlsCount = 1;
			break;

		default:
			currentCtlHalAlsaMapT->ctlsCount = 0;
			currentCtlHalAlsaMapT->ctls = NULL;
			AFB_ApiWarning(apiHandle, "%s: couldn't get content of 'halmap' section in:\n-- %s", __func__, json_object_get_string(AlsaMapJ));
			return -1;
	}

	ctlMaps = calloc(currentCtlHalAlsaMapT->ctlsCount, sizeof(struct CtlHalAlsaMap));

	for(idx = 0; idx < currentCtlHalAlsaMapT->ctlsCount; idx++)
		err += HalCtlsProcessOneHalMapObject(apiHandle, &ctlMaps[idx], currentCtlHalAlsaMapT->ctlsCount == 1 ? AlsaMapJ : json_object_array_get_idx(AlsaMapJ, idx));

	currentCtlHalAlsaMapT->ctls = ctlMaps;

	return err;
}

int HalCtlsHandleAllHalMap(AFB_ApiT apiHandle, int sndCardId, struct CtlHalAlsaMapT *currentCtlHalAlsaMapT)
{
	int idx, err = 0;

	char cardIdString[6];

	snprintf(cardIdString, 6, "hw:%i", sndCardId);

	HalCtlsSubscribeToAlsaCardEvent(apiHandle, cardIdString);

	for(idx = 0; idx < currentCtlHalAlsaMapT->ctlsCount; idx++)
		err += HalCtlsHandleOneHalMapObject(apiHandle, cardIdString, &currentCtlHalAlsaMapT->ctls[idx]);

	return err;
}

int HalCtlsHalMapConfig(AFB_ApiT apiHandle, CtlSectionT *section, json_object *AlsaMapJ)
{
	CtlConfigT *ctrlConfig;
	struct SpecificHalData *currentHalData;

	ctrlConfig = (CtlConfigT *) afb_dynapi_get_userdata(apiHandle);
	if(! ctrlConfig)
		return -1;

	currentHalData = (struct SpecificHalData *) ctrlConfig->external;
	if(! currentHalData)
		return -2;

	if(AlsaMapJ) {
		currentHalData->ctlHalSpecificData->ctlHalAlsaMapT = calloc(1, sizeof(struct CtlHalAlsaMapT));

		if(HalCtlsProcessAllHalMap(apiHandle, AlsaMapJ, currentHalData->ctlHalSpecificData->ctlHalAlsaMapT)) {
			AFB_ApiError(apiHandle, "%s: failed to process 'halmap' section", __func__);
			return -3;
		}
	}
	else if(currentHalData->status == HAL_STATUS_UNAVAILABLE) {
		AFB_ApiWarning(apiHandle, "%s: hal is unavailable, 'halmap' section data can't be handle", __func__);
		return 1;
	}
	else if(currentHalData->sndCardId < 0) {
		AFB_ApiError(apiHandle, "%s: hal alsa card id is not valid, 'halmap' section data can't be handle", __func__);
		return -6;
	}
	else if(! currentHalData->ctlHalSpecificData->ctlHalAlsaMapT) {
		AFB_ApiWarning(apiHandle, "%s: 'halmap' section data is empty", __func__);
		return 2;
	}
	else if(! (currentHalData->ctlHalSpecificData->ctlHalAlsaMapT->ctlsCount > 0)) {
		AFB_ApiWarning(apiHandle, "%s: no alsa controls defined in 'halmap' section", __func__);
		return 3;
	}
	else if(HalCtlsHandleAllHalMap(apiHandle, currentHalData->sndCardId, currentHalData->ctlHalSpecificData->ctlHalAlsaMapT)) {
		AFB_ApiError(apiHandle, "%s: failed to handle 'halmap' section", __func__);
		return -9;
	}

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
		AFB_ReqSuccessF(request,
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
		AFB_ReqSuccess(request, NULL, "No data to answer, no streams found");
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

	AFB_ReqSuccess(request, requestAnswer, "Requested data");
}