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

#ifndef _HAL_UTILITIES_APPFW_RESP_HANDLER_INCLUDE_
#define _HAL_UTILITIES_APPFW_RESP_HANDLER_INCLUDE_

#include <stdio.h>

#include <wrap-json.h>

#include <afb-definitions.h>

// Enum for the type of error detected
enum CallError {
	CALL_ERROR_INVALID_ARGS=-1,
	CALL_ERROR_REQUEST_UNAVAILABLE=-2,
	CALL_ERROR_REQUEST_NOT_VALID=-3,
	CALL_ERROR_REQUEST_STATUS_UNAVAILABLE=-4,
	CALL_ERROR_REQUEST_STATUS_NOT_VALID=-5,
	CALL_ERROR_API_UNAVAILABLE=-6,
	CALL_ERROR_VERB_UNAVAILABLE=-7,
	CALL_ERROR_REQUEST_INFO_UNAVAILABLE=-8,
	CALL_ERROR_REQUEST_INFO_NOT_VALID=-9,
	CALL_ERROR_RETURNED=-10,
};

// Handle application framework response function
enum CallError HalUtlHandleAppFwCallError(AFB_ApiT apiHandle, char *apiCalled, char *verbCalled, json_object *callReturnJ, char **returnedStatus, char **returnedInfo);
void HalUtlHandleAppFwCallErrorInRequest(AFB_ReqT request, char *apiCalled, char *verbCalled, json_object *callReturnJ, char *errorStatusToSend);

#endif /* _HAL_UTILITIES_APPFW_RESP_HANDLER_INCLUDE_ */