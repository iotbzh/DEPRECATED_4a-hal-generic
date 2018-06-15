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
#include "device_value.h"
#include "setup.h"

#define MUTE_VALUE      0x03FFU
#define MUTE_VALUE_HB   0x03U
#define MUTE_VALUE_LB   0xFFU

#define CONTROL_MASTER  0x07U
#define CONTROL_CH_1    0x08U
#define CONTROL_CH_2    0x09U

CDeviceValue::CDeviceValue(uint16_t address, DeviceValueType type, uint16_t key)
{
    this->_is_available = false;
    this->_is_busy = false;
    this->_address = address;
    this->_target_value = 0x01u;
    this->_actual_value = 0x01u;

    this->_result_fptr = NULL;
    this->_result_user_ptr = NULL;

    this->_type = type;
    this->_key = key;

    _tx_payload[0] = CONTROL_MASTER;// 7: master, 8: channel 1, 9: Channel 2
    _tx_payload[1] = MUTE_VALUE_HB; //HB:Volume
    _tx_payload[2] = MUTE_VALUE_LB; //LB:Volume
    _tx_payload_sz = 3u;
}

CDeviceValue::~CDeviceValue()
{
}

void CDeviceValue::ApplyMostValue(uint8_t value, DeviceValueType type, uint8_t tx_payload[])
{
    uint16_t tmp = MUTE_VALUE;

    switch (type)
    {
        case DEVICE_VAL_LEFT:
            tmp = (uint16_t)(0x80U + 0x37FU - (0x37FU * ((int32_t)value) / (0xFFU)));
            //tmp = 0x3FF - (0x3FF * ((int32_t)value) / (0xFF));
            //tmp = 0x100 + 0x2FF - (0x2FF * ((int32_t)value) / (0xFF));
            tx_payload[0] = CONTROL_CH_1;
            break;
        case DEVICE_VAL_RIGHT:
            tmp = (uint16_t)(0x80U + 0x37FU - (0x37FU * ((int32_t)value) / (0xFFU)));
            //tmp = 0x3FF - (0x3FF * ((int32_t)value) / (0xFF));
            //tmp = 0x100 + 0x2FF - (0x2FF * ((int32_t)value) / (0xFF));
            tx_payload[0] = CONTROL_CH_2;
            break;
        default:
            /*std::cerr << "CDeviceValue::ApplyMostValue() error matching incorrect" << std::endl;*/
        case DEVICE_VAL_MASTER:
            tmp = (uint16_t)(0x100U + 0x2FFU - (0x2FFU * ((int32_t)value) / (0xFFU)));
            tx_payload[0] = CONTROL_MASTER;
            break;
    }

    tx_payload[1] = (uint8_t)((tmp >> 8U) & (uint16_t)0xFFU); //HB:Volume
    tx_payload[2] = (uint8_t)(tmp  & (uint16_t)0xFFU); //LB:Volume
}

// returns true if target is not actual value
bool CDeviceValue::RequiresUpdate()
{
    if (this->_is_available && !this->_is_busy && (this->_target_value != this->_actual_value))
    {
        return true;
    }

    return false;
}

bool CDeviceValue::FireUpdateMessage(lib_most_volume_writei2c_cb_t writei2c_fptr,
                                     lib_most_volume_writei2c_result_cb_t result_fptr,
                                     void *result_user_ptr)
{
    int ret = -1;
    ApplyMostValue(this->_target_value, _type, _tx_payload);

    if (this->_is_available && !this->_is_busy)
    {
        ret = writei2c_fptr(this->_address, &_tx_payload[0], _tx_payload_sz,
                            &OnI2cResult,
                            this);

        if (ret == 0)
        {
            this->_transmitted_value = this->_target_value;
            this->_is_busy = true;
            this->_result_fptr = result_fptr;
            this->_result_user_ptr = result_user_ptr;
            return true;
        }
    }

    return false;
}

void CDeviceValue::HandleI2cResult(uint8_t result)
{
    if (result == 0)
    {
        /* transmission succeeded - now apply transmitted value */
        this->_actual_value = this->_transmitted_value;
    }

    if (this->_result_fptr)
    {
        /* notify container */
        this->_result_fptr(result, this->_result_user_ptr);
    }

    this->_result_fptr = NULL;
    this->_result_user_ptr = NULL;
    this->_is_busy = false;
}

void CDeviceValue::OnI2cResult(uint8_t result, void *obj_ptr)
{
    ((CDeviceValue*)obj_ptr)->HandleI2cResult(result);
}
