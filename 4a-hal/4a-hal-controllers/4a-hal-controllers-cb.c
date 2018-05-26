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

#include <wrap-json.h>

#include "../4a-hal-utilities/4a-hal-utilities-data.h"
#include "../4a-hal-utilities/4a-hal-utilities-verbs-loader.h"

#include "4a-hal-controllers-cb.h"

/*******************************************************************************
 *		HAL controllers sections parsing functions		       *
 ******************************************************************************/

int HalCtlsHalMixerConfig(afb_dynapi *apiHandle, CtlSectionT *section, json_object *MixerJ)
{
	unsigned int streamCount, idx;
	char *currentStreamName;

	CtlConfigT *ctrlConfig;
	struct SpecificHalData *currentHalData;

	json_object *streamsArray, *currentStream;

	struct HalUtlApiVerb *CtlHalDynApiStreamVerbs;

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

		if(! json_object_object_get_ex(MixerJ, "streams", &streamsArray))
			return -6;

		switch(json_object_get_type(streamsArray)) {
			case json_type_object:
				streamCount = 1;
				break;
			case json_type_array:
				streamCount = json_object_array_length(streamsArray);
				break;
			default:
				return -7;
		}

		currentHalData->ctlHalSpecificData->ctlHalStreamsData.count = streamCount;
		currentHalData->ctlHalSpecificData->ctlHalStreamsData.data =
			(struct CtlHalStreamData *) calloc(streamCount, sizeof (struct CtlHalStreamData *));

		CtlHalDynApiStreamVerbs = alloca((streamCount + 1) * sizeof(struct HalUtlApiVerb));
		memset(CtlHalDynApiStreamVerbs, '\0', (streamCount + 1) * sizeof(struct HalUtlApiVerb));
		CtlHalDynApiStreamVerbs[streamCount + 1].verb = NULL;

		for(idx = 0; idx < streamCount; idx++) {
			if(streamCount > 1)
				currentStream = json_object_array_get_idx(streamsArray, (int) idx);
			else
				currentStream = streamsArray;

			if(wrap_json_unpack(currentStream, "{s:s}", "uid", &currentStreamName))
				return -9-idx;

			currentHalData->ctlHalSpecificData->ctlHalStreamsData.data[idx].name = strdup(currentStreamName);
			currentHalData->ctlHalSpecificData->ctlHalStreamsData.data[idx].cardId = NULL;

			CtlHalDynApiStreamVerbs[idx].verb = currentStreamName;
			CtlHalDynApiStreamVerbs[idx].callback = HalCtlsActionOnStream;
			CtlHalDynApiStreamVerbs[idx].info = "Peform action on this stream";
		}

		if(HalUtlLoadVerbs(apiHandle, CtlHalDynApiStreamVerbs))
			return -8;
	}

	return 0;
}

// TODO JAI: create a new section parser to get 'halmap' and store them into hal structure

/*******************************************************************************
 *		HAL controllers verbs functions				       *
 ******************************************************************************/

// TODO JAI : to implement
void HalCtlsActionOnStream(afb_request *request)
{
	AFB_REQUEST_WARNING(request, "JAI :%s not implemented yet", __func__);

	afb_request_success(request, json_object_new_boolean(false), NULL);
}

// TODO JAI : to implement
void HalCtlsListVerbs(afb_request *request)
{
	AFB_REQUEST_WARNING(request, "JAI :%s not implemented yet", __func__);

	afb_request_success(request, json_object_new_boolean(false), NULL);
}