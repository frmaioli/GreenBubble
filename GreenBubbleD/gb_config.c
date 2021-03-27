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
#include <syslog.h>
#include <jansson.h>

#include <gb_config.h>
#include <gb_led.h>
#include <gb_main.h>
#include <gb_serial.h>

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

static void cfg_print(gbCfg_t *cfg)
{
    int i;

    debug("CFG LOADED:\n");
    debug("    ld_instant_mode: %d\n", cfg->ld_instant_mode);
    debug("    ld_instant:\n");
    debug("        white:\n");
    debug("            enable: %d\n", cfg->ld_instant[LD_WHITE].enable);
    debug("            vset: %u\n", cfg->ld_instant[LD_WHITE].vset);
    debug("            cset: %u\n", cfg->ld_instant[LD_WHITE].cset);
    debug("        blue:\n");
    debug("            enable: %d\n", cfg->ld_instant[LD_BLUE].enable);
    debug("            vset: %u\n", cfg->ld_instant[LD_BLUE].vset);
    debug("            cset: %u\n", cfg->ld_instant[LD_BLUE].cset);
    debug("        red:\n");
    debug("            enable: %d\n", cfg->ld_instant[LD_RED].enable);
    debug("            vset: %u\n", cfg->ld_instant[LD_RED].vset);
    debug("            cset: %u\n", cfg->ld_instant[LD_RED].cset);
    debug("    ld_spec:\n");
    debug("        white: [ "); for (i=0; i<ROUT_STEP; i++) debug("%u ", cfg->ld_spec[LD_WHITE][i]); debug("]\n");
    debug("        blue:  [ "); for (i=0; i<ROUT_STEP; i++) debug("%u ", cfg->ld_spec[LD_BLUE][i]); debug("]\n");
    debug("        red:   [ "); for (i=0; i<ROUT_STEP; i++) debug("%u ", cfg->ld_spec[LD_RED][i]); debug("]\n\n");

    return;
}

void cfg_load(gbCfg_t *cfg)
{
    json_t *j_body, *root, *obj, *elem;
    json_error_t error;
    const char *key;
    int i = 0;

    j_body = json_load_file("./CFG.json", 0, &error);
    if (!j_body) {
        cfg_load_dflt(cfg);
        syslog(LOG_ERR, "Unable to open config file (%s). Default config will be used.", error.text);
        goto decref;
    }


    cfg->ld_instant_mode = json_boolean_value(json_object_get(j_body,"ld_instant_mode"));


    root = json_object_get(j_body,"ld_instant");
    //key is white, blue, red. Obj is what is inside it, ex vset: 120000
    json_object_foreach(root, key, obj) {
        if (!strncmp("white", key, 5)) i=LD_WHITE;
        else if (!strncmp("blue",  key, 4)) i=LD_BLUE;
        else if (!strncmp("red",   key, 3)) i=LD_RED;
        else {
            syslog(LOG_ERR, "Config file is invalid.");
            goto decref;
        }
        cfg->ld_instant[i].enable = json_boolean_value(json_object_get(obj,"enable"));
        cfg->ld_instant[i].vset = json_integer_value(json_object_get(obj,"vset"));
        cfg->ld_instant[i].cset = json_integer_value(json_object_get(obj,"cset"));
    }


    {
        unsigned char *p = &(cfg->ld_spec[0][0]);
        int e;
        root = json_object_get(j_body,"ld_spec");
        json_object_foreach(root, key, obj) {
            i = 0;
            json_array_foreach(obj, e, elem) {
                i++;
                if(i <= ROUT_STEP) {
                    *p = (unsigned char) json_integer_value(elem);
                    p++;
                } else
                    goto decref;
            }
        }
    }
    //Generate intemediate points
    ld_generate_points();

    syslog(LOG_NOTICE, "Config successfully loaded.");
    cfg_print(cfg);

decref:
    json_decref(j_body);
    json_decref(root);
    json_decref(obj);
    json_decref(elem);

    return;
}

void cfg_save(gbCfg_t *cfg)
{
    int i;
    json_t *array_w = json_array();
    json_t *array_b = json_array();
    json_t *array_r = json_array();
    json_t *j_body;

    for (i = 0; i < ROUT_STEP; i++) {
        json_array_append_new(array_w, json_integer(cfg->ld_spec[LD_WHITE][i]));
        json_array_append_new(array_b, json_integer(cfg->ld_spec[LD_BLUE][i]));
        json_array_append_new(array_r, json_integer(cfg->ld_spec[LD_RED][i]));
    }

    j_body = json_pack("{sbs{s{sbsisi}s{sbsisi}s{sbsisi}}s{sososo}}",
            "ld_instant_mode", cfg->ld_instant_mode,
            "ld_instant",
                "white",
                    "enable", cfg->ld_instant[LD_WHITE].enable,
                    "vset", cfg->ld_instant[LD_WHITE].vset,
                    "cset", cfg->ld_instant[LD_WHITE].cset, 
                "blue",
                    "enable", cfg->ld_instant[LD_BLUE].enable,
                    "vset", cfg->ld_instant[LD_BLUE].vset,
                    "cset", cfg->ld_instant[LD_BLUE].cset, 
                "red",
                    "enable", cfg->ld_instant[LD_RED].enable,
                    "vset", cfg->ld_instant[LD_RED].vset,
                    "cset", cfg->ld_instant[LD_RED].cset,
            "ld_spec",
                "white", array_w,
                "blue", array_b,
                "red", array_r);


    if (json_dump_file(j_body, "./CFG.json", JSON_INDENT(4)) != 0)
        syslog(LOG_ERR, "Unable to save config.");

    json_decref(j_body);
    json_decref(array_w);
    json_decref(array_b);
    json_decref(array_r);
    return;
}

void cfg_apply(gbCfg_t *cfg)
{
    int i, ret;
    bool enable;

    FOR_EACH_LED(i) {
 
        ret = ld_set_voltage(i, cfg->ld_instant[i].vset);
        cfg->init_volt_applied[i] = (ret == 0) ? true : false;

        if (cfg->ld_instant_mode) {
            enable = (cfg->ld_instant[i].enable & cfg->ld_instant[i].cset);
            ret |= ld_set_current(i, cfg->ld_instant[i].cset);
            if (!ret)
                ret |= ld_set_output(i, enable);
            if (ret)
                syslog(LOG_ERR, "Error applying the config on Led %i.", i);
        }
    }

    return;
}
