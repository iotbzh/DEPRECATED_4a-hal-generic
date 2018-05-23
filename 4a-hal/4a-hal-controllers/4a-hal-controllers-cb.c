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

#include "4a-hal-controllers-cb.h"

/*******************************************************************************
 *		HAL Manager verbs functions				       *
 ******************************************************************************/

// TODO JAI : to implement
void HalCtlsListVerbs(afb_request *request)
{
	AFB_REQUEST_WARNING(request, "JAI :%s not implemented yet", __func__);

	afb_request_success(request, json_object_new_boolean(false), NULL);
}