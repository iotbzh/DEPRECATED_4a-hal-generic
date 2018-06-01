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

#include "../4a-hal-utilities/4a-hal-utilities-verbs-loader.h"

#include "4a-hal-controllers-mixer-handler.h"
#include "4a-hal-controllers-cb.h"

/*******************************************************************************
 *		HAL controllers hanlde mixer responses functions	       *
 ******************************************************************************/

void HalCtlsHandleMixerCallError(AFB_ReqT request, char *apiCalled, char *verbCalled, json_object *callReturnJ, char *errorStatus)
{
	char *returnedStatus, *returnedInfo;

	json_object *returnedRequestJ, *returnedStatusJ, *returnedInfoJ;

	if(! json_object_object_get_ex(callReturnJ, "request", &returnedRequestJ)) {
		AFB_ReqFail(request, errorStatus, "Couldn't get response request object");
		return;
	}

	if(! json_object_is_type(returnedRequestJ, json_type_object)) {
		AFB_ReqFail(request, errorStatus, "Response request object is not valid");
		return;
	}

	if(! json_object_object_get_ex(returnedRequestJ, "status", &returnedStatusJ)) {
		AFB_ReqFail(request, errorStatus, "Couldn't get response status object");
		return;
	}

	if(! json_object_is_type(returnedStatusJ, json_type_string)) {
		AFB_ReqFail(request, errorStatus, "Response status object is not valid");
		return;
	}

	returnedStatus = (char *) json_object_get_string(returnedStatusJ);

	if(! strcmp(returnedStatus, "unknown-api")) {
		AFB_ReqFailF(request,
				   errorStatus,
				   "Api %s not found",
				   apiCalled);
		return;
	}

	if(! strcmp(returnedStatus, "unknown-verb")) {
		AFB_ReqFailF(request,
				   errorStatus,
				   "Verb %s of api %s not found",
				   verbCalled,
				   apiCalled);
		return;
	}

	if(! json_object_object_get_ex(returnedRequestJ, "info", &returnedInfoJ)) {
		AFB_ReqFail(request, errorStatus, "Couldn't get response info object");
		return;
	}

	if(! json_object_is_type(returnedInfoJ, json_type_string)) {
		AFB_ReqFail(request, errorStatus, "Response info object is not valid");
		return;
	}

	returnedInfo = (char *) json_object_get_string(returnedInfoJ);

	AFB_ReqFailF(request,
			   errorStatus,
			   "Api %s and verb %s found, but this error was raised : '%s' with this info : '%s'",
			   apiCalled,
			   verbCalled,
			   returnedStatus,
			   returnedInfo);
}

int HalCtlsHandleMixerAttachResponse(AFB_ReqT request, struct CtlHalStreamsDataT *currentHalStreamsData, json_object *mixerResponseJ)
{
	int err = 0;
	unsigned int idx;

	char *currentStreamName, *currentStreamCardId;

	AFB_ApiT apiHandle;

	struct HalUtlApiVerb *CtlHalDynApiStreamVerbs;

	json_object *currentStreamJ;

	apiHandle = (AFB_ApiT) afb_request_get_dynapi(request);
	if(! apiHandle) {
		AFB_ReqWarning(request, "%s: Can't get current hal api handle", __func__);
		return -1;
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
			AFB_ReqWarning(request, "%s: no streams returned", __func__);
			return -2;
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
			AFB_ReqWarning(request, "%s: can't find name in current stream object", __func__);
			err -= 10;
		}
		else if(wrap_json_unpack(currentStreamJ, "{s:s}", "alsa", &currentStreamCardId)) {
			AFB_ReqWarning(request, "%s: can't find card id in current stream object", __func__);
			err -= 1000;
		}
		else {
			currentHalStreamsData->data[idx].name = strdup(currentStreamName);
			currentHalStreamsData->data[idx].cardId = strdup(currentStreamCardId);

			CtlHalDynApiStreamVerbs[idx].verb = currentStreamName;
			CtlHalDynApiStreamVerbs[idx].callback = HalCtlsActionOnStream;
			CtlHalDynApiStreamVerbs[idx].info = "Peform action on this stream";
		}
	}

	if(HalUtlLoadVerbs(apiHandle, CtlHalDynApiStreamVerbs)) {
		AFB_ReqWarning(request, "%s: error while creating verbs for streams", __func__);
		err -= 100000;
	}

	return err;
}