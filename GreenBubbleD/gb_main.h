/*
 * gb_main.h:
 *	Main loop for the GreenBubble project
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

#ifndef GB_MAIN_H
#define GB_MAIN_H

#include <stdbool.h>

#define MAX_LIMIT(VALUE, LIMIT) (VALUE > LIMIT) ? LIMIT : VALUE

typedef enum {
    GPIO_02 = 2, // I2C SDA
    GPIO_03 = 3, // I2C SCL
    GPIO_04 = 4,
    GPIO_05 = 5,
    GPIO_06 = 6,
    GPIO_07 = 7, // SPI0 CE1
    GPIO_08 = 8, // SPI0 CE0
    GPIO_09 = 9, // SPI0 MISO
    GPIO_10 = 10, // SPI0 MOSI
    GPIO_11 = 11, // SPI0 SCLK
    GPIO_12 = 12, // HW PWM
    GPIO_13 = 13, // HW PWM
    GPIO_14 = 14, // UART TX - Used as UART TX to the Led Drivers
    GPIO_15 = 15, // UART RX - Used as UART RX to the Led Drivers
    GPIO_16 = 16, // SPI1 CE2
    GPIO_17 = 17, // SPI1 CE1             - Used as GPIO to Led Drivers Chip Select 0
    GPIO_18 = 18, // SPI1 CE0 or HW PWM   - Used as GPIO to Led Drivers Chip Select 1
    GPIO_19 = 19, // SPI1 MISO or HW PWM
    GPIO_20 = 20, // SPI1 MOSI
    GPIO_21 = 21, // SPI1 SCLK
    GPIO_22 = 22,
    GPIO_23 = 23,
    GPIO_24 = 24,
    GPIO_25 = 25,
    GPIO_26 = 26,
    GPIO_27 = 27
} PiGpio_t;

#define LD_NUMB 3
typedef enum {
    LD_WHITE = 0,
    LD_BLUE,
    LD_RED
} ldBoard_t;

typedef struct {
    char model[10];
    char version[10];
    char name[17];
    bool default_on;
    bool autocommit;
    unsigned int max_curr; //mA
} ldSys_t;

typedef struct {
    bool enable;
    float vset; // V: 2.321
    float cset; // A: 0.755
} ldCfg_t;

typedef struct {
    bool enable;
    unsigned int vin_raw;
    unsigned int vout_raw;
    unsigned int cout_raw;
    float vin;  // V: 35.980
    float vout; // V: 2.321
    float cout; // A: 0.755
    bool constant_current; // If false, we are in constant voltage
} ldSts_t;

/***************** STATUS *******************/

typedef struct {
    float temp_air;
    float temp_water;
    float humidity_air;
    ldSts_t ld_sts[LD_NUMB];
    //...
} gbSts_t;

extern gbSts_t Gb_sts;


/***************** CONFIG *******************/
#define TIME_LD 1 //in Minuts
typedef struct {
    unsigned char ld_routine_perc[LD_NUMB][(1440/TIME_LD)];    //144 = 1 each 10min for 24hs
    ldCfg_t ld_instant[LD_NUMB];
} gbCfg_t;

extern ldSys_t Gb_ld_sys[LD_NUMB];
extern gbCfg_t Gb_cfg;
extern gbSts_t Gb_sts;

#endif //GB_MAIN_H
