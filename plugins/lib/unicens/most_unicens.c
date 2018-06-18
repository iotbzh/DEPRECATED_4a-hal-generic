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

static uint8_t initialized = 0;

// Call at initialisation time
CTLP_ONLOAD(plugin, callbacks)
{
	AFB_ApiNotice(plugin->api, "Hal-Unicens Plugin Register: uid='%s' 'info='%s'", plugin->uid, plugin->info);

	unicensHalApiHandle = plugin->api;

	return 0;
}

CTLP_CAPI(MasterVol, source, argsJ, queryJ)
{
	int master_volume;

	json_object *valueJ;

	if(! initialized) {
		AFB_ApiWarning(source->api, "%s: Link to unicens binder is not initialized, can't set master volume, value=%s", __func__, json_object_get_string(queryJ));
		return -1;
	}

	if(! json_object_is_type(queryJ, json_type_array) || json_object_array_length(queryJ) <= 0) {
		AFB_ApiError(source->api, "%s: invalid json (should be a non empty json array) value=%s", __func__, json_object_get_string(queryJ));
		return -2;
	}

	valueJ = json_object_array_get_idx(queryJ, 0);
	if(! json_object_is_type(valueJ, json_type_int)) {
		AFB_ApiError(source->api, "%s: invalid json (should be an array of int) value=%s", __func__, json_object_get_string(queryJ));
		return -3;
	}

	master_volume = json_object_get_int(valueJ);
	wrap_volume_master(source->api, master_volume);

	return 0;
}

CTLP_CAPI(MasterSwitch, source, argsJ, queryJ)
{
	json_bool master_switch;
	json_object *valueJ;

	if(! initialized) {
		AFB_ApiWarning(source->api, "%s: Link to unicens binder is not initialized, can't set master switch, value=%s", __func__, json_object_get_string(queryJ));
		return -1;
	}

	if(! json_object_is_type(queryJ, json_type_array) || json_object_array_length(queryJ) <= 0) {
		AFB_ApiError(source->api, "%s: invalid json (should be a non empty json array) value=%s", __func__, json_object_get_string(queryJ));
		return -2;
	}

	// In case if alsa doesn't return a proper json boolean
	valueJ = json_object_array_get_idx(queryJ, 0);
	switch(json_object_get_type(valueJ)) {
		case json_type_boolean:
			master_switch = json_object_get_boolean(valueJ);
			break;

		case json_type_int:
			master_switch = json_object_get_int(valueJ);
			break;

		default:
			AFB_ApiError(source->api, "%s: invalid json (should be an array of boolean/int) value=%s", __func__, json_object_get_string(queryJ));
			return -3;
	}

	// TBD: implement pause action for Unicens
	AFB_ApiWarning(source->api, "%s: Try to set master switch to %i, but function is not implemented", __func__, (int) master_switch);

	return 0;
}

CTLP_CAPI(PCMVol, source, argsJ, queryJ)
{
	int idx;
	int pcm_volume[PCM_MAX_CHANNELS];

	json_object *valueJ;

	if(! initialized) {
		AFB_ApiWarning(source->api, "%s: Link to unicens binder is not initialized, can't set PCM volume, value=%s", __func__, json_object_get_string(queryJ));
		return -1;
	}

	if(! json_object_is_type(queryJ, json_type_array) || json_object_array_length(queryJ) <= 0) {
		AFB_ApiError(source->api, "%s: invalid json (should be a non empty json array) value=%s", __func__, json_object_get_string(queryJ));
		return -1;
	}

	for(idx = 0; idx < json_object_array_length(queryJ); idx++) {
		valueJ = json_object_array_get_idx(queryJ, idx);
		if(! json_object_is_type(valueJ, json_type_int)) {
			AFB_ApiError(source->api, "%s: invalid json (should be an array of int) value=%s", __func__, json_object_get_string(queryJ));
			return -2;
		}

		pcm_volume[idx] = json_object_get_int(valueJ);
	}

	// If control only has one value, then replicate the first value
	for(idx = idx; idx < 6; idx ++)
		pcm_volume[idx] = pcm_volume[0];

	wrap_volume_pcm(source->api, pcm_volume, PCM_MAX_CHANNELS);

	return 0;
}

/* initializes ALSA sound card, UNICENS API */
CTLP_CAPI(Init, source, argsJ, queryJ)
{
	int err = 0;
	int pcm_volume[PCM_MAX_CHANNELS] = { 80, 80, 80, 80, 80, 80 };

	AFB_ApiNotice(source->api, "Initializing HAL-MOST-UNICENS-BINDING");

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

	initialized = 1;

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
