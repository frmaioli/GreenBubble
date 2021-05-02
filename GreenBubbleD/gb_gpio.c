/*
 * gb_gpio.c:
 *	GPIO Access for the GreenBubble project
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
#include <wiringPi.h>
#include <ds18b20.h>
#include <rht03.h>

#include <gb_gpio.h>

static void gb_sensor_init(void)
{
    int ret;

    /* We dont neet to use a pin, the kernel identify the sensor */
    //TODO: Below add the correct serial Nb of each sensor
    ret =  ds18b20Setup(DS18_01, "0000053af458"); //PS
    ret |= ds18b20Setup(DS18_02, "0000053af458"); //water
    ret |= ds18b20Setup(DS18_03, "0000053af458"); //needed?

    if (ret)
        syslog(LOG_ERR, "Could not initialize DS18B20 Sensors Node.");

    //DHT22 (same as RHT03)
    ret =  rht03Setup(DHT22_01, BCM_26);
    if (ret)
        syslog(LOG_ERR, "Could not initialize DHT22 Sensor Node.");

    return;
}

void gb_gpio_init(void)
{
    // Initialiye the WiringPi library
    wiringPiSetupGpio();

    /* Set Pin Modes */
    pinMode(BCM_16, OUTPUT); //PWC
    pinMode(BCM_17, OUTPUT); //Fog
    pinMode(BCM_18, INPUT);  //Rain
    pinMode(BCM_22, OUTPUT); //Led CS0
    pinMode(BCM_23, OUTPUT); //Led CS1
    pinMode(BCM_25, INPUT);  //Temp
    pinMode(BCM_26, INPUT);  //Humid+Temp

    /* Set Pull Resistors for Input Pins */
    pullUpDnControl(BCM_18, PUD_DOWN);
    pullUpDnControl(BCM_25, PUD_OFF); //External Circuit has Pull Ups
    pullUpDnControl(BCM_26, PUD_OFF); //External Circuit has Pull Ups

    /* Set Initial Values of Outputs */
    digitalWrite(BCM_16, LOW);
    digitalWrite(BCM_17, LOW);
    digitalWrite(BCM_22, LOW);
    digitalWrite(BCM_23, LOW);

    /* Initialize temperature and humidity sensor nodes */
    gb_sensor_init();

    return;
}
