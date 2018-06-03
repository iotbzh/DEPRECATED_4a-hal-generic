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

#ifndef HAL_CTLS_ALSACORE_LINK_INCLUDE_
#define HAL_CTLS_ALSACORE_LINK_INCLUDE_

#include <stdio.h>
#include <stdbool.h>

#define ALSACORE_API		"alsacore"
#define ALSACORE_GETINFO_VERB	"infoget"

// HAL controllers alsacore calls funtions
bool HalCtlsGetCardIdByCardPath(afb_dynapi *apiHandle, struct SpecificHalData *currentCtlHalData);

#endif /* HAL_CTLS_ALSACORE_LINK_INCLUDE_ */