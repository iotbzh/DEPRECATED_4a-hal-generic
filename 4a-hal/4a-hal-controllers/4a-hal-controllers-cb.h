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

#ifndef _HAL_CTLS_CB_INCLUDE_
#define _HAL_CTLS_CB_INCLUDE_

#include <stdio.h>

#include <afb-definitions.h>

#include <ctl-config.h>

// HAL controller event handler function
void HalCtlsDispatchApiEvent(afb_dynapi *apiHandle, const char *evtLabel, json_object *eventJ);

// HAL controllers sections parsing functions
int HalCtlsHalMixerConfig(AFB_ApiT apiHandle, CtlSectionT *section, json_object *MixerJ);
int HalCtlsHalMapConfig(AFB_ApiT apiHandle, CtlSectionT *section, json_object *StreamControlsJ);

// HAL controllers verbs functions
void HalCtlsActionOnStream(AFB_ReqT request);
void HalCtlsInfo(AFB_ReqT request);

#endif /* _HALMGR_CB_INCLUDE_ */