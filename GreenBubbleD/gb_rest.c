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

#define PORT 8537
#define PREFIX "/GBBL"
#define PREFIXJSON "/testjson"
#define PREFIXCOOKIE "/testcookie"

/**
 * callback functions declaration
 */
int callback_hello_world (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_ld_enable (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_gb_status (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_gb_system (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_post_light (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_post_cfg (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_default (const struct _u_request * request, struct _u_response * response, void * user_data);

/**
 * decode a u_map into a string
 */
static char * print_map(const struct _u_map * map) {
  char * line, * to_return = NULL;
  const char **keys, * value;
  int len, i;
  if (map != NULL) {
    keys = u_map_enum_keys(map);
    for (i=0; keys[i] != NULL; i++) {
      value = u_map_get(map, keys[i]);
      len = snprintf(NULL, 0, "key is %s, value is %s", keys[i], value);
      line = o_malloc((len+1)*sizeof(char));
      snprintf(line, (len+1), "key is %s, value is %s", keys[i], value);
      if (to_return != NULL) {
        len = o_strlen(to_return) + o_strlen(line) + 1;
        to_return = o_realloc(to_return, (len+1)*sizeof(char));
        if (o_strlen(to_return) > 0) {
          strcat(to_return, "\n");
        }
      } else {
        to_return = o_malloc((o_strlen(line) + 1)*sizeof(char));
        to_return[0] = 0;
      }
      strcat(to_return, line);
      o_free(line);
    }
    return to_return;
  } else {
    return NULL;
  }
}

int rest_ulfius_init (struct _u_instance *instance) {
    if (ulfius_init_instance(instance, PORT, NULL, NULL) != U_OK) {
        fprintf (stderr, "Ulfius unable to initiate instance: %s\n", strerror(errno));
        return -1;
    }
  
    //u_map_put(instance.default_headers, "Access-Control-Allow-Origin", "*");
  
    // Maximum body size sent by the client is 16 Kb
    instance->max_post_body_size = 16*1024;
  
    // Endpoint list declaration
    ulfius_add_endpoint_by_val(instance, "GET", PREFIX, "/set/led/@color/enable/@on", 0, &callback_ld_enable, NULL);
    ulfius_add_endpoint_by_val(instance, "GET", PREFIX, "/status", 0, &callback_gb_status, NULL);
    ulfius_add_endpoint_by_val(instance, "GET", PREFIX, "/system", 0, &callback_gb_system, NULL);
    ulfius_add_endpoint_by_val(instance, "GET", "/helloworld", NULL, 0, &callback_hello_world, NULL);
    ulfius_add_endpoint_by_val(instance, "POST", PREFIX, "/cfg/light", 0, &callback_post_light, NULL);
    ulfius_add_endpoint_by_val(instance, "POST", PREFIX, "/config", 0, &callback_post_cfg, NULL);

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
 * Callback function for the web application on /helloworld url call
 */
int callback_hello_world (const struct _u_request * request, struct _u_response * response, void * user_data) {
  ulfius_set_string_body_response(response, 200, "Hello World!");
  return U_CALLBACK_CONTINUE;
}

//sends a json
int callback_gb_status (const struct _u_request * request, struct _u_response * response, void * user_data) {

    json_t * j_body = json_pack("{sisisisis{sisi}s{sisi}s{sisi}}",
            "temp_air", Gb_sts.temp_air,
            "temp_water", Gb_sts.temp_water,
            "humidity_air", Gb_sts.humidity_air,
            "voltage_in", Gb_sts.ld_sts[LD_WHITE].vin,
            "led_white",
                "voltage", Gb_sts.ld_sts[LD_WHITE].vout,
                "current", Gb_sts.ld_sts[LD_WHITE].cout,
            "led_blue",
                "voltage", Gb_sts.ld_sts[LD_BLUE].vout,
                "current", Gb_sts.ld_sts[LD_BLUE].cout,
            "led_red",
                "voltage", Gb_sts.ld_sts[LD_RED].vout,
                "current", Gb_sts.ld_sts[LD_RED].cout);

    ulfius_set_json_body_response(response, 200, j_body);
    json_decref(j_body);

    /* below works too
  json_t * j_query = json_pack("{sss{sisf}}",
         "name", "fabiano",
         "dados",
            "idade", 36,
            "altura", 1.76);
    ulfius_set_json_body_response(response, 200, j_query);
  
     below works too
  json_t * json_body = NULL;

  json_body = json_object();
  json_object_set_new(json_body, "nbsheep", json_real(1234.567));
  ulfius_set_json_body_response(response, 200, json_body);
  json_decref(json_body);
  */
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


/* URL/led/@color/enable/@on Ex: led/white/enable/1 */
int callback_ld_enable (const struct _u_request * request, struct _u_response * response, void * user_data) {

    ldBoard_t color;
    const char *color_str = u_map_get(request->map_url, "color");
    bool enable = (strcmp(u_map_get(request->map_url, "on"), "1") == 0) ? 1 : 0;

    if (strcmp(color_str, "white") == 0) {
        color = LD_WHITE;
    } else if (strcmp(color_str, "blue") == 0) {
        color = LD_BLUE;
    } else if (strcmp(color_str, "red") == 0) {
        color = LD_RED;
    } else {
      response->status = 404;
      ulfius_set_string_body_response(response, 404, "Led color invalid");
      return U_CALLBACK_ERROR;
    }
    
    ld_set_output(color, enable);
    //check return and change Cfg - to be created
    
    syslog(LOG_INFO, "Led color %s: enable state set to %d\n", color_str, enable);
    ulfius_set_string_body_response(response, 200, "Success Operation");
    return U_CALLBACK_CONTINUE;
}

//BETTER THE BELOW WAY

void light_generate_points()
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
    Gb_cfg.ld_routine_init = true;  
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
int callback_post_light (const struct _u_request * request, struct _u_response * response, void * user_data) {
    
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
        light_generate_points();
        ld_daily_routine(1);
    }

    if (save) {
        //TODO
    }

    if (ret) {
        response_body = msprintf("Error setting lights: Code %i\n", ret);
        syslog(LOG_ERR, response_body);
        ulfius_set_string_body_response(response, 500, response_body);
        o_free(response_body);
        json_decref(json_body_req);
        return U_CALLBACK_CONTINUE;
    }

    response_body = msprintf("Light Cfg - Save: %d, Inst mode: %d, Enable: %d, W: %d, B: %d, R: %d\n",
            save, instant_mode, en, Gb_cfg.ld_instant[LD_WHITE].cset, Gb_cfg.ld_instant[LD_BLUE].cset
            ,Gb_cfg.ld_instant[LD_RED].cset);
    syslog(LOG_INFO, response_body);
    debug(response_body);
  
    ulfius_set_string_body_response(response, 200, response_body);
    o_free(response_body);
    json_decref(json_body_req);
    return U_CALLBACK_CONTINUE;
}


/**
 * Callback function that receives a post in json format
 *{
 *      "name": "Fabiano",
 *      "age": 36
 *}
 */
int callback_post_cfg (const struct _u_request * request, struct _u_response * response, void * user_data) {
  json_t * json_body_req = ulfius_get_json_body_request(request, NULL);
  char * response_body;
  
  int age = json_integer_value(json_object_get(json_body_req,"age"));
  response_body = msprintf("Hello papito!\nAge: %i", age);
  
  ulfius_set_string_body_response(response, 200, response_body);
  o_free(response_body);
  json_decref(json_body_req);
  return U_CALLBACK_CONTINUE;
}

/**
 * Default callback function called if no endpoint has a match
 */
int callback_default (const struct _u_request * request, struct _u_response * response, void * user_data) {
    ulfius_set_string_body_response(response, 404, "GreenBubble - Page not found");
    return U_CALLBACK_CONTINUE;
}


// Next: finish the call to gb_serial, testing errors
// angharad.c and angharad.service.js are good exemples to continue









/*-------------------------------------------------------------------*/

#if 0
/**
 * callback functions declaration
 */
int callback_get_test (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_get_empty_response (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_post_test (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_all_test_foo (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_get_cookietest (const struct _u_request * request, struct _u_response * response, void * user_data);

int callback_default (const struct _u_request * request, struct _u_response * response, void * user_data);

/**
 * decode a u_map into a string
 */
char * print_map(const struct _u_map * map) {
  char * line, * to_return = NULL;
  const char **keys, * value;
  int len, i;
  if (map != NULL) {
    keys = u_map_enum_keys(map);
    for (i=0; keys[i] != NULL; i++) {
      value = u_map_get(map, keys[i]);
      len = snprintf(NULL, 0, "key is %s, value is %s", keys[i], value);
      line = o_malloc((len+1)*sizeof(char));
      snprintf(line, (len+1), "key is %s, value is %s", keys[i], value);
      if (to_return != NULL) {
        len = o_strlen(to_return) + o_strlen(line) + 1;
        to_return = o_realloc(to_return, (len+1)*sizeof(char));
        if (o_strlen(to_return) > 0) {
          strcat(to_return, "\n");
        }
      } else {
        to_return = o_malloc((o_strlen(line) + 1)*sizeof(char));
        to_return[0] = 0;
      }
      strcat(to_return, line);
      o_free(line);
    }
    return to_return;
  } else {
    return NULL;
  }
}

char * read_file(const char * filename) {
  char * buffer = NULL;
  long length;
  FILE * f = fopen (filename, "rb");
  if (f != NULL) {
    fseek (f, 0, SEEK_END);
    length = ftell (f);
    fseek (f, 0, SEEK_SET);
    buffer = o_malloc (length + 1);
    if (buffer != NULL) {
      fread (buffer, 1, length, f);
      buffer[length] = '\0';
    }
    fclose (f);
  }
  return buffer;
}

int main (int argc, char **argv) {
  int ret;
  
  // Set the framework port number
  struct _u_instance instance;
  
  y_init_logs("simple_example", Y_LOG_MODE_CONSOLE, Y_LOG_LEVEL_DEBUG, NULL, "Starting simple_example");
  
  if (ulfius_init_instance(&instance, PORT, NULL, NULL) != U_OK) {
    y_log_message(Y_LOG_LEVEL_ERROR, "Error ulfius_init_instance, abort");
    return(1);
  }
  
  //u_map_put(instance.default_headers, "Access-Control-Allow-Origin", "*");
  
  // Maximum body size sent by the client is 1 Kb
  instance.max_post_body_size = 1024;
  
  // Endpoint list declaration
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, NULL, 0, &callback_get_test, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/empty", 0, &callback_get_empty_response, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/multiple/:multiple/:multiple/:not_multiple", 0, &callback_all_test_foo, NULL);
  ulfius_add_endpoint_by_val(&instance, "POST", PREFIX, NULL, 0, &callback_post_test, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIX, "/param/:foo", 0, &callback_all_test_foo, "user data 1");
  ulfius_add_endpoint_by_val(&instance, "POST", PREFIX, "/param/:foo", 0, &callback_all_test_foo, "user data 2");
  ulfius_add_endpoint_by_val(&instance, "PUT", PREFIX, "/param/:foo", 0, &callback_all_test_foo, "user data 3");
  ulfius_add_endpoint_by_val(&instance, "DELETE", PREFIX, "/param/:foo", 0, &callback_all_test_foo, "user data 4");
  ulfius_add_endpoint_by_val(&instance, "GET", PREFIXCOOKIE, "/:lang/:extra", 0, &callback_get_cookietest, NULL);
  
  // default_endpoint declaration
  ulfius_set_default_endpoint(&instance, &callback_default, NULL);
  
  // Start the framework
  if (argc == 4 && o_strcmp("-secure", argv[1]) == 0) {
    // If command-line options are -secure <key_file> <cert_file>, then open an https connection
    char * key_pem = read_file(argv[2]), * cert_pem = read_file(argv[3]);
    ret = ulfius_start_secure_framework(&instance, key_pem, cert_pem);
    o_free(key_pem);
    o_free(cert_pem);
  } else {
    // Open an http connection
    ret = ulfius_start_framework(&instance);
  }
  
  if (ret == U_OK) {
    y_log_message(Y_LOG_LEVEL_DEBUG, "Start %sframework on port %d", ((argc == 4 && o_strcmp("-secure", argv[1]) == 0)?"secure ":""), instance.port);
    
    // Wait for the user to press <enter> on the console to quit the application
    getchar();
  } else {
    y_log_message(Y_LOG_LEVEL_DEBUG, "Error starting framework");
  }
  y_log_message(Y_LOG_LEVEL_DEBUG, "End framework");
  
  y_close_logs();
  
  ulfius_stop_framework(&instance);
  ulfius_clean_instance(&instance);
  
  return 0;
}

/**
 * Callback function that put a "Hello World!" string in the response
 */
int callback_get_test (const struct _u_request * request, struct _u_response * response, void * user_data) {
  ulfius_set_string_body_response(response, 200, "Hello World!");
  return U_CALLBACK_CONTINUE;
}

/**
 * Callback function that put an empty response and a status 200
 */
int callback_get_empty_response (const struct _u_request * request, struct _u_response * response, void * user_data) {
  return U_CALLBACK_CONTINUE;
}

/**
 * Callback function that put a "Hello World!" and the post parameters send by the client in the response
 */
int callback_post_test (const struct _u_request * request, struct _u_response * response, void * user_data) {
  char * post_params = print_map(request->map_post_body);
  char * response_body = msprintf("Hello World!\n%s", post_params);
  ulfius_set_string_body_response(response, 200, response_body);
  o_free(response_body);
  o_free(post_params);
  return U_CALLBACK_CONTINUE;
}

/**
 * Callback function that put "Hello World!" and all the data sent by the client in the response as string (http method, url, params, cookies, headers, post, json, and user specific data in the response
 */
int callback_all_test_foo (const struct _u_request * request, struct _u_response * response, void * user_data) {
  char * url_params = print_map(request->map_url), * headers = print_map(request->map_header), * cookies = print_map(request->map_cookie), 
        * post_params = print_map(request->map_post_body);
#ifndef _WIN32
  char * response_body = msprintf("Hello World!\n\n  method is %s\n  url is %s\n\n  parameters from the url are \n%s\n\n  cookies are \n%s\n\n  headers are \n%s\n\n  post parameters are \n%s\n\n  user data is %s\n\nclient address is %s\n\n",
                                  request->http_verb, request->http_url, url_params, cookies, headers, post_params, (char *)user_data, inet_ntoa(((struct sockaddr_in *)request->client_address)->sin_addr));
#else
  char * response_body = msprintf("Hello World!\n\n  method is %s\n  url is %s\n\n  parameters from the url are \n%s\n\n  cookies are \n%s\n\n  headers are \n%s\n\n  post parameters are \n%s\n\n  user data is %s\n\n",
                                  request->http_verb, request->http_url, url_params, cookies, headers, post_params, (char *)user_data);
#endif
  ulfius_set_string_body_response(response, 200, response_body);
  o_free(url_params);
  o_free(headers);
  o_free(cookies);
  o_free(post_params);
  o_free(response_body);
  return U_CALLBACK_CONTINUE;
}

/**
 * Callback function that sets cookies in the response
 * The counter cookie is incremented every time the client reloads this url
 */
int callback_get_cookietest (const struct _u_request * request, struct _u_response * response, void * user_data) {
  const char * lang = u_map_get(request->map_url, "lang"), * extra = u_map_get(request->map_url, "extra"), 
             * counter = u_map_get(request->map_cookie, "counter");
  char new_counter[8];
  int i_counter;
  
  if (counter == NULL) {
    i_counter = 0;
  } else {
    i_counter = strtol(counter, NULL, 10);
    i_counter++;
  }
  snprintf(new_counter, 7, "%d", i_counter);
  ulfius_add_cookie_to_response(response, "lang", lang, NULL, 0, NULL, NULL, 0, 0);
  ulfius_add_cookie_to_response(response, "extra", extra, NULL, 0, NULL, NULL, 0, 0);
  ulfius_add_cookie_to_response(response, "counter", new_counter, NULL, 0, NULL, NULL, 0, 0);
  ulfius_set_string_body_response(response, 200, "Cookies set!");
  
  return U_CALLBACK_CONTINUE;
}

/**
 * Default callback function called if no endpoint has a match
 */
int callback_default (const struct _u_request * request, struct _u_response * response, void * user_data) {
  ulfius_set_string_body_response(response, 404, "Page not found, do what you want");
  return U_CALLBACK_CONTINUE;
}
#endif
