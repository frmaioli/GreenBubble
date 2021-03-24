/*
 * gb_config.c:
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
#include <jansson.h>

#include <gb_config.h>
#include <gb_led.h>
#include <gb_main.h>

static void cfg_load_dflt(gbCfg_t *cfg)
{
    cfg->ld_instant_mode = false;

    cfg->ld_instant[LD_WHITE].enable = false;    
    cfg->ld_instant[LD_WHITE].vset = 120000;    //120V for BST900. We will limit in current
    cfg->ld_instant[LD_WHITE].cset = 100;       //0.1A

    cfg->ld_instant[LD_BLUE].enable = false;
    cfg->ld_instant[LD_BLUE].vset = 20000;      //20V
    cfg->ld_instant[LD_BLUE].cset = 100;        //0.1A

    cfg->ld_instant[LD_RED].enable = false;
    cfg->ld_instant[LD_RED].vset = 20000;       //20V
    cfg->ld_instant[LD_RED].cset = 100;         //0.1A

    {
        // (0 and 24hs will always repeat the number)
        unsigned char tmp[LD_NUMB][ROUT_STEP] = {
                //0hs, 3hs, 6hs, 9hs, 12hs, 15hs, 18hs, 21hs, 24hs
                { 0,   0,   0,   75,  100,  100,  75,   0,    0 }, //White
                { 0,   0,   0,   80,   40,   10,   0,   3,    0 }, //Blue
                { 0,   0,   0,    5,   15,   40,  80,   0,    0 }  //Red
                };
        memcpy(cfg->ld_spec, tmp, sizeof(cfg->ld_spec));
    }

    // Generate the numbers in between for ld_routine_perc
    // It will also set ld_routine_init to true
    ld_generate_points();

    return;
}

/*Modify a bit: I would create 2 functions:
1. cfg_load: check if there is a file with cfg, if yes, load it into cfg, otherwise load dflt cfg. Or load 1st dflt and after the file to ensure all fiels are filled.
2. cfg_apply: This would apply the CFG
3. cfg_save: would save the CFG into the file */

void cfg_load(gbCfg_t *cfg)
{
    //First Load the Dflt Cfg to Ensure all filds have a cfg
    cfg_load_dflt(cfg);
}

void cfg_save(gbCfg_t *cfg)
{
    int i, result;
    json_t *array_w = json_array();
    json_t *array_b = json_array();
    json_t *array_r = json_array();
    json_t *j_body;

    for (i = 0; i < ROUT_STEP; i++) {
        json_array_append_new(array_w, json_integer(cfg->ld_spec[LD_WHITE][i]));
        json_array_append_new(array_b, json_integer(cfg->ld_spec[LD_BLUE][i]));
        json_array_append_new(array_r, json_integer(cfg->ld_spec[LD_RED][i]));
    }

    j_body = json_pack("{sbs{sbsisi}s{sbsisi}s{sbsisi}s[o,o,o]}",
            "ld_instant_mode", cfg->ld_instant_mode,
            "ld_instant_white",
                "enable", cfg->ld_instant[LD_WHITE].enable,
                "vset", cfg->ld_instant[LD_WHITE].vset,
                "cset", cfg->ld_instant[LD_WHITE].cset, 
            "ld_instant_blue",
                "enable", cfg->ld_instant[LD_BLUE].enable,
                "vset", cfg->ld_instant[LD_BLUE].vset,
                "cset", cfg->ld_instant[LD_BLUE].cset, 
            "ld_instant_red",
                "enable", cfg->ld_instant[LD_RED].enable,
                "vset", cfg->ld_instant[LD_RED].vset,
                "cset", cfg->ld_instant[LD_RED].cset,
            "ld_spec", array_w, array_b, array_r);


    result = json_dump_file(j_body, "/home/pi/Developer/GreenBubbleD/CFG.json", 0);
    if (result != 0)
        debug("json_dump_file failed");

    json_decref(j_body);
}
