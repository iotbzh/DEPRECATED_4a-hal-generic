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
#include <stddef.h>
#include "setup.h"

CSetup* CSetup::_instance = NULL;

// singleton
CSetup* CSetup::GetInstance() {
    if (_instance == NULL) {
        _instance = new CSetup();
    }

    return _instance;
}

// singleton
void CSetup::Release() {
    if (_instance != NULL) {
        delete _instance;
    }
    _instance = NULL;
}

CSetup::CSetup()
    :   _volume_amp_270_m(0x270U, DEVICE_VAL_MASTER, LIB_MOST_VOLUME_MASTER),
        _volume_amp_270_l(0x270U, DEVICE_VAL_LEFT, LIB_MOST_VOLUME_CH_FRONT_LEFT),
        _volume_amp_270_r(0x270U, DEVICE_VAL_RIGHT, LIB_MOST_VOLUME_CH_FRONT_RIGHT),
        _volume_amp_271_m(0x271U, DEVICE_VAL_MASTER, LIB_MOST_VOLUME_MASTER),
        _volume_amp_271_l(0x271U, DEVICE_VAL_LEFT, LIB_MOST_VOLUME_CH_REAR_LEFT),
        _volume_amp_271_r(0x271U, DEVICE_VAL_RIGHT, LIB_MOST_VOLUME_CH_REAR_RIGHT),
        _volume_amp_272_m(0x272U, DEVICE_VAL_MASTER, LIB_MOST_VOLUME_MASTER),
        _volume_amp_272_l(0x272U, DEVICE_VAL_LEFT, LIB_MOST_VOLUME_CH_CENTER),
        _volume_amp_272_r(0x272U, DEVICE_VAL_RIGHT, LIB_MOST_VOLUME_CH_SUB),
        _value_container()
{
    static CDeviceValue* value_list[9] = {  &_volume_amp_270_m,
                                            &_volume_amp_270_l,
                                            &_volume_amp_270_r,
                                            &_volume_amp_271_m,
                                            &_volume_amp_271_l,
                                            &_volume_amp_271_r,
                                            &_volume_amp_272_m,
                                            &_volume_amp_272_l,
                                            &_volume_amp_272_r};

    _value_container.RegisterValues(value_list, 9U);
}

CSetup::~CSetup()
{

}

void CSetup::Configure(lib_most_volume_init_t *init_ptr)
{
    this->init_data = *init_ptr;
    _value_container.AssignService(&this->init_data);
}

void CSetup::SetVolume(enum lib_most_volume_channel_t channel, uint8_t volume)
{
    _value_container.SetValue((uint16_t)channel, volume);
}

void CSetup::Update()
{
    _value_container.Update();
}

void CSetup::SetNodeAvailable(uint16_t address, bool available)
{
    _value_container.ChangeNodeAvailable(address, available);
}
