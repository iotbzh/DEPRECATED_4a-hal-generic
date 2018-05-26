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

#ifndef _HAL_UTILITIES_VERBS_LOADER_INCLUDE_
#define _HAL_UTILITIES_VERBS_LOADER_INCLUDE_

#include <stdio.h>

#define AFB_BINDING_VERSION dyn
#include <afb/afb-binding.h>

// Structure to store data necessary to add a verb to a dynamic api
struct HalUtlApiVerb {
	const char *verb;			/* name of the verb, NULL only at end of the array */
	void (*callback)(afb_request *req);	/* callback function implementing the verb */
	const struct afb_auth *auth;		/* required authorisation, can be NULL */
	const char *info;			/* some info about the verb, can be NULL */
	uint32_t session;
};

// Verb that allows to add verb to a dynamic api
int HalUtlLoadVerbs(afb_dynapi *apiHandle, struct HalUtlApiVerb *verbs);

#endif /* _HAL_UTILITIES_VERBS_LOADER_INCLUDE_ */