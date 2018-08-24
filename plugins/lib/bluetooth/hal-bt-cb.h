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

#ifndef _HAL_BT_CB_INCLUDE_
#define _HAL_BT_CB_INCLUDE_

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <ctl-plugin.h>

#define HAL_BT_GET_STREAMING_STATUS_VERB	"get_bt_streaming_status"
#define HAL_BT_SET_STREAMING_STATUS_VERB	"set_bt_streaming_status"
#define HAL_BT_GET_CONNECTED_DEVICES_VERB	"get_connected_bt_devices"
#define HAL_BT_GET_SELECTED_DEVICE_VERB		"get_selected_bt_device"
#define HAL_BT_SET_SELECTED_DEVICE_VERB		"set_selected_bt_device"

// HAL Bluetooth plugin verbs functions
void HalBtGetStreamingStatus(AFB_ReqT request);
void HalBtSetStreamingStatus(AFB_ReqT request);
void HalBtGetConnectedBluetoothDevices(AFB_ReqT request);
void HalBtGetSelectedBluetoothDevice(AFB_ReqT request);
void HalBtSetSelectedBluetoothDevice(AFB_ReqT request);

#endif /* _HAL_BT_CB_INCLUDE_ */