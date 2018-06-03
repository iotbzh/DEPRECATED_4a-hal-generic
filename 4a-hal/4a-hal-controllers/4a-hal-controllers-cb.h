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

#define AFB_BINDING_VERSION dyn
#include <afb/afb-binding.h>

#include <ctl-config.h>

// HAL controllers sections parsing functions
int HalCtlsHalMixerConfig(afb_dynapi *apiHandle, CtlSectionT *section, json_object *MixerJ);
int HalCtlsHalMapConfig(afb_dynapi *apiHandle, CtlSectionT *section, json_object *StreamControlsJ);

// HAL controllers verbs functions
void HalCtlsActionOnStream(afb_request *request);
void HalCtlsListVerbs(afb_request *request);
void HalCtlsInitMixer(afb_request *request);

#endif /* _HALMGR_CB_INCLUDE_ */