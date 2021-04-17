/*
 * gb_rest.c:
 *	Rest endpoints for the GreenBubble project
 *	Based on Ulfius Framework
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
#include <errno.h>
#include <syslog.h>
#include <jansson.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ulfius.h>

#include <gb_rest.h>
#include <gb_serial.h>
#include <gb_led.h>
#include <gb_config.h>

#define PORT 8537
#define PREFIX "/GBBL"
#define PREFIXJSON "/testjson"
#define PREFIXCOOKIE "/testcookie"

/**
 * callback functions declaration
 */
int callback_gb_status (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_gb_charts (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_gb_system (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_gb_config (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_post_config (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_options (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_default (const struct _u_request * request, struct _u_response * response, void * user_data);

int rest_ulfius_init (struct _u_instance *instance) {
    if (ulfius_init_instance(instance, PORT, NULL, NULL) != U_OK) {
        fprintf (stderr, "Ulfius unable to initiate instance: %s\n", strerror(errno));
        return -1;
    }
  
    //u_map_put(instance.default_headers, "Access-Control-Allow-Origin", "*");
  
    // Maximum body size sent by the client is 16 Kb
    instance->max_post_body_size = 16*1024;
  
    // Endpoint list declaration
    ulfius_add_endpoint_by_val(instance, "GET", PREFIX, "/status", 0, &callback_gb_status, NULL);
    ulfius_add_endpoint_by_val(instance, "GET", PREFIX, "/charts", 0, &callback_gb_charts, NULL);
    ulfius_add_endpoint_by_val(instance, "GET", PREFIX, "/system", 0, &callback_gb_system, NULL);
    ulfius_add_endpoint_by_val(instance, "GET", PREFIX, "/config", 0, &callback_gb_config, NULL);
    ulfius_add_endpoint_by_val(instance, "POST", PREFIX, "/post/config", 0, &callback_post_config, NULL);
    ulfius_add_endpoint_by_val(instance, "OPTIONS", PREFIX, "/post/config", 0, &callback_options, NULL);

    // Set default headers for CORS
    u_map_put(instance->default_headers, "Access-Control-Allow-Origin", "*");
    u_map_put(instance->default_headers, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    u_map_put(instance->default_headers, "Access-Control-Max-Age", "1800");
    u_map_put(instance->default_headers, "Access-Control-Allow-Credentials", "true");
    u_map_put(instance->default_headers, "Cache-Control", "no-store");
    u_map_put(instance->default_headers, "Pragma", "no-cache");
    u_map_put(instance->default_headers, "Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept, Bearer, Authorization");


    // default_endpoint declaration
    ulfius_set_default_endpoint(instance, &callback_default, NULL);

    // Start the framework
    if (ulfius_start_framework(instance) == U_OK) {
        syslog(LOG_INFO, "Ulfius Started framework on port %d\n", instance->port);
    } else {
        fprintf (stderr, "Unable to start Ulfius framework: %s\n", strerror(errno));
        rest_ulfius_stop(instance);
        return -1;
    }

  return 0;
}

void rest_ulfius_stop (struct _u_instance *instance)
{
    ulfius_stop_framework(instance);
    ulfius_clean_instance(instance);
    return;
}

/**
 * Default callback function called if no endpoint has a match
 */
int callback_default (const struct _u_request * request, struct _u_response * response, void * user_data) {
    ulfius_set_string_body_response(response, 404, "GreenBubble - Page not found");
    return U_CALLBACK_CONTINUE;
}

/**
 * Before send a Post, web browser send an OPTIONS to confirm CORS. Answer OKwith headers.
 */
int callback_options (const struct _u_request * request, struct _u_response * response, void * user_data) {
    ulfius_set_string_body_response(response, 200, "GreenBubble - Options return");
    return U_CALLBACK_CONTINUE;
}

//sends a json
int callback_gb_charts (const struct _u_request * request, struct _u_response * response, void * user_data) {

    json_t *j_body = json_pack("{s{sososo}sososososososo}",
            "hist_ld_spec",
                "white", Gb_sts.hist.intens[LD_WHITE],
                "blue", Gb_sts.hist.intens[LD_BLUE],
                "red", Gb_sts.hist.intens[LD_RED],
            "hist_vin", Gb_sts.hist.vin,
            "hist_humidity", Gb_sts.hist.humidity,
            "hist_rain", Gb_sts.hist.rain,
            "hist_fog", Gb_sts.hist.fog,
            "hist_tempPS", Gb_sts.hist.tPS,
            "hist_tempAir", Gb_sts.hist.tAir,
            "hist_tempWater", Gb_sts.hist.tWater);

    ulfius_set_json_body_response(response, 200, j_body);

    /*Used to debug only */
    //if (json_dump_file(j_body, "./jsonTime.json", JSON_INDENT(4)) != 0)
    //    syslog(LOG_ERR, "Unable to save config.");

    //Do not decref! As it continues to be used.
    return U_CALLBACK_CONTINUE;
}

//sends a json
int callback_gb_status (const struct _u_request * request, struct _u_response * response, void * user_data) {

    json_t * j_body = json_pack("{sisisisbsbsisis{sisisi}s{sisisi}s{sisisi}}",
            "temp_air", Gb_sts.temp_air,
            "temp_water", Gb_sts.temp_water,
            "humidity_air", Gb_sts.humidity_air,
            "rain", Gb_sts.rain,
            "fog", Gb_sts.fog,
            "temp_PS", Gb_sts.temp_PS,
            "voltage_in", Gb_sts.ld_sts[LD_WHITE].vin,
            "led_white",
                "voltage", Gb_sts.ld_sts[LD_WHITE].vout,
                "current", Gb_sts.ld_sts[LD_WHITE].cout,
                "intens", get_perc_from_curr(LD_WHITE, Gb_sts.ld_sts[LD_WHITE].cout),
            "led_blue",
                "voltage", Gb_sts.ld_sts[LD_BLUE].vout,
                "current", Gb_sts.ld_sts[LD_BLUE].cout,
                "intens", get_perc_from_curr(LD_BLUE, Gb_sts.ld_sts[LD_BLUE].cout),
            "led_red",
                "voltage", Gb_sts.ld_sts[LD_RED].vout,
                "current", Gb_sts.ld_sts[LD_RED].cout,
                "intens", get_perc_from_curr(LD_RED, Gb_sts.ld_sts[LD_RED].cout));

    ulfius_set_json_body_response(response, 200, j_body);
    json_decref(j_body);

  return U_CALLBACK_CONTINUE;
}

//sends a json
int callback_gb_system (const struct _u_request * request, struct _u_response * response, void * user_data) {

    json_t * j_body = json_pack("{s{sssssssbsbsisisisi}s{sssssssbsbsisisisi}s{sssssssbsbsisisisi}}",
            "ld_white",
                "model", Gb_ld_sys[LD_WHITE].model,
                "version", Gb_ld_sys[LD_WHITE].version,
                "name", Gb_ld_sys[LD_WHITE].name,
                "default_on", Gb_ld_sys[LD_WHITE].default_on,
                "autocommit", Gb_ld_sys[LD_WHITE].autocommit,
                "fwd_led_curr", Gb_ld_sys[LD_WHITE].fwd_led_curr,
                "fwd_led_volt", Gb_ld_sys[LD_WHITE].fwd_led_volt,
                "numb_leds", Gb_ld_sys[LD_WHITE].numb_leds,
                "wave_length", Gb_ld_sys[LD_WHITE].wave_length,
            "ld_blue",
                "model", Gb_ld_sys[LD_BLUE].model,
                "version", Gb_ld_sys[LD_BLUE].version,
                "name", Gb_ld_sys[LD_BLUE].name,
                "default_on", Gb_ld_sys[LD_BLUE].default_on,
                "autocommit", Gb_ld_sys[LD_BLUE].autocommit,
                "fwd_led_curr", Gb_ld_sys[LD_BLUE].fwd_led_curr,
                "fwd_led_volt", Gb_ld_sys[LD_BLUE].fwd_led_volt,
                "numb_leds", Gb_ld_sys[LD_BLUE].numb_leds,
                "wave_length", Gb_ld_sys[LD_BLUE].wave_length,
            "ld_red",
                "model", Gb_ld_sys[LD_RED].model,
                "version", Gb_ld_sys[LD_RED].version,
                "name", Gb_ld_sys[LD_RED].name,
                "default_on", Gb_ld_sys[LD_RED].default_on,
                "autocommit", Gb_ld_sys[LD_RED].autocommit,
                "fwd_led_curr", Gb_ld_sys[LD_RED].fwd_led_curr,
                "fwd_led_volt", Gb_ld_sys[LD_RED].fwd_led_volt,
                "numb_leds", Gb_ld_sys[LD_RED].numb_leds,
                "wave_length", Gb_ld_sys[LD_RED].wave_length);

    ulfius_set_json_body_response(response, 200, j_body);
    json_decref(j_body);

  return U_CALLBACK_CONTINUE;
}

//sends a json
int callback_gb_config (const struct _u_request * request, struct _u_response * response, void * user_data) {
    int i;
    json_t *array_w = json_array();
    json_t *array_b = json_array();
    json_t *array_r = json_array();
    json_t *j_body;

    for (i = 0; i < ROUT_STEP; i++) {
        json_array_append_new(array_w, json_integer(Gb_cfg.ld_spec[LD_WHITE][i]));
        json_array_append_new(array_b, json_integer(Gb_cfg.ld_spec[LD_BLUE][i]));
        json_array_append_new(array_r, json_integer(Gb_cfg.ld_spec[LD_RED][i]));
    }

    j_body = json_pack("{sbs{s{sbsisi}s{sbsisi}s{sbsisi}}s{sososo}}",
            "ld_instant_mode", Gb_cfg.ld_instant_mode,
            "ld_instant",
                "white",
                    "enable", Gb_cfg.ld_instant[LD_WHITE].enable,
                    "vset", Gb_cfg.ld_instant[LD_WHITE].vset,
                    "cset", Gb_cfg.ld_instant[LD_WHITE].cset, 
                "blue",
                    "enable", Gb_cfg.ld_instant[LD_BLUE].enable,
                    "vset", Gb_cfg.ld_instant[LD_BLUE].vset,
                    "cset", Gb_cfg.ld_instant[LD_BLUE].cset, 
                "red",
                    "enable", Gb_cfg.ld_instant[LD_RED].enable,
                    "vset", Gb_cfg.ld_instant[LD_RED].vset,
                    "cset", Gb_cfg.ld_instant[LD_RED].cset,
            "ld_spec",
                "white", array_w,
                "blue", array_b,
                "red", array_r);

    ulfius_set_json_body_response(response, 200, j_body);

    json_decref(j_body);
    json_decref(array_w);
    json_decref(array_b);
    json_decref(array_r);

  return U_CALLBACK_CONTINUE;
}

/**
 * Callback function that receives a post in json format
 *{
 *	"save_cfg": true,
 *	"instant_mode": true,
 *	"enable": true,
 *      "white_intensity": 62,
 *      "blue_intensity": 10,
 *      "red_intensity": 88
 *      "light_spec": [[0, 100, 100, 75, 100, 100, 75, 0, 0], [0, 0, 0, 80, 40, 10, 0, 3, 0],…]
 *}
 */
int callback_post_config (const struct _u_request * request, struct _u_response * response, void * user_data) {
    
    bool save, instant_mode, enable, en;
    int ret=0, count=0;
    unsigned int intens, curr;
    char * response_body;
    json_t * json_body_req = ulfius_get_json_body_request(request, NULL);
    
    save = json_boolean_value(json_object_get(json_body_req,"save_cfg"));
    instant_mode = json_boolean_value(json_object_get(json_body_req,"instant_mode"));
    en = json_boolean_value(json_object_get(json_body_req,"enable"));
    
    /* Check the Mode of Operation */
    if (instant_mode) {
        //Instant Mode

        //White
        intens = json_integer_value(json_object_get(json_body_req,"white_intensity"));
        curr = get_curr_from_perc(LD_WHITE, intens);
        enable = (en & intens);
        if ((ld_set_current(LD_WHITE, curr) < 0) || (ld_set_output(LD_WHITE, enable) < 0))
            ret |= 1;
        else {
            Gb_cfg.ld_instant[LD_WHITE].cset = curr;
            Gb_cfg.ld_instant[LD_WHITE].enable = enable;
        }

        //Blue
        intens = json_integer_value(json_object_get(json_body_req,"blue_intensity"));
        curr = get_curr_from_perc(LD_BLUE, intens);
        enable = (en & intens);
        if ((ld_set_current(LD_BLUE, curr) < 0) || (ld_set_output(LD_BLUE, enable) < 0))
            ret |= 2;
        else {
            Gb_cfg.ld_instant[LD_BLUE].cset = curr;
            Gb_cfg.ld_instant[LD_BLUE].enable = enable;
        }

        //RED
        intens = json_integer_value(json_object_get(json_body_req,"red_intensity"));
        curr = get_curr_from_perc(LD_RED, intens);
        enable = (en & intens);
        if ((ld_set_current(LD_RED, curr) < 0) || (ld_set_output(LD_RED, enable) < 0))
            ret |= 4;
        else {
            Gb_cfg.ld_instant[LD_RED].cset = curr;
            Gb_cfg.ld_instant[LD_RED].enable = enable;
        }

    } else {
        //Spectrum Mode
        int a, e;
        unsigned char *p = &(Gb_cfg.ld_spec[0][0]);
        json_t * j_element, * j_array;
        json_t * j_obj = json_object_get(json_body_req,"light_spec");

        //Iterate over Json points
        json_array_foreach(j_obj, a, j_array) {
            json_array_foreach(j_array, e, j_element) {
                count++;
                if(count <= LD_NUMB*ROUT_STEP) {
                    *p = (unsigned char) json_integer_value(j_element);
                    p++;
                } else
                    ret |= 8;
            }
        }
        //Generate intemediate points
        ld_generate_points();
        ld_daily_routine(1);
    }

    Gb_cfg.ld_instant_mode = instant_mode;

    if (save) {
        cfg_save(&Gb_cfg);
    }

    if (ret) {
        response_body = msprintf("Error setting config: Code %i\n", ret);
        syslog(LOG_ERR, response_body);
        ulfius_set_string_body_response(response, 500, response_body);
        o_free(response_body);
        json_decref(json_body_req);
        return U_CALLBACK_CONTINUE;
    }

    response_body = msprintf("%s:\nConfig Successfully Applied%s\n", instant_mode ? "Instant Control" : "Spectrum", save ? " and Saved." : ".");
    syslog(LOG_INFO, response_body);
    debug(response_body);
  
    ulfius_set_string_body_response(response, 200, response_body);
    o_free(response_body);
    json_decref(json_body_req);
    return U_CALLBACK_CONTINUE;
}
