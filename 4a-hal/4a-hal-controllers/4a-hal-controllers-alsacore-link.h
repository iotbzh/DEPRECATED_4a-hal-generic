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

#ifndef _HAL_CTLS_ALSACORE_LINK_INCLUDE_
#define _HAL_CTLS_ALSACORE_LINK_INCLUDE_

#include <stdio.h>

#include <wrap-json.h>

#include <alsa/asoundlib.h>

#include <ctl-config.h>

#define ALSACORE_API			"alsacore"
#define ALSACORE_SUBSCRIBE_VERB		"subscribe"
#define ALSACORE_GETINFO_VERB		"infoget"
#define ALSACORE_CTLGET_VERB		"ctlget"
#define ALSACORE_CTLSET_VERB		"ctlset"
#define ALSACORE_ADDCTL_VERB		"addcustomctl"

struct CtlHalAlsaDBScale {
	int min;
	int max;
	int step;
	int mute;
};

struct CtlHalAlsaCtlProperties {
	snd_ctl_elem_type_t type;
	int count;
	int minval;
	int maxval;
	int step;
	// TBD JAI : use them
	const char **enums;
	struct CtlHalAlsaDBScale *dbscale;
};

struct CtlHalAlsaCtl {
	char *name;
	int numid;
	int value;
	struct CtlHalAlsaCtlProperties alsaCtlProperties;
	struct CtlHalAlsaCtlProperties *alsaCtlCreation;
};

struct CtlHalAlsaMap {
	const char *uid;
	char *info;
	struct CtlHalAlsaCtl ctl;
	json_object *actionJ;
	CtlActionT *action;
};

struct CtlHalAlsaMapT {
	struct CtlHalAlsaMap *ctls;
	unsigned int ctlsCount;
};

// Alsa control types map from string function
snd_ctl_elem_type_t HalCtlsMapsAlsaTypeToEnum(const char *label);

// Free contents of 'CtlHalAlsaMapT' data structure
uint8_t HalCtlsFreeAlsaCtlsMap(struct CtlHalAlsaMapT *alsaCtlsMap);

// HAL controllers alsacore calls funtions
int HalCtlsGetCardIdByCardPath(AFB_ApiT apiHandle, char *devPath);
int HalCtlsSubscribeToAlsaCardEvent(AFB_ApiT apiHandle, char *cardId);
int HalCtlsGetAlsaCtlInfo(AFB_ApiT apiHandle, char *cardId, struct CtlHalAlsaCtl *currentAlsaCtl);
int HalCtlsSetAlsaCtlValue(AFB_ApiT apiHandle, char *cardId, int ctlId, json_object *valuesJ);
int HalCtlsCreateAlsaCtl(AFB_ApiT apiHandle, char *cardId, struct CtlHalAlsaCtl *alsaCtlToCreate);

// HAL controllers alsacore controls request callback
void HalCtlsActionOnAlsaCtl(AFB_ReqT request);

#endif /* _HAL_CTLS_ALSACORE_LINK_INCLUDE_ */