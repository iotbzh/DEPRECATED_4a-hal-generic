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

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <limits.h>
#include <math.h>

#include <wrap-json.h>

#include "4a-hal-controllers-value-handler.h"
#include "4a-hal-controllers-alsacore-link.h"

/*******************************************************************************
 *		Simple conversion value to/from percentage functions	       *
 ******************************************************************************/

int HalCtlsConvertValueToPercentage(double val, double min, double max)
{
	double range;

	range = max - min;
	if(range <= 0)
		return -INT_MAX;

	val -= min;

	return (int) round(val / range * 100);
}

int HalCtlsConvertPercentageToValue(int percentage, int min, int max)
{
	int range;

	range = max - min;
	if(range <= 0)
		return -INT_MAX;

	return (int) round((double) percentage * (double) range * 0.01 + (double) min);
}

/*******************************************************************************
 *		Convert json object from percentage to value	 	       *
 ******************************************************************************/

int HalCtlsNormalizeJsonValues(AFB_ApiT apiHandle, struct CtlHalAlsaCtlProperties *alsaCtlProperties, json_object *toConvertJ, json_object **ConvertedJ)
{
	int err = 0, idx, count, initialValue, convertedValue;

	json_type toConvertType;
	json_object *toConvertObjectJ, *convertedValueJ, *convertedArrayJ;

	toConvertType = json_object_get_type(toConvertJ);
	switch(toConvertType) {
		case json_type_array:
			count = (int) json_object_array_length(toConvertJ);
			break;

		case json_type_null:
		case json_type_object:
			count = 0;
			AFB_ApiWarning(apiHandle, "%s: can't normalize json values :\n-- %s", __func__, json_object_get_string(toConvertJ));
			*ConvertedJ = json_object_get(toConvertJ);
			return -1;

		default:
			count = 1;
			break;
	}

	convertedArrayJ = json_object_new_array();

	for(idx = 0; idx < count; idx++) {
		if(toConvertType == json_type_array)
			toConvertObjectJ = json_object_array_get_idx(toConvertJ, idx);
		else
			toConvertObjectJ = toConvertJ;

		switch(alsaCtlProperties->type) {
			case SND_CTL_ELEM_TYPE_INTEGER:
			case SND_CTL_ELEM_TYPE_INTEGER64:
				if(! json_object_is_type(toConvertObjectJ, json_type_int)) {
					err += -10*idx;
					AFB_ApiWarning(apiHandle,
						"%s: Try normalize an integer control but value sent in json is not an integer : '%s'",
						__func__,
						json_object_get_string(toConvertObjectJ));
					convertedValueJ = toConvertObjectJ;
					break;
				}

				initialValue = json_object_get_int(toConvertObjectJ);
				convertedValue = HalCtlsConvertPercentageToValue(initialValue, alsaCtlProperties->minval, alsaCtlProperties->maxval);
				if(convertedValue == -INT_MAX) {
					AFB_ApiWarning(apiHandle,
						"%s: Can't normalize %i (using min %i et max %i), value set to 0",
						__func__,
						initialValue,
						alsaCtlProperties->minval,
						alsaCtlProperties->maxval);
					convertedValue = 0;
				}

				if(alsaCtlProperties->step) {
					// Round value to the nearest step
					convertedValue = (int) round((double) convertedValue / (double) alsaCtlProperties->step);
					convertedValue *= alsaCtlProperties->step;
				}

				convertedValueJ = json_object_new_int(convertedValue);
				break;

			case SND_CTL_ELEM_TYPE_BOOLEAN:
				if(! (json_object_is_type(toConvertObjectJ, json_type_int) ||
				      json_object_is_type(toConvertObjectJ, json_type_boolean))) {
					err += -1000*idx;
					AFB_ApiWarning(apiHandle,
						"%s: Try normalize a boolean control but value sent in json is not a boolean/integer : '%s'",
						__func__,
						json_object_get_string(toConvertObjectJ));
					convertedValueJ = toConvertObjectJ;
					break;
				}

				initialValue = json_object_get_int(toConvertObjectJ);
				convertedValueJ = json_object_new_int(initialValue ? 1 : 0);
				break;

			default:
				AFB_ApiWarning(apiHandle,
					       "%s: Normalization not handle for the alsa control type %i, sending back the object as it was '%s'",
					       __func__,
					       (int) alsaCtlProperties->type,
					       json_object_get_string(toConvertJ));
				convertedValueJ = toConvertObjectJ;
				break;
		}

		json_object_array_put_idx(convertedArrayJ, idx, convertedValueJ);
	}

	*ConvertedJ = convertedArrayJ;

	return 0;
}