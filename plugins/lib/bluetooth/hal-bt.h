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

#ifndef _HAL_BT_INCLUDE_
#define _HAL_BT_INCLUDE_

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define HAL_BT_PLUGIN_NAME		"hal-bt"

#define BT_MANAGER_API			"Bluetooth-Manager"
#define BT_MANAGER_GET_POWER_INFO	"power"
#define BT_MANAGER_SUBSCRIBE_VERB	"subscribe"
#define BT_MANAGER_GET_DEVICES_VERB	"discovery_result"

#define BT_MANAGER_DEVICE_UPDATE_EVENT	"device_updated"

#endif /* _HAL_BT_INCLUDE_ */