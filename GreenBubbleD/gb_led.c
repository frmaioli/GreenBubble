/*
 * gb_led.c:
 *	General Led routines for the GreenBubble project
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

#include <syslog.h>
#include <string.h>
#include <time.h>

#include <gb_main.h>
#include <gb_serial.h>


/***************** INITS *******************/

int ld_sys_init(void)
{
    int ret = 0;

    // White: OSRAM GW CSSRM2.CM-M5M7-XX51-1
    Gb_ld_sys[LD_WHITE].fwd_led_curr = 700; //mA
    Gb_ld_sys[LD_WHITE].fwd_led_volt = 2800; //mV
    Gb_ld_sys[LD_WHITE].numb_leds = 36;
    Gb_ld_sys[LD_WHITE].wave_length = 6500; //kelvin
    
    // Blue: OSRAM GD CS8PM1.14-UOVJ-W4-1
    Gb_ld_sys[LD_BLUE].fwd_led_curr = 350; //mA
    Gb_ld_sys[LD_BLUE].fwd_led_volt = 2850; //mV
    Gb_ld_sys[LD_BLUE].numb_leds = 6;
    Gb_ld_sys[LD_BLUE].wave_length = 451; //nm

    // Red: OSRAM GH CS8PM1.24-4T2U-1
    Gb_ld_sys[LD_RED].fwd_led_curr = 350; //mA
    Gb_ld_sys[LD_RED].fwd_led_volt = 2150; //mV
    Gb_ld_sys[LD_RED].numb_leds = 6;
    Gb_ld_sys[LD_RED].wave_length = 660; //nm

    // WHITE: Get System and Confirm we have a correct color-device match
    if (ld_get_system(LD_WHITE, &Gb_ld_sys[LD_WHITE]) != 0) {
        Gb_ld_sys[LD_WHITE].device_ok = false;
        ret = -1;
    } else if ((strncmp(Gb_ld_sys[LD_WHITE].model, "BST900", 6) != 0) ||
                (strncmp(Gb_ld_sys[LD_WHITE].name, "LED_WHITE", 9) != 0)) {
        syslog(LOG_CRIT, "White Led Device's model or name does not match with the expected.");
        Gb_ld_sys[LD_WHITE].device_ok = false;
        ret = -2;
    } else {
        Gb_ld_sys[LD_WHITE].device_ok = true;
    }

    // BLUE: Get System and Confirm we have a correct color-device match
    if (ld_get_system(LD_BLUE, &Gb_ld_sys[LD_BLUE]) != 0) {
        Gb_ld_sys[LD_BLUE].device_ok = false;
        ret = -1;
    } else if ((strncmp(Gb_ld_sys[LD_BLUE].model, "B6303", 5) != 0) ||
                (strncmp(Gb_ld_sys[LD_BLUE].name, "LED_BLUE", 8) != 0)) {
        syslog(LOG_CRIT, "Blue Led Device's model or name does not match with the expected.");
        Gb_ld_sys[LD_BLUE].device_ok = false;
        ret = -2;
    } else {
        Gb_ld_sys[LD_BLUE].device_ok = true;
    }

    // RED: Get System and Confirm we have a correct color-device match
    if (ld_get_system(LD_RED, &Gb_ld_sys[LD_RED]) != 0) {
        Gb_ld_sys[LD_RED].device_ok = false;
        ret = -1;
    } else if ((strncmp(Gb_ld_sys[LD_RED].model, "B6303", 5) != 0) ||
                (strncmp(Gb_ld_sys[LD_RED].name, "LED_RED", 7) != 0)) {
        syslog(LOG_CRIT, "Red Led Device's model or name does not match with the expected.");
        Gb_ld_sys[LD_RED].device_ok = false;
        ret = -2;
    } else {
        Gb_ld_sys[LD_RED].device_ok = true;
    }

    return ret;
}

/***************** FUNCT *******************/

void ld_daily_routine(bool update_now)
{
    time_t t = time(NULL);
    struct tm tmt = *localtime(&t);
    int mins_of_day = tmt.tm_hour*60 + tmt.tm_min;
    int ret = 0;
    unsigned char i = 0;
    static int latest_mins = -1;

    //Only run the routine if we are in this mode
    if (Gb_cfg.ld_instant_mode == true)
        return;

    //Check if routine was generated
    if (Gb_cfg.ld_routine_init == false)
        return;

    //Update lights each TIME_LD min
    if(((latest_mins != mins_of_day) & (mins_of_day%TIME_LD == 0)) || update_now) {
  
        //Get the value to be applied for this time and limit to 100% for protection
        i = mins_of_day/TIME_LD;
        if (i > ROUT_TOT) {
           ret |= 1;
           i = ROUT_TOT;
        }

  //      if (ld_set_current(LD_WHITE, get_curr_from_perc(LD_WHITE, Gb_cfg.ld_routine_perc[LD_WHITE][i])) < 0)
    //        ret |= 2;
        
      //  if (ld_set_current(LD_BLUE, get_curr_from_perc(LD_BLUE, Gb_cfg.ld_routine_perc[LD_BLUE][i])) < 0)
//            ret |= 4;
        
  //      if (ld_set_current(LD_RED, get_curr_from_perc(LD_RED, Gb_cfg.ld_routine_perc[LD_RED][i])) < 0)
    //        ret |= 8;
        
        debug("Led Routine set intensity to: [%i] W%i B%i R%i.\n", i,
                Gb_cfg.ld_routine_perc[LD_WHITE][i],
                Gb_cfg.ld_routine_perc[LD_BLUE][i],
                Gb_cfg.ld_routine_perc[LD_RED][i]);
        syslog(LOG_INFO, "Led Routine set intensity to: [%i] W%i B%i R%i.", i,
                ((ret && 2) ? -1 : Gb_cfg.ld_routine_perc[LD_WHITE][i]),
                ((ret && 4) ? -1 : Gb_cfg.ld_routine_perc[LD_BLUE][i]),
                ((ret && 8) ? -1 : Gb_cfg.ld_routine_perc[LD_RED][i]));
        if(ret)
            syslog(LOG_ERR, "Error setting routine led intensity: %i", ret);
        
        latest_mins = mins_of_day;
    }
    
    return;
}

void ld_generate_points(void)
{
    unsigned char color, s, n;
    int point;
    float ystep, curve;

    for (color = 0; color < LD_NUMB; color++) {
        point = -1;
        debug("\n\n");
        for (s = 1; s < ROUT_STEP; s++) {
            point++;
            ystep = ((float)Gb_cfg.ld_spec[color][s] - (float)Gb_cfg.ld_spec[color][s-1])/ROUT_N;
            curve = Gb_cfg.ld_spec[color][s-1];
            Gb_cfg.ld_routine_perc[color][point] = curve;
            debug("[%i] %d, ", point, Gb_cfg.ld_routine_perc[color][point]);
            for (n = 1; n < ROUT_N; n++) {
                point++;
                curve += ystep;
                Gb_cfg.ld_routine_perc[color][point] = curve;  
                debug("[%i] %d, ", point, Gb_cfg.ld_routine_perc[color][point]);
            }
        }
    }
    debug("\n\n");
    
    Gb_cfg.ld_routine_init = true;
    return;
}


