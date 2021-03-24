/*
 * gb_main.c:
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

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

#include <wiringPi.h>

#include <gb_main.h>
#include <gb_config.h>
#include <gb_serial.h>
#include <gb_rest.h>
#include <gb_led.h>

//Global GreenBubble entities
ldSys_t Gb_ld_sys[LD_NUMB];
gbSts_t Gb_sts;
gbCfg_t Gb_cfg;

static void daemon_init()
{
#if 0
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    /* Catch, ignore and handle signals */
    //TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);
    
    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/");

    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
    {
        close (x);
    }

#endif
    /* Open the log file */
    openlog("GreenBubbleD", LOG_PID, LOG_DAEMON);
}

int main()
{
    struct _u_instance ulfius_instance;

    //Initilize global entities
    memset(&Gb_ld_sys, 0, sizeof(Gb_ld_sys));
    memset(&Gb_sts, 0, sizeof(Gb_sts));
    memset(&Gb_cfg, 0, sizeof(Gb_cfg));
    
    // Initialiye the Daemon
    daemon_init();

    // Create default Config
    cfg_load(&Gb_cfg);
    cfg_save(&Gb_cfg);  //test

    // Initialiye the WiringPi library
    wiringPiSetupGpio();

    // Initialiye the UART to communicate with Led Drivers
    if (ld_serial_init() < 0)
        syslog(LOG_CRIT, "Unable to open serial device.");

    // Initialiye the web server for the REST endpoints
    if (rest_ulfius_init(&ulfius_instance) < 0)
        syslog(LOG_CRIT, "Unable to start ulfius web service.");

    //Get LED System information
    if (ld_sys_init() < 0)
        syslog(LOG_CRIT, "Unable to get Led Device Stystem's information.");

    //Apply default cfg to the Leds (ex. set voltage, as after we control only current
//    else {
//        if (ld_apply_dflt_cfg(&Gb_cfg) < 0)
//            syslog(LOG_ERR, "Unable to apply Led Device default config.");
//    }
    
    syslog(LOG_NOTICE, "GreenBubble daemon started.");

    while (1)
    {
        //TODO: Insert daemon code here.
        ld_daily_routine(0);

	//just testing smtg....
        ld_get_config(LD_WHITE, &Gb_cfg.ld_instant[LD_WHITE]);
    	debug("config vset: %u cset: %u\n", Gb_cfg.ld_instant[LD_WHITE].vset, Gb_cfg.ld_instant[LD_WHITE].cset);

        sleep (20);
//        break;
    }

    // Terminate the Daemon
    rest_ulfius_stop(&ulfius_instance);
    syslog(LOG_NOTICE, "GreenBubble daemon terminated.");
    closelog();

    return EXIT_SUCCESS;
}
