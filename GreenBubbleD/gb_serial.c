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
#include "gb_main.h"
#include "gb_gpio.h"

#define CHECK(x) if ((Fd < 0) || (x >= LD_NUMB) || (Gb_ld_sys[x].device_ok == false)) return -1

int Fd = -1;
char Rd_buffer[512];
char Driver[17];

static void ld_select_driver(ldBoard_t color)
{
    switch (color) {
        case LD_WHITE:
            digitalWrite(BCM_22, LOW);
            digitalWrite(BCM_23, LOW);
            strcpy(Driver, "LED_WHITE");
            break;
        case LD_BLUE:
            digitalWrite(BCM_22, LOW);
            digitalWrite(BCM_23, HIGH);
            strcpy(Driver, "LED_BLUE");
            break;
        case LD_RED:
            digitalWrite(BCM_22, HIGH);
            digitalWrite(BCM_23, LOW);
            strcpy(Driver, "LED_RED");
            break;
    }
    serialFlush(Fd);
    return;
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
                 errno = EBADMSG;
                 break;
             }
             i++;
        }
    } 
    if ((errno != 0) || (errno != EBADMSG))
        errno = EIO;

    Rd_buffer[i+1]='\0';
    serialFlush(Fd);

    if (errno) {
        fprintf (stderr, "%s: Unable to complete serial reading: %s\n", Driver, strerror(errno));
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
        fprintf (stderr, "%s: Error reading ON/OFF parameter: %s\n", Driver, str_sts);
        return -1;
    }

    return 0;
}

int ld_get_system(ldBoard_t color, ldSys_t *sys)
{
    char sts1[5], sts2[5];

    if ((Fd < 0) || (color >= LD_NUMB)) return -1; //Do not use check macro here

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

int ld_get_config(ldBoard_t color, ldCfg_t *cfg)
{
    char sts1[5];
    float fv, fc;

    CHECK(color);
    
    // Select the Board and send the command
    ld_select_driver(color);
    serialPrintf(Fd, "CONFIG\n");

    // Get the message
    if (ld_read_feedback())
        return -1;

    // Parse the result
    if (sscanf(Rd_buffer, "OUTPUT: %s\r\nVSET: %f\r\nCSET: %f\r\n", sts1, &fv, &fc) == EOF)
        return -1;
    
    cfg->vset = (unsigned int)(fv*1000); //convert to mV
    cfg->cset = (unsigned int)(fc*1000); //convert to mA

    if (ld_onoff2bool(sts1, &cfg->enable)) return -1;

    return 0;
}

int ld_get_status(ldBoard_t color, ldSts_t *sts)
{
    char sts1[5], sts2[10];
    float fvi, fv, fc;

    CHECK(color);
    
    // Select the Board and send the command
    ld_select_driver(color);
    serialPrintf(Fd, "STATUS\n");

    // Get the message
    if (ld_read_feedback())
        return -1;

    // Parse the result
    if (sscanf(Rd_buffer, "OUTPUT: %s\r\nVIN: %f %u\r\nVOUT: %f %u\r\nCOUT: %f %u\r\nCONSTANT: %s\r\n",
            sts1, &fvi, &sts->vin_raw, &fv, &sts->vout_raw, &fc, &sts->cout_raw, sts2) == EOF)
        return -1;

    sts->vin = (unsigned int)(fvi*1000); //convert to mV
    sts->vout = (unsigned int)(fv*1000); //convert to mV
    sts->cout = (unsigned int)(fc*1000); //convert to mA

    if (ld_onoff2bool(sts1, &sts->enable)) return -1;

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
    float fv;

    CHECK(color);

    if (voltage > Gb_ld_sys[color].max_volt)
        voltage = Gb_ld_sys[color].max_volt;

    if (voltage < Gb_ld_sys[color].min_volt)
        voltage = Gb_ld_sys[color].min_volt;

    fv = ((float)(voltage))/1000; //convert to 1.23 format

    // Select the Board and send the command
    ld_select_driver(color);
    serialPrintf(Fd, "VOLTAGE %.2f\n", fv);

    // Get the message
    if (ld_read_feedback())
        return -1;

    return 0;
}

int ld_set_current(ldBoard_t color, unsigned int current)
{
    float fc;

    CHECK(color);

    if (current > Gb_ld_sys[color].fwd_led_curr)
        current = Gb_ld_sys[color].fwd_led_curr;

    fc = ((float)(current))/1000; //convert to 1.23 format

    // Select the Board and send the command
    ld_select_driver(color);
    serialPrintf(Fd, "CURRENT %.2f\n", fc);

    // Get the message
    if (ld_read_feedback())
        return -1;

    return 0;
}

int ld_set_output(ldBoard_t color, bool output)
{
    CHECK(color);
    
    // Select the Board and send the command
    ld_select_driver(color);
    serialPrintf(Fd, "OUTPUT %b\n", output);

    // Get the message
    if (ld_read_feedback())
        return -1;

    return 0;
}

unsigned int get_curr_from_perc(ldBoard_t color, unsigned char perc)
{
    unsigned int curr;

    if (color >= LD_NUMB) return 0;
    
    curr = (Gb_ld_sys[color].fwd_led_curr * perc)/100;
    if (curr > Gb_ld_sys[color].fwd_led_curr)
       return Gb_ld_sys[color].fwd_led_curr;
    else
       return curr; 
}

unsigned char get_perc_from_curr(ldBoard_t color, unsigned int curr)
{
    if (color >= LD_NUMB) return 0;
    return (unsigned char) ((curr*100)/(Gb_ld_sys[color].fwd_led_curr));
}

int ld_serial_init()
{
	Fd = serialOpen("/dev/ttyAMA0", 38400);
	if (Fd < 0)
		fprintf (stderr, "Unable to open serial device: %s\n", strerror(errno));
		
	return Fd;
}
