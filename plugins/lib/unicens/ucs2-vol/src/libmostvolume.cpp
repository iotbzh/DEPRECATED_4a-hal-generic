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

#include "libmostvolume.h"
#include "setup.h"
/*#include <iostream>*/

static bool _running = false;

extern "C" uint8_t lib_most_volume_init(lib_most_volume_init_t *init_ptr)
{
    uint8_t success = 1U;
    /*std::cerr << "lib_most_volume_init(): called" << std::endl;*/

    if (!_running && init_ptr)
    {
        CSetup::GetInstance()->Configure(init_ptr);
        success = 0U;
        _running = true;
    }

    return success;
}

extern "C" uint8_t lib_most_volume_exit(void)
{
    uint8_t success = 1U;
    /*std::cerr << "lib_most_volume_exit(): called" << std::endl;*/

    if (_running)
    {
        CSetup::Release();
        success = 0U;
        _running = false;
    }

    return success;
}

extern "C" uint8_t lib_most_volume_set(enum lib_most_volume_channel_t channel, uint8_t volume)
{
    uint8_t success = 1U;
    /*std::cerr << "lib_most_volume_set(): channel=" << channel << ", volume=" << (int)volume << std::endl;*/

    if (_running)
    {
        CSetup::GetInstance()->SetVolume(channel, volume);
        success = 0U;
    }

    return success;
}

extern uint8_t lib_most_volume_node_available(uint16_t address, uint8_t available)
{
    uint8_t success = 1U;

    if (_running)
    {
        CSetup::GetInstance()->SetNodeAvailable(address, available);
        success = 0U;
    }

    return success;
}

extern "C" uint8_t lib_most_volume_service(void)
{
    uint8_t success = 1U;
    /*std::cerr << "lib_most_volume_service(): called" << std::endl;*/

    if (_running)
    {
        CSetup::GetInstance()->Update();
        success = 0U;
    }

    return success;
}
