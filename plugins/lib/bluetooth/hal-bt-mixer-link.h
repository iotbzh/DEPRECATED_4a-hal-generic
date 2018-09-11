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

#ifndef _HAL_BT_MIXER_LINK_INCLUDE_
#define _HAL_BT_MIXER_LINK_INCLUDE_

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <ctl-config.h>

#define MIXER_SET_STREAMED_BT_DEVICE_VERB	"bluezalsa_dev"

// HAL controllers handle mixer bt calls functions	
int HalBtMixerLinkSetBtStreamingSettings(AFB_ApiT apiHandle, char *mixerApiName, unsigned int btStreamStatus, char *hci, char *btAddress);

#endif /* _HAL_BT_MIXER_LINK_INCLUDE_ */