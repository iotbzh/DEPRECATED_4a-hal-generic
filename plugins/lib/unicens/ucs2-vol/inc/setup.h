/*
 * libmostvolume example
 *
 * Copyright (C) 2017 Microchip Technology Germany II GmbH & Co. KG
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * You may also obtain this software under a propriety license from Microchip.
 * Please contact Microchip for further information.
 *
 */

#ifndef SETUP_H
#define	SETUP_H

#include "libmostvolume.h"
#include "device_value.h"
#include "device_container.h"

#define MAX_CHANNELS    (LIB_MOST_VOLUME_MASTER + 1u)
#define MAX_MASTERS     3u

class CSetup {

public:
    void Configure(lib_most_volume_init_t *init_ptr);
    void SetVolume(enum lib_most_volume_channel_t channel, uint8_t volume);
    void Update();
    void SetNodeAvailable(uint16_t address, bool available);

private:
    CDeviceValue _volume_amp_270_m;
    CDeviceValue _volume_amp_270_l;
    CDeviceValue _volume_amp_270_r;

    CDeviceValue _volume_amp_271_m;
    CDeviceValue _volume_amp_271_l;
    CDeviceValue _volume_amp_271_r;

    CDeviceValue _volume_amp_272_m;
    CDeviceValue _volume_amp_272_l;
    CDeviceValue _volume_amp_272_r;

    CDeviceContainer _value_container;
    lib_most_volume_init_t init_data;

public:
    static CSetup* GetInstance();   // singleton
    static void Release();          // singleton

protected:
    CSetup();                       // singleton
    virtual ~CSetup();              // singleton

private:
    static CSetup* _instance;       // singleton
};

#endif	/* SETUP_H */

