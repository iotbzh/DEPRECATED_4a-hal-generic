/*
 * Copyright (C) 2018, "IoT.bzh", Microchip Technology Inc. and its subsidiaries.
 * Author Jonathan Aillet
 * Author Tobias Jahnke
 * Contrib Fulup Ar Foll
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
 *
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <wrap-json.h>

#include <ctl-plugin.h>

#include "wrap_unicens.h"
#include "wrap_volume.h"

CTLP_CAPI_REGISTER("hal-unicens")

#define PCM_MAX_CHANNELS  6

AFB_ApiT unicensHalApiHandle;

static int master_volume = 80;
static json_bool master_switch;
static int pcm_volume[PCM_MAX_CHANNELS] = {100,100,100,100,100,100};

CTLP_CAPI(MasterVol, source, argsJ, queryJ)
{
	const char *j_str = json_object_to_json_string(queryJ);

	if(! wrap_json_unpack(queryJ, "[i!]", "val", &master_volume)) {
		AFB_ApiNotice(source->api, "master_volume: %s, value=%d", j_str, master_volume);
		wrap_volume_master(source->api, master_volume);
	}
	else {
		AFB_ApiNotice(source->api, "master_volume: INVALID STRING %s", j_str);
	}

	return 0;
}

CTLP_CAPI(MasterSwitch, source, argsJ, queryJ)
{
	const char *j_str = json_object_to_json_string(queryJ);

	if(! wrap_json_unpack(queryJ, "[b!]", "val", &master_switch))
		AFB_ApiNotice(source->api, "master_switch: %s, value=%d", j_str, master_switch);
	else
		AFB_ApiNotice(source->api, "master_switch: INVALID STRING %s", j_str);

	return 0;
}

CTLP_CAPI(PCMVol, source, argsJ, queryJ)
{
	const char *j_str = json_object_to_json_string(queryJ);

	if(! wrap_json_unpack(queryJ, "[iiiiii!]",
				     "val",
				     &pcm_volume[0],
				     &pcm_volume[1],
				     &pcm_volume[2],
				     &pcm_volume[3],
				     &pcm_volume[4],
				     &pcm_volume[5])) {
		AFB_ApiNotice(source->api, "pcm_vol: %s", j_str);
		wrap_volume_pcm(source->api, pcm_volume, PCM_MAX_CHANNELS);
	}
	else {
		AFB_ApiNotice(source->api, "pcm_vol: INVALID STRING %s", j_str);
	}

	return 0;
}

/* initializes ALSA sound card, UNICENS API */
CTLP_CAPI(Init, source, argsJ, queryJ)
{
	int err = 0;

	AFB_ApiNotice(source->api, "Initializing HAL-MOST-UNICENS-BINDING");

	unicensHalApiHandle = source->api;

	if((err = wrap_ucs_subscribe_sync(source->api))) {
		AFB_ApiError(source->api, "Failed to subscribe to UNICENS binding");
		return err;
	}

	if((err = wrap_volume_init())) {
		AFB_ApiError(source->api, "Failed to initialize wrapper for volume library");
		return err;
	}

	// Set output volume to pre-defined level in order to
	// avoid muted volume to be persistent after boot.
	wrap_volume_master(source->api, 80);
	wrap_volume_pcm(source->api, pcm_volume, PCM_MAX_CHANNELS);

	AFB_ApiNotice(source->api, "Initializing HAL-MOST-UNICENS-BINDING done..");

	return 0;
}

// This receive UNICENS events
CTLP_CAPI(Events, source, argsJ, queryJ)
{
	int node;
	int available;

	if(wrap_json_unpack(queryJ, "{s:i,s:b}", "node", &node, "available", &available)) {
		AFB_ApiNotice(source->api, "Node-Availability: node=0x%03X, available=%d", node, available);
		wrap_volume_node_avail(source->api, node, available);
	}

	return 0;
}
