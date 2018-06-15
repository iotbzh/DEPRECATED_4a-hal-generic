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

typedef struct async_job_ {
	wrap_ucs_result_cb_t result_fptr;
	void *result_user_ptr;
} async_job_t;

typedef struct parse_result_ {
	int done;
	char *str_result;
} parse_result_t;

/*
 * Subscribes to unicens2-binding events.
 * \return	Returns 0 if successful, otherwise != 0".
 */
extern int wrap_ucs_subscribe_sync(AFB_ApiT apiHandle)
{
	int err;

	json_object *j_response, *j_query = NULL;

	/* Build an empty JSON object */
	if((err = wrap_json_pack(&j_query, "{}"))) {
		AFB_ApiError(apiHandle, "Failed to create subscribe json object");
		return err;
	}

	if((err = AFB_ServiceSync(apiHandle, "UNICENS", "subscribe", j_query, &j_response))) {
		AFB_ApiError(apiHandle, "Fail subscribing to UNICENS events");
		return err;
	}
	else {
		AFB_ApiNotice(apiHandle, "Subscribed to UNICENS events, res=%s", json_object_to_json_string(j_response));
		json_object_put(j_response);
	}

	return 0;
}

/*
 * Write I2C command to a network node.
 * \param	node	Node address
 * \param	ata_ptr Reference to command data
 * \param 	data_sz	Size of the command data. Valid values: 1..32.
 * \return	Returns 0 if successful, otherwise != 0".
 */
extern int wrap_ucs_i2cwrite_sync(AFB_ApiT apiHandle, uint16_t node, uint8_t *data_ptr, uint8_t data_sz)
{
	int err;
	uint8_t cnt;

	json_object *j_response, *j_query, *j_array = NULL;

	j_query = json_object_new_object();
	j_array = json_object_new_array();

	if(! j_query || ! j_array) {
		AFB_ApiError(apiHandle, "Failed to create writei2c json objects");
		return -1;
	}

	for(cnt = 0U; cnt < data_sz; cnt++)
		json_object_array_add(j_array, json_object_new_int(data_ptr[cnt]));

	json_object_object_add(j_query, "node", json_object_new_int(node));
	json_object_object_add(j_query, "data", j_array);

	if((err = AFB_ServiceSync(apiHandle, "UNICENS", "writei2c", j_query, &j_response))) {
		AFB_ApiError(apiHandle, "Failed to call writei2c_sync");
		return err;
	}
	else {
		AFB_ApiInfo(apiHandle, "Called writei2c_sync, res=%s", json_object_to_json_string(j_response));
		json_object_put(j_response);
	}

	return 0;
}

/* ---------------------------- ASYNCHRONOUS API ---------------------------- */

static void wrap_ucs_i2cwrite_cb(void *closure, int status, struct json_object *j_result, AFB_ApiT apiHandle)
{
	async_job_t *job_ptr;

	AFB_ApiInfo(apiHandle, "%s: closure=%p status=%d, res=%s", __func__, closure, status, json_object_to_json_string(j_result));

	if(closure) {
		job_ptr = (async_job_t *) closure;

		if(job_ptr->result_fptr)
			job_ptr->result_fptr((uint8_t) abs(status), job_ptr->result_user_ptr);

		free(closure);
	}
}

/*
 * Write I2C command to a network node.
 * \param	node		Node address
 * \param	data_ptr	Reference to command data
 * \param	data_sz		Size of the command data. Valid values: 1..32.
 * \return	Returns 0 if successful, otherwise != 0".
 */
extern int wrap_ucs_i2cwrite(uint16_t node,
			     uint8_t *data_ptr,
			     uint8_t data_sz,
			     wrap_ucs_result_cb_t result_fptr,
			     void *result_user_ptr)
{
	uint8_t cnt;

	json_object *j_query, *j_array = NULL;
	async_job_t *job_ptr = NULL;

	j_query = json_object_new_object();
	j_array = json_object_new_array();

	if(! j_query || ! j_array) {
		AFB_ApiError(unicensHalApiHandle, "Failed to create writei2c json objects");
		return -1;
	}

	for(cnt = 0U; cnt < data_sz; cnt++)
		json_object_array_add(j_array, json_object_new_int(data_ptr[cnt]));

	json_object_object_add(j_query, "node", json_object_new_int(node));
	json_object_object_add(j_query, "data", j_array);

	job_ptr = malloc(sizeof(async_job_t));

	if(! job_ptr) {
		AFB_ApiError(unicensHalApiHandle, "Failed to create async job object");
		json_object_put(j_query);
		return -2;
	}

	job_ptr->result_fptr = result_fptr;
	job_ptr->result_user_ptr = result_user_ptr;

	AFB_ServiceCall(unicensHalApiHandle, "UNICENS", "writei2c", j_query, wrap_ucs_i2cwrite_cb, job_ptr);
	return 0;
}
