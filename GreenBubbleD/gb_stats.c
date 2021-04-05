/*
 * gb_stats.c:
 *	Configuration handling for GreenBubble project
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

#include <string.h>
#include <syslog.h>
#include <jansson.h>
#include <sys/time.h>

#include <gb_stats.h>
#include <gb_led.h>
#include <gb_main.h>
#include <gb_serial.h>

void cfg_big_json_test(gbCfg_t *cfg)
{
    int i;
    json_t *array_w = json_array();
    json_t *array_b = json_array();
    json_t *array_r = json_array();
    json_t *j_body;

    for (i = 0; i < 432; i++) { //1 per 10min for 3D
        json_array_append_new(array_w, json_pack("[i, i]", i, i+100));
        json_array_append_new(array_b, json_pack("[i, i]", i, i+200));
        json_array_append_new(array_r, json_pack("[i, i]", i, i+300));
    }


    j_body = json_pack("{s{sososo}}",
            "ld_spec",
                "white", array_w,
                "blue", array_b,
                "red", array_r);


    if (json_dump_file(j_body, "./BigChart_1x10mx3D.json", JSON_INDENT(4)) != 0)
        syslog(LOG_ERR, "Unable to save config.");

    json_decref(j_body);
    json_decref(array_w);
    json_decref(array_b);
    json_decref(array_r);
    return;
}

void gb_stats_init(gbSts_t *sts)
{
    int i;
    FOR_EACH_LED(i)
        sts->hist.intens[i] = json_array();

    sts->hist.humidity = json_array();
    sts->hist.rain = json_array();
    sts->hist.fog = json_array();
    sts->hist.tAir = json_array();
    sts->hist.tWater = json_array();
    sts->hist.vin = json_array();
    sts->hist.tPS = json_array();

    //TODO: could test if the above was OK.
    return;
}

void gb_stats_decref(gbSts_t *sts)
{
    int i;
    FOR_EACH_LED(i)
        json_decref(sts->hist.intens[i]);

    json_decref(sts->hist.humidity);
    json_decref(sts->hist.rain);
    json_decref(sts->hist.fog);
    json_decref(sts->hist.tAir);
    json_decref(sts->hist.tWater);
    json_decref(sts->hist.vin);
    json_decref(sts->hist.tPS);

    //TODO: could test if the above was OK.
    return;
}

static void hist_append(json_t *jarray, unsigned int value)
{
    static int apagar;
    long long time_ms;
    struct timeval te;
    json_t *elem = json_array(); //decrefing it was generating a problem, therefore using append_new
    
    apagar++;
    value = apagar;

    gettimeofday(&te, NULL); // get current time
    time_ms = te.tv_sec*1000LL + te.tv_usec/1000;
    time_ms += 2*3600*1000; //add 2hs summer timezone. It should be changed for smtg smarter...nothing worked.
    //debug("milliseconds: %lld\n", time_ms);

    json_array_append_new(elem, json_integer(time_ms));
    json_array_append_new(elem, json_integer(value)); 
    json_array_append_new(jarray, elem);

    //Check if oldest data is older than 3 Days. If yes, remove it.
    if(json_integer_value(json_array_get(json_array_get(jarray, 0), 0)) < (time_ms - 259200000))
        json_array_remove(jarray, 0);

    return;
}

#define STATUS_TIMER 600 //10min
void gb_get_status(gbSts_t *sts)
{
    int i;
    static int timer;

    timer += MAIN_LOOP_SEC;
    if (timer >= STATUS_TIMER) {
        //Get last data
        FOR_EACH_LED(i)        
            ld_get_status(i, &Gb_sts.ld_sts[i]);

        FOR_EACH_LED(i)        
            hist_append(sts->hist.intens[i], get_perc_from_curr(i, sts->ld_sts[i].cout));

        hist_append(sts->hist.vin, sts->ld_sts[LD_WHITE].vin);
        hist_append(sts->hist.humidity, sts->humidity_air);
        hist_append(sts->hist.rain, sts->rain ? 100 : 0);
        hist_append(sts->hist.fog, sts->fog ? 100 : 0);
        hist_append(sts->hist.tPS, sts->temp_PS);
        hist_append(sts->hist.tAir, sts->temp_air);
        hist_append(sts->hist.tWater, sts->temp_water);

        timer = 0;
    }

    return;
}
