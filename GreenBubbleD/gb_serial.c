/*
 * gb_serial.c:
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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <wiringPi.h>
#include <wiringSerial.h>

#include "gb_serial.h"

int Fd;
char Rd_buffer[512];

static void ld_select_driver(ldBoard_t color)
{
    switch (color) {
        case LD_WHITE:
            digitalWrite(GPIO_17, LOW);
            digitalWrite(GPIO_18, LOW);
            break;
        case LD_BLUE:
            digitalWrite(GPIO_17, LOW);
            digitalWrite(GPIO_18, HIGH);
            break;
        case LD_RED:
            digitalWrite(GPIO_17, HIGH);
            digitalWrite(GPIO_18, LOW);
            break;
    }
}

static int ld_read_feedback()
{
    unsigned int start = millis();
    int i = 0;
    
    while ((millis() - start) < 10) {
        if (serialDataAvail(Fd)) {
             Rd_buffer[i] = serialGetchar(Fd);
             if ((i >= 2) && (strncmp((Rd_buffer + i-2), "OK\n", 3) == 0))
                 break;
             if ((i >= 2) && (strncmp((Rd_buffer + i-2), "E!\n", 3) == 0)) {
                 errno = EIO;
                 break;
             }
             i++;
        }
    } 
    if ((errno != 0) || (errno != EIO))
        errno = EBADMSG;

    Rd_buffer[i+1]='\0';
    serialFlush(Fd);

    if (errno) {
        fprintf (stderr, "Unable to complete serial reading: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static int ld_onoff2bool(char *str_sts, bool *bool_sts)
{
    if (strncmp(str_sts, "ON", 2) == 0)
        *bool_sts = TRUE;
    else if (strncmp(str_sts, "OFF", 3) == 0)
        *bool_sts = FALSE;
    else {
        errno = EINVAL;
        return -1;
    }

    return 0;
}

int ld_get_system(ldBoard_t color, ldSys_t *sys)
{
    char sts1[5], sts2[5];

    // Select the Board and send the command
    ld_select_driver(color);
	serialPrintf(Fd, "SYSTEM\n");

	// Get the message
    if (ld_read_feedback())
        return -1;

    // Parse the result
    if (sscanf(Rd_buffer, "M: %s\r\nV: %s\r\nN: %s\r\nO: %s\r\nAC: %s\r\n", sys->model, sys->version, sys->name, sts1, sts2) == EOF)
        return -1;
    
    if (ld_onoff2bool(sts1, &sys->default_on)) return -1;
    if (ld_onoff2bool(sts2, &sys->autocommit)) return -1;

    return 0;
}

int ld_get_config(ldBoard_t color, ldSys_t *sys, ldCfg_t *cfg)
{
    char sts1[5];

    // Select the Board and send the command
    ld_select_driver(color);
	serialPrintf(Fd, "CONFIG\n");

	// Get the message
    if (ld_read_feedback())
        return -1;

    // Parse the result
    if (sscanf(Rd_buffer, "OUTPUT: %s\r\nVSET: %f\r\nCSET: %f\r\n", sts1, &cfg->vset, &cfg->cset) == EOF)
        return -1;
    
    if (ld_onoff2bool(sts1, &sys->output)) return -1;

    return 0;
}

int ld_get_status(ldBoard_t color, ldSts_t *sts, ldSys_t *sys)
{
    char sts1[5], sts2[10];

    // Select the Board and send the command
    ld_select_driver(color);
	serialPrintf(Fd, "STATUS\n");

	// Get the message
    if (ld_read_feedback())
        return -1;

    // Parse the result
    if (sscanf(Rd_buffer, "OUTPUT: %s\r\nVIN: %f %u\r\nVOUT: %f %u\r\nCOUT: %f %u\r\nCONSTANT: %s\r\n",
            sts1, &sts->vin, &sts->vin_raw, &sts->vout, &sts->vout_raw, &sts->cout, &sts->cout_raw, sts2) == EOF)
        return -1;
    
    if (ld_onoff2bool(sts1, &sys->output)) return -1;

    if (strncmp(sts2, "VOLTAGE", 7) == 0)
        sts->constant_current = FALSE;
    else if (strncmp(sts2, "CURRENT", 7) == 0)
        sts->constant_current = TRUE;
    else {
        errno = EIO;
        return -1;
    }

    return 0;
}

int ld_set_voltage(ldBoard_t color, unsigned int voltage)
{
    // Select the Board and send the command
    ld_select_driver(color);
	serialPrintf(Fd, "VOLTAGE %u\n", voltage);

	// Get the message
    if (ld_read_feedback())
        return -1;

    return 0;
}

int ld_set_current(ldBoard_t color, unsigned int current)
{
    // Select the Board and send the command
    ld_select_driver(color);
	serialPrintf(Fd, "CURRENT %u\n", current);

	// Get the message
    if (ld_read_feedback())
        return -1;

    return 0;
}

int ld_set_output(ldBoard_t color, bool output)
{
    // Select the Board and send the command
    ld_select_driver(color);
	serialPrintf(Fd, "OUTPUT %b\n", output);

	// Get the message
    if (ld_read_feedback())
        return -1;

    return 0;
}

int ld_serial_init()
{
	Fd = serialOpen("/dev/ttyAMA0", 38400);
	if (Fd < 0)
		fprintf (stderr, "Unable to open serial device: %s\n", strerror(errno));
		
	return Fd;
}
