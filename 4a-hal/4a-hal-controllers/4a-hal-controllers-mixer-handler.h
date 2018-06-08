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

#define MIXER_ATTACH_VERB	"create"

// Enum for the type of error detected
enum MixerStatus {
	MIXER_NO_ERROR=0,
	MIXER_ERROR_API_UNAVAILABLE=-1,
	MIXER_ERROR_NO_STREAMS=-2,
	MIXER_ERROR_STREAM_NAME_UNAVAILABLE=-10,
	MIXER_ERROR_STREAM_CARDID_UNAVAILABLE=-1000,
	MIXER_ERROR_COULDNT_ADD_STREAMS_AS_VERB =-100000,
};

// HAL controllers handle mixer calls functions
int HalCtlsAttachToMixer(AFB_ApiT apiHandle);

#endif /* _HAL_CTLS_SOFTMIXER_LINK_INCLUDE_ */