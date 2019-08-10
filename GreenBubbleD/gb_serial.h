/*
 * gb_serial.h:
 *	Serial instructions to communicate with the Led Driveres (ld) for the GreenBubble project
 *
 * Copyright (c) 2018-2019 Fabiano R. Maioli <frmaioli@gmail.com>
 ***********************************************************************
 *    GreenBubble is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    GreenBubble is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with GreenBubble.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#ifndef GB_SERIAL_H
#define GB_SERIAL_H

#include "gb_main.h"

typedef enum {
    LD_WHITE = 1,
    LD_BLUE,
    LD_RED
} ldBoard_t;

int ld_get_system(ldBoard_t color, ldSys_t *sys);
int ld_get_config(ldBoard_t color, ldSys_t *sys, ldCfg_t *cfg);
int ld_get_status(ldBoard_t color, ldSts_t *sts, ldSys_t *sys);
int ld_set_voltage(ldBoard_t color, unsigned int voltage);
int ld_set_current(ldBoard_t color, unsigned int current);
int ld_set_output(ldBoard_t color, bool output);
int ld_serial_init();

#endif //GB_SERIAL_H
