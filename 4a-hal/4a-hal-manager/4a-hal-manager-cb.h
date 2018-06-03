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

#ifndef _HALMGR_CB_INCLUDE_
#define _HALMGR_CB_INCLUDE_

#include <stdio.h>

#define AFB_BINDING_VERSION dyn
#include <afb/afb-binding.h>

// Event handler
void HalMgrDispatchApiEvent(afb_dynapi *apiHandle, const char *evtLabel, json_object *eventJ);

// Exported verbs callbacks
void HalMgrPing(afb_request *request);

void HalMgrLoaded(afb_request *request);
void HalMgrLoad(afb_request *request);
void HalMgrUnload(afb_request *request);
void HalMgrSubscribeEvent(afb_request *request);
void HalMgrUnsubscribeEvent(afb_request *request);

#endif /* _HALMGR_CB_INCLUDE_ */