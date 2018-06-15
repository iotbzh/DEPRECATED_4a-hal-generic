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

#ifndef LIB_MOST_VOLUME_H
#define	LIB_MOST_VOLUME_H

#include <stdint.h>


#ifdef	__cplusplus
extern "C" {
#endif

/** Describes the volume control */
enum lib_most_volume_channel_t{
    LIB_MOST_VOLUME_CH_FRONT_LEFT   = 0,
    LIB_MOST_VOLUME_CH_FRONT_RIGHT  = 1,
    LIB_MOST_VOLUME_CH_REAR_LEFT    = 2,
    LIB_MOST_VOLUME_CH_REAR_RIGHT   = 3,
    LIB_MOST_VOLUME_CH_CENTER       = 4,
    LIB_MOST_VOLUME_CH_SUB          = 5,
    LIB_MOST_VOLUME_MASTER          = 6
};

/** Is fired when the application shall call "lib_most_volume_service()" after a certain time
  * \param timeout  Time in ms after which the application shall call lib_most_volume_service().
  *                 Valid values:
  *                 0x0000: as soon as possible,
  *                 0x0001..0xFFFE: timeout in ms,
  *                 0xFFFF: never
  */
typedef void (*lib_most_volume_service_cb_t)(uint16_t timeout);

typedef void (*lib_most_volume_writei2c_result_cb_t)(uint8_t result, void *user_ptr);

/** Callback function required to write volume changes to network devices
  * \param node     The node address
  * \param data_ptr I2C command data
  * \param data_sz  Size of the I2C command data
  * \return         Returns \c 0 for success, other value on failure.
  */
typedef int (*lib_most_volume_writei2c_cb_t)(uint16_t node, uint8_t *data_ptr, uint8_t data_sz,
                                            lib_most_volume_writei2c_result_cb_t result_fptr,
                                            void *result_user_ptr);

typedef struct lib_most_volume_init_ {
    lib_most_volume_service_cb_t  service_cb;
    lib_most_volume_writei2c_cb_t writei2c_cb;

} lib_most_volume_init_t;


/** Initializes the library
  * \param   UNICENS_inst       Reference to the UNICENS instance, created by the application.
  * \param   req_service_fptr   Callback function which is fired if the application shall call
  *                             lib_most_volume_service.
  * \return  '0' on success, otherwise value >'0'.
  */
extern uint8_t lib_most_volume_init(lib_most_volume_init_t *init_ptr);

/** Terminates the library
  * \return  '0' on success, otherwise value >'0'.
  */
extern uint8_t lib_most_volume_exit(void);

/** Sets a single volume value.
  * \param   channel        The volume control to be set.
  * \param   volume         The volume value to be set. Valid values: 0..255.
  * \return  '0' on success, otherwise value >'0'.
  */
extern uint8_t lib_most_volume_set(enum lib_most_volume_channel_t channel, uint8_t volume);

/** Must be called when the availability of a node has changed
  * \param   address        The node address
  * \param   available      Availability: 0 - not available, 1 - available
  * \return  '0' on success, otherwise value >'0'.
  */
extern uint8_t lib_most_volume_node_available(uint16_t address, uint8_t available);

/** Shall be called either cyclically (e.g. 50ms -> polling) or after "timeout"
  * when "service_fptr" is fired (-> event triggered).
  * \return  '0' on success, otherwise value >'0'.
  */
extern uint8_t lib_most_volume_service(void);

#ifdef	__cplusplus
}
#endif

#endif	/* LIB_MOST_VOLUME_H */

