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

#ifndef _HAL_CTLS_VALUE_NORMALIZATION_INCLUDE_
#define _HAL_CTLS_VALUE_NORMALIZATION_INCLUDE_

#include <stdio.h>

#include <wrap-json.h>

#include "4a-hal-controllers-alsacore-link.h"

// Simple conversion value to/from percentage functions
int HalCtlsConvertValueToPercentage(double val, double min, double max);
int HalCtlsConvertPercentageToValue(int percentage, int min, int max);

// Convert json object from percentage to value
int HalCtlsNormalizeJsonValues(AFB_ApiT apiHandle, struct CtlHalAlsaCtlProperties *alsaCtlProperties, json_object *toConvertJ, json_object ** ConvertedJ);

#endif /* _HAL_CTLS_VALUE_NORMALIZATION_INCLUDE_ */