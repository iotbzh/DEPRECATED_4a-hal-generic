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
#include <string.h>
#include <stdbool.h>

#include <wrap-json.h>

#include <ctl-config.h>

#include "hal-bt-mixer-link.h"

/*******************************************************************************
 *		HAL controllers handle mixer bt calls functions		       *
 ******************************************************************************/

int HalBtMixerLinkSetBtStreamingSettings(AFB_ApiT apiHandle, char *mixerApiName, unsigned int btStreamStatus, char *hci, char *btAddress)
{
	char *returnedError = NULL;

	json_object *toSendJ, *returnedJ;

	if(! apiHandle || ! mixerApiName)
		return -1;

	if(! btStreamStatus)
		toSendJ = NULL;
	else if(btStreamStatus == 1 && hci && btAddress)
		wrap_json_pack(&toSendJ, "{s:s s:s s:s}", "interface", hci, "device", btAddress, "profile", "a2dp");
	else
		return -3;

	if(AFB_ServiceSync(apiHandle, mixerApiName, MIXER_SET_STREAMED_BT_DEVICE_VERB, toSendJ, &returnedJ)) {
		AFB_ApiError(apiHandle,
			     "Error during call to verb %s of %s api",
			     MIXER_SET_STREAMED_BT_DEVICE_VERB,
			     mixerApiName);
		return -3;
	}
	else if(! wrap_json_unpack(returnedJ, "{s:{s:s}}", "request", "info", &returnedError)) {
		AFB_ApiError(apiHandle,
			     "Error '%s' happened during set bluetooth streaming settings during call to verb '%s' of api '%s'",
			     returnedError ? returnedError : "no_error_string",
			     MIXER_SET_STREAMED_BT_DEVICE_VERB,
			     mixerApiName);
		return -4;
	}

	return 0;
}