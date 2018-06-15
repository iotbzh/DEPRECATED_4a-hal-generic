/*
 * Copyright (C) 2018, Microchip Technology Inc. and its subsidiaries, "IoT.bzh".
 * Author Tobias Jahnke
 * Author Jonathan Aillet
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

#include <time.h>
#include <assert.h>

#include <wrap-json.h>

#include <ctl-plugin.h>

#include "wrap_volume.h"
#include "wrap_unicens.h"

#include "libmostvolume.h"

static int wrap_volume_service_timeout_cb(sd_event_source* source,
					  uint64_t timer __attribute__((__unused__)),
					  void *userdata __attribute__((__unused__)))
{
	uint8_t ret;

	sd_event_source_unref(source);

	if((ret = lib_most_volume_service()))
		AFB_ApiError(unicensHalApiHandle, "lib_most_volume_service returns %d", ret);

	return 0;
}

static void wrap_volume_service_cb(uint16_t timeout)
{
	uint64_t usec;

	sd_event_now(AFB_GetEventLoop(unicensHalApiHandle), CLOCK_BOOTTIME, &usec);

	sd_event_add_time(AFB_GetEventLoop(unicensHalApiHandle),
			  NULL,
			  CLOCK_MONOTONIC,
			  usec + (timeout*1000),
			  250,
			  wrap_volume_service_timeout_cb,
			  NULL);
}

/* Retrieves a new value adapted to a new maximum value. Minimum value is
 * always zero. */
static int wrap_volume_calculate(int value, int max_old, int max_new)
{
	if(value > max_old)
		value = max_old;

	value = (value * max_new) / max_old; /* calculate range: 0..255 */
	assert(value <= max_new);

	return value;
}

extern int wrap_volume_init(void)
{
	uint8_t ret = 0;

	lib_most_volume_init_t mv_init;

	mv_init.writei2c_cb = &wrap_ucs_i2cwrite;
	mv_init.service_cb = wrap_volume_service_cb;

	ret = lib_most_volume_init(&mv_init);

	return -ret;
}

extern int wrap_volume_master(AFB_ApiT apiHandle, int volume)
{
	int ret;

	if((ret = lib_most_volume_set(LIB_MOST_VOLUME_MASTER,
				     (uint8_t) wrap_volume_calculate(volume, 100, 255)))) {
		AFB_ApiError(apiHandle, "%s: volume library not ready.", __func__);
	}

	return -ret;
}

extern int wrap_volume_pcm(AFB_ApiT apiHandle, int *volume_ptr, int volume_sz)
{
	const int MAX_PCM_CHANNELS = 6;
	int cnt, ret, new_value;

	assert(volume_ptr != NULL);
	assert(volume_sz <= MAX_PCM_CHANNELS);

	for(cnt = 0; cnt < volume_sz; cnt ++) {
		new_value = wrap_volume_calculate(volume_ptr[cnt], 100, 255);

		if((ret = lib_most_volume_set((enum lib_most_volume_channel_t) cnt, (uint8_t) new_value))) {
			AFB_ApiError(apiHandle, "%s: volume library not ready.", __func__);
			break;
		}
	}

	return -ret;
}

extern int wrap_volume_node_avail(AFB_ApiT apiHandle, int node, int available)
{
	int ret;

	if((ret = lib_most_volume_node_available((uint16_t) node, (uint8_t) available)))
		AFB_ApiError(apiHandle, "%s: volume library not ready.", __func__);

	return -ret;
}

