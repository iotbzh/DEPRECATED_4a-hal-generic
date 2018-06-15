/*
 * Copyright (C) 2018,, Microchip Technology Inc. and its subsidiaries, "IoT.bzh".
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

#pragma once

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <ctl-plugin.h>

extern AFB_ApiT unicensHalApiHandle;

/* Asynchronous API: result callback */
typedef void (*wrap_ucs_result_cb_t)(uint8_t result, void *user_ptr);

/* Asynchronous API: functions */
extern int wrap_ucs_i2cwrite(uint16_t node,
			     uint8_t *data_ptr,
			     uint8_t data_sz,
			     wrap_ucs_result_cb_t result_fptr,
			     void *result_user_ptr);

/* Synchronous API: functions */
extern int wrap_ucs_subscribe_sync(AFB_ApiT apiHandle);
extern int wrap_ucs_i2cwrite_sync(AFB_ApiT apiHandle, uint16_t node, uint8_t *data_ptr, uint8_t data_sz);
