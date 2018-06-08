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

#include "../4a-hal-utilities/4a-hal-utilities-data.h"
#include "../4a-hal-utilities/4a-hal-utilities-appfw-responses-handler.h"

#include "4a-hal-controllers-alsacore-link.h"

/*******************************************************************************
 *		HAL controllers alsacore calls funtions			       *
 ******************************************************************************/

int HalCtlsGetCardIdByCardPath(AFB_ApiT apiHandle, char *devPath)
{
	int cardId = -1;

	char *returnedStatus = NULL, *returnedInfo = NULL, *cardIdString = NULL;

	enum CallError returnedError;

	json_object *toSendJ, *returnJ, *responsJ, *devidJ;

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

	return cardId;
}