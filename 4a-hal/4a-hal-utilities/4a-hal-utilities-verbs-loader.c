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
#include <stdbool.h>
#include <string.h>

#include "4a-hal-utilities-verbs-loader.h"

/*******************************************************************************
 *		Dynamic API common functions				       *
 *		TODO JAI : Use API-V3 instead of API-PRE-V3		       *
 ******************************************************************************/

int HalUtlLoadVerbs(afb_dynapi *apiHandle, struct HalUtlApiVerb *verbs)
{
	int idx, errCount = 0;

	if(! apiHandle || ! verbs)
		return -1;

	for (idx = 0; verbs[idx].verb; idx++) {
		errCount+= afb_dynapi_add_verb(apiHandle,
					       verbs[idx].verb,
					       NULL,
					       verbs[idx].callback,
					       (void*) &verbs[idx],
					       verbs[idx].auth,
					       0);
	}

	return errCount;
}