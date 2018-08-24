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

#include <wrap-json.h>

#include <alsa/asoundlib.h>

#include <ctl-plugin.h>

#include "../4a-hal-utilities/4a-hal-utilities-data.h"
#include "../4a-hal-utilities/4a-hal-utilities-appfw-responses-handler.h"

#include "4a-hal-controllers-alsacore-link.h"
#include "4a-hal-controllers-value-handler.h"

/*******************************************************************************
 *		Map to alsa control types				       *
 ******************************************************************************/

static const char *const snd_ctl_elem_type_names[] = {
	[SND_CTL_ELEM_TYPE_NONE]= "NONE",
	[SND_CTL_ELEM_TYPE_BOOLEAN]= "BOOLEAN",
	[SND_CTL_ELEM_TYPE_INTEGER]="INTEGER",
	[SND_CTL_ELEM_TYPE_ENUMERATED]="ENUMERATED",
	[SND_CTL_ELEM_TYPE_BYTES]="BYTES",
	[SND_CTL_ELEM_TYPE_IEC958]="IEC958",
	[SND_CTL_ELEM_TYPE_INTEGER64]="INTEGER64",
};

/*******************************************************************************
 *		Alsa control types map from string function		       *
 ******************************************************************************/

snd_ctl_elem_type_t HalCtlsMapsAlsaTypeToEnum(const char *label)
{
	int idx;
	static int length = sizeof(snd_ctl_elem_type_names) / sizeof(char *);

	for(idx = 0; idx < length; idx++) {
		if(! strcasecmp(label, snd_ctl_elem_type_names[idx]))
			return (snd_ctl_elem_type_t) idx;
	}

	return SND_CTL_ELEM_TYPE_NONE;
}

/*******************************************************************************
 *		Free contents of 'CtlHalAlsaMapT' data structure	       *
 ******************************************************************************/

uint8_t HalCtlsFreeAlsaCtlsMap(struct CtlHalAlsaMapT *alsaCtlsMap)
{
	int idx;

	if(! alsaCtlsMap)
		return -1;

	if(alsaCtlsMap->ctlsCount > 0 && ! alsaCtlsMap->ctls)
		return -2;

	for(idx = 0; idx < alsaCtlsMap->ctlsCount; idx++) {
		free(alsaCtlsMap->ctls[idx].action);
		free(alsaCtlsMap->ctls[idx].ctl.alsaCtlProperties.enums);
		free(alsaCtlsMap->ctls[idx].ctl.alsaCtlProperties.dbscale);
	}

	free(alsaCtlsMap->ctls);

	free(alsaCtlsMap);

	return 0;
}

/*******************************************************************************
 *		HAL controllers alsacore calls funtions			       *
 ******************************************************************************/

int HalCtlsGetCardIdByCardPath(AFB_ApiT apiHandle, char *devPath)
{
	int cardId = -1;

	char *returnedStatus = NULL, *returnedInfo = NULL, *cardIdString = NULL;

	enum CallError returnedError;

	json_object *toSendJ, *returnJ = NULL, *responsJ, *devidJ;

	if(! apiHandle) {
		AFB_ApiError(apiHandle, "%s: api handle not available", __func__);
		return -1;
	}

	if(! devPath) {
		AFB_ApiError(apiHandle, "%s: dev path is not available", __func__);
		return -2;
	}

	wrap_json_pack(&toSendJ, "{s:s}", "devpath", devPath);

	if(AFB_ServiceSync(apiHandle, ALSACORE_API, ALSACORE_GETINFO_VERB, toSendJ, &returnJ)) {
		returnedError = HalUtlHandleAppFwCallError(apiHandle, ALSACORE_API, ALSACORE_GETINFO_VERB, returnJ, &returnedStatus, &returnedInfo);
		AFB_ApiWarning(apiHandle,
			       "Error %i during call to verb %s of %s api with status '%s' and info '%s'",
			       (int) returnedError,
			       ALSACORE_GETINFO_VERB,
			       ALSACORE_API,
			       returnedStatus ? returnedStatus : "not returned",
			       returnedInfo ? returnedInfo : "not returned");
	}
	else if(json_object_object_get_ex(returnJ, "response", &responsJ)) {
		if(json_object_object_get_ex(responsJ, "devid", &devidJ) && json_object_is_type(devidJ, json_type_string)) {
			cardIdString = (char *) json_object_get_string(devidJ);
			if(sscanf(cardIdString, "hw:%i", &cardId) <= 0) {
				AFB_ApiWarning(apiHandle, "Couldn't get valid devid from string: '%s'", cardIdString);
				cardId = -2;
			}
		}
		else {
			AFB_ApiWarning(apiHandle, "Response devid is not present/valid");
		}
	}

	if(returnJ)
		json_object_put(returnJ);

	return cardId;
}

int HalCtlsSubscribeToAlsaCardEvent(AFB_ApiT apiHandle, char *cardId)
{
	int err = 0;

	char *returnedStatus = NULL, *returnedInfo = NULL;

	enum CallError returnedError;

	json_object *subscribeQueryJ, *returnedJ = NULL, *returnedWarningJ;

	wrap_json_pack(&subscribeQueryJ, "{s:s}", "devid", cardId);
	if(AFB_ServiceSync(apiHandle, ALSACORE_API, ALSACORE_SUBSCRIBE_VERB, subscribeQueryJ, &returnedJ)) {
		returnedError = HalUtlHandleAppFwCallError(apiHandle, ALSACORE_API, ALSACORE_SUBSCRIBE_VERB, returnedJ, &returnedStatus, &returnedInfo);
		AFB_ApiError(apiHandle,
			     "Error %i during call to verb %s of %s api with status '%s' and info '%s'",
			     (int) returnedError,
			     ALSACORE_SUBSCRIBE_VERB,
			     ALSACORE_API,
			     returnedStatus ? returnedStatus : "not returned",
			     returnedInfo ? returnedInfo : "not returned");
		err = -1;
	}
	else if(! wrap_json_unpack(returnedJ, "{s:{s:o}}", "request", "info", &returnedWarningJ)) {
		AFB_ApiError(apiHandle,
			     "Warning raised during call to verb %s of %s api : '%s'",
			     ALSACORE_SUBSCRIBE_VERB,
			     ALSACORE_API,
			     json_object_get_string(returnedWarningJ));
		err = -2;
	}

	if(returnedJ)
		json_object_put(returnedJ);

	return err;
}

int HalCtlsGetAlsaCtlInfo(AFB_ApiT apiHandle, char *cardId, struct CtlHalAlsaCtl *currentAlsaCtl)
{
	int err = 0;

	char *returnedStatus = NULL, *returnedInfo = NULL;

	enum CallError returnedError;

	json_object *queryJ, *returnedJ = NULL;

	if(! apiHandle) {
		AFB_ApiError(apiHandle, "%s: api handle not available", __func__);
		return -1;
	}

	if(! cardId) {
		AFB_ApiError(apiHandle, "%s: card id is not available", __func__);
		return -2;
	}

	if(! currentAlsaCtl) {
		AFB_ApiError(apiHandle, "%s: alsa control data structure is not available", __func__);
		return -3;
	}

	if(currentAlsaCtl->name && currentAlsaCtl->numid > 0) {
		AFB_ApiError(apiHandle,
			     "%s: Can't have both a control name (%s) and a control uid (%i)",
			     __func__,
			     currentAlsaCtl->name,
			     currentAlsaCtl->numid);
		return -4;
	}
	else if(currentAlsaCtl->name) {
		wrap_json_pack(&queryJ, "{s:s s:s s:i}", "devid", cardId, "ctl", currentAlsaCtl->name, "mode", 3);
	}
	else if(currentAlsaCtl->numid > 0) {
		wrap_json_pack(&queryJ, "{s:s s:i s:i}", "devid", cardId, "ctl", currentAlsaCtl->numid, "mode", 3);
	}
	else {
		AFB_ApiError(apiHandle, "%s: Need at least a control name or a control uid", __func__);
		return -5;
	}

	if(AFB_ServiceSync(apiHandle, ALSACORE_API, ALSACORE_CTLGET_VERB, queryJ, &returnedJ)) {
		returnedError = HalUtlHandleAppFwCallError(apiHandle, ALSACORE_API, ALSACORE_CTLGET_VERB, returnedJ, &returnedStatus, &returnedInfo);
		AFB_ApiError(apiHandle,
			     "Error %i during call to verb %s of %s api with status '%s' and info '%s'",
			     (int) returnedError,
			     ALSACORE_CTLGET_VERB,
			     ALSACORE_API,
			     returnedStatus ? returnedStatus : "not returned",
			     returnedInfo ? returnedInfo : "not returned");
		err = -6;
	}
	else if(currentAlsaCtl->name && wrap_json_unpack(returnedJ, "{s:{s:i}}", "response", "id", &currentAlsaCtl->numid)) {
		AFB_ApiError(apiHandle, "Can't find alsa control 'id' from control 'name': '%s' on device '%s'", currentAlsaCtl->name, cardId);
		err = -7;
	}
	else if(! json_object_object_get_ex(returnedJ, "response", NULL)) {
		AFB_ApiError(apiHandle, "Can't find alsa control 'id': %i on device '%s'", currentAlsaCtl->numid, cardId);
		err = -8;
	}
	// TBD JAI : get dblinear/dbminmax/... values
	else if(wrap_json_unpack(returnedJ, "{s:{s:{s?:i s?:i s?:i s?:i s?:i}}}",
					    "response",
					    "ctl",
					    "type", (int *) &currentAlsaCtl->alsaCtlProperties.type,
					    "count", &currentAlsaCtl->alsaCtlProperties.count,
					    "min", &currentAlsaCtl->alsaCtlProperties.minval,
					    "max", &currentAlsaCtl->alsaCtlProperties.maxval,
					    "step", &currentAlsaCtl->alsaCtlProperties.step)) {
		AFB_ApiError(apiHandle,
			     "Didn't succeed to get control %i properties on device '%s' : '%s'",
			     currentAlsaCtl->numid,
			     cardId,
			     json_object_get_string(returnedJ));
		err = -9;
	}

	if(returnedJ)
		json_object_put(returnedJ);

	return err;
}

int HalCtlsSetAlsaCtlValue(AFB_ApiT apiHandle, char *cardId, int ctlId, json_object *valuesJ)
{
	int err = 0;

	char *returnedStatus = NULL, *returnedInfo = NULL;

	enum CallError returnedError;

	json_object *queryJ, *returnedJ = NULL, *returnedWarningJ;

	if(! apiHandle) {
		AFB_ApiError(apiHandle, "%s: api handle not available", __func__);
		return -1;
	}

	if(! cardId) {
		AFB_ApiError(apiHandle, "%s: card id is not available", __func__);
		return -2;
	}

	if(ctlId <= 0) {
		AFB_ApiError(apiHandle, "%s: Alsa control id is not valid", __func__);
		return -3;
	}

	if(! valuesJ) {
		AFB_ApiError(apiHandle, "%s: values to set json is not available", __func__);
		return -4;
	}

	wrap_json_pack(&queryJ, "{s:s s:{s:i s:o}}", "devid", cardId, "ctl", "id", ctlId, "val", json_object_get(valuesJ));

	if(AFB_ServiceSync(apiHandle, ALSACORE_API, ALSACORE_CTLSET_VERB, queryJ, &returnedJ)) {
		returnedError = HalUtlHandleAppFwCallError(apiHandle, ALSACORE_API, ALSACORE_CTLSET_VERB, returnedJ, &returnedStatus, &returnedInfo);
		AFB_ApiError(apiHandle,
			     "Error %i during call to verb %s of %s api with status '%s' and info '%s'",
			     (int) returnedError,
			     ALSACORE_CTLSET_VERB,
			     ALSACORE_API,
			     returnedStatus ? returnedStatus : "not returned",
			     returnedInfo ? returnedInfo : "not returned");
		err = 1;
	}
	else if(! wrap_json_unpack(returnedJ, "{s:{s:o}}", "request", "info", &returnedWarningJ)) {
		AFB_ApiError(apiHandle,
			     "Warning raised during call to verb %s of %s api : '%s'",
			     ALSACORE_CTLSET_VERB,
			     ALSACORE_API,
			     json_object_get_string(returnedWarningJ));
		err = 2;
	}

	if(returnedJ)
		json_object_put(returnedJ);

	return err;
}

int HalCtlsCreateAlsaCtl(AFB_ApiT apiHandle, char *cardId, struct CtlHalAlsaCtl *alsaCtlToCreate)
{
	int err = 0;

	char *returnedStatus = NULL, *returnedInfo = NULL;

	enum CallError returnedError;

	json_object *queryJ, *returnedJ = NULL, *returnedWarningJ, *responseJ;

	if(! apiHandle) {
		AFB_ApiError(apiHandle, "%s: api handle not available", __func__);
		return -1;
	}

	if(! cardId) {
		AFB_ApiError(apiHandle, "%s: card id is not available", __func__);
		return -2;
	}

	if(! alsaCtlToCreate) {
		AFB_ApiError(apiHandle, "%s: alsa control data structure is not available", __func__);
		return -3;
	}

	if(! alsaCtlToCreate->alsaCtlCreation) {
		AFB_ApiError(apiHandle, "%s: alsa control data for creation structure is not available", __func__);
		return -4;
	}

	wrap_json_pack(&queryJ, "{s:s s:{s:i s:s s:i s:i s:i s:i s:i}}",
				"devid", cardId,
				"ctl",
				"ctl", -1,
				"name", alsaCtlToCreate->name,
				"min", alsaCtlToCreate->alsaCtlCreation->minval,
				"max", alsaCtlToCreate->alsaCtlCreation->maxval,
				"step", alsaCtlToCreate->alsaCtlCreation->step,
				"type", (int) alsaCtlToCreate->alsaCtlCreation->type,
				"count", alsaCtlToCreate->alsaCtlCreation->count);

	if(AFB_ServiceSync(apiHandle, ALSACORE_API, ALSACORE_ADDCTL_VERB, queryJ, &returnedJ)) {
		returnedError = HalUtlHandleAppFwCallError(apiHandle, ALSACORE_API, ALSACORE_ADDCTL_VERB, returnedJ, &returnedStatus, &returnedInfo);
		AFB_ApiError(apiHandle,
			     "Error %i during call to verb %s of %s api with status '%s' and info '%s'",
			     (int) returnedError,
			     ALSACORE_GETINFO_VERB,
			     ALSACORE_API,
			     returnedStatus ? returnedStatus : "not returned",
			     returnedInfo ? returnedInfo : "not returned");
		err = -5;
	}
	else if(! wrap_json_unpack(returnedJ, "{s:{s:o}}", "request", "info", &returnedWarningJ)) {
		AFB_ApiError(apiHandle,
			     "Warning raised during call to verb %s of %s api : '%s'",
			     ALSACORE_GETINFO_VERB,
			     ALSACORE_API,
			     json_object_get_string(returnedWarningJ));
		err = -6;
	}
	else if(wrap_json_unpack(returnedJ, "{s:o}", "response", &responseJ)) {
		AFB_ApiError(apiHandle,
			     "Can't get response of call to verb %s of %s api : %s",
			     ALSACORE_GETINFO_VERB,
			     ALSACORE_API,
			     json_object_get_string(returnedJ));
		err = -7;
	}
	else if(wrap_json_unpack(responseJ, "{s:i}", "id", &alsaCtlToCreate->numid)) {
		AFB_ApiError(apiHandle,
			     "Can't get create id from %s of %s api",
			     ALSACORE_GETINFO_VERB,
			     ALSACORE_API);
		err = -8;
	}
	else if(wrap_json_unpack(responseJ, "{s:o}", "ctl", NULL)) {
		AFB_ApiWarning(apiHandle, "Control %s was already present but has been updated", alsaCtlToCreate->name);
	}

	if(returnedJ)
		json_object_put(returnedJ);

	return err;
}

/*******************************************************************************
 *		HAL controllers alsacore controls request callback	       *
 ******************************************************************************/

void HalCtlsActionOnAlsaCtl(AFB_ReqT request)
{
	char cardIdString[6];

	AFB_ApiT apiHandle;

	CtlConfigT *ctrlConfig;

	struct SpecificHalData *currentCtlHalData;
	struct CtlHalAlsaMap *currentAlsaCtl;

	json_object *requestJson, *valueJ, *convertedJ;

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

	if(currentCtlHalData->status == HAL_STATUS_UNAVAILABLE) {
		AFB_ReqFail(request, "hal_unavailable", "Seems that hal is not available");
		return;
	}

	currentAlsaCtl = (struct CtlHalAlsaMap *) afb_request_get_vcbdata(request);
	if(! currentAlsaCtl) {
		AFB_ReqFail(request, "alsa_control_data", "Can't get current alsa control data");
		return;
	}

	if(currentAlsaCtl->ctl.numid <= 0) {
		AFB_ReqFail(request, "alsa_control_id", "Alsa control id is not valid");
		return;
	}

	requestJson = AFB_ReqJson(request);
	if(! requestJson) {
		AFB_ReqFail(request, "request_json", "Can't get request json");
		return;
	}

	switch(json_object_get_type(requestJson)) {
		case json_type_object :
		case json_type_array :
			break;

		default:
			AFB_ReqFailF(request, "request_json", "Request json is not valid '%s'", json_object_get_string(requestJson));
			return;
	}

	snprintf(cardIdString, 6, "hw:%i", currentCtlHalData->sndCardId);

	if(wrap_json_unpack(requestJson, "{s:o}", "val", &valueJ)) {
		AFB_ReqFailF(request,
			     "request_json", "Error when trying to get request value object inside request '%s'",
			     json_object_get_string(requestJson));
		return;
	}

	if(HalCtlsNormalizeJsonValues(apiHandle, &currentAlsaCtl->ctl.alsaCtlProperties, valueJ, &convertedJ)) {
		AFB_ReqFailF(request, "request_json", "Error when trying to convert request json '%s'", json_object_get_string(requestJson));
		return;
	}

	if(HalCtlsSetAlsaCtlValue(apiHandle, cardIdString, currentAlsaCtl->ctl.numid, convertedJ)) {
		AFB_ReqFailF(request,
			     "alsa_control_call_error",
			     "Error while trying to set value on alsa control %i, device '%s', message '%s'",
			     currentAlsaCtl->ctl.numid,
			     cardIdString,
			     json_object_get_string(requestJson));
		json_object_put(convertedJ);
		return;
	}

	AFB_ReqSuccess(request, NULL, "Action on alsa control correclty done");

	json_object_put(convertedJ);
}