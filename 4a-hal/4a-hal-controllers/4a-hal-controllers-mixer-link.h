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

#ifndef _HAL_CTLS_SOFTMIXER_LINK_INCLUDE_
#define _HAL_CTLS_SOFTMIXER_LINK_INCLUDE_

#include <stdio.h>

#include <wrap-json.h>

#include "afb-helpers-utils.h"

#include "../4a-hal-utilities/4a-hal-utilities-data.h"

#define MIXER_ATTACH_VERB	"attach"
#define MIXER_INFO_VERB		"info"

#define HAL_PLAYBACK_VERB	"playback"
#define HAL_CAPTURE_VERB	"capture"

// Enum for the type of object sent back by the mixer
enum MixerDataType {
	MIXER_DATA_STREAMS = 1,
	MIXER_DATA_PLAYBACKS = 2,
	MIXER_DATA_CAPTURES = 3
};

// Enum for the type of error detected
enum MixerStatus {
	MIXER_NO_ERROR=0,
	MIXER_ERROR_API_UNAVAILABLE=-1,
	MIXER_ERROR_DATA_UNAVAILABLE=-2,
	MIXER_ERROR_DATA_EMPTY =-3,
	MIXER_ERROR_DATA_NAME_UNAVAILABLE=-10,
	MIXER_ERROR_DATA_CARDID_UNAVAILABLE=-1000,
	MIXER_ERROR_STREAM_VERB_NOT_CREATED =-100000,
	MIXER_ERROR_PLAYBACK_VERB_NOT_CREATED =-4,
	MIXER_ERROR_CAPTURE_VERB_NOT_CREATED =-5
};

// HAL controllers handle mixer calls functions
int HalCtlsAttachToMixer(AFB_ApiT apiHandle);
int HalCtlsGetInfoFromMixer(AFB_ApiT apiHandle, char *apiToCall, json_object *requestJson, json_object **toReturnJ, char **returnedStatus, char **returnedInfo);

#endif /* _HAL_CTLS_SOFTMIXER_LINK_INCLUDE_ */