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

#pragma once

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <systemd/sd-event.h>

#include <ctl-plugin.h>

extern int wrap_volume_init(void);
extern int wrap_volume_master(AFB_ApiT apiHandle, int volume);
extern int wrap_volume_pcm(AFB_ApiT apiHandle, int *volume_ptr, int volume_sz);
extern int wrap_volume_node_avail(AFB_ApiT apiHandle, int node, int available);