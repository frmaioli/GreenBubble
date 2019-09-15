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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <time.h>

#include <wiringPi.h>

#include <gb_main.h>
#include <gb_serial.h>
#include <gb_rest.h>

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


static void ld_daily_routine()
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    int mins_of_day = tm.tm_hour*60 + tm.tm_min;
    int ret = 0;
    unsigned char i = 0;
    static int latest_mins = -1;

    //Update lights each 10min
    if((latest_mins != mins_of_day) & (mins_of_day%TIME_LD == 0)) {
        //add routine
        printf("currw %i\n", get_curr_from_perc(LD_WHITE, 30));
        printf("currb %i\n", get_curr_from_perc(LD_BLUE, 10));
        printf("currr %i\n", get_curr_from_perc(LD_RED, 50));
        printf("currerr %i\n", get_curr_from_perc(5, 50));
        printf("curr100 %i\n", get_curr_from_perc(1, 120));
        
        //Get the value to be applied for this time and limit to 100% for protection
        i = mins_of_day/TIME_LD;
        if (ld_set_current(LD_WHITE, get_curr_from_perc(LD_WHITE, Gb_cfg.ld_routine_perc[LD_WHITE][i])) < 0)
            ret |= 1;
        
        if (ld_set_current(LD_BLUE, get_curr_from_perc(LD_BLUE, Gb_cfg.ld_routine_perc[LD_BLUE][i])) < 0)
            ret |= 2;
        
        if (ld_set_current(LD_RED, get_curr_from_perc(LD_RED, Gb_cfg.ld_routine_perc[LD_RED][i])) < 0)
            ret |= 4;
        
        printf("now: %d-%d-%d %d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        latest_mins = mins_of_day;
    }
    printf("mins: %d\n", mins_of_day);

    if(ret) syslog(LOG_ERR, "Error setting routine led current: %d\n", ret);
    return;
}

static void ld_sys_init(void)
{
    Gb_ld_sys[LD_WHITE].max_curr = 700; //mA
    Gb_ld_sys[LD_BLUE].max_curr = 300; //mA
    Gb_ld_sys[LD_RED].max_curr = 600; //mA
    //TODO: Get sys for each and correct the above values
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

    // Initialiye the WiringPi library
    wiringPiSetupGpio();

    // Initialiye the UART to communicate with Led Drivers
    if (ld_serial_init() < 0)
        syslog(LOG_CRIT, "Unable to open serial device.");

    // Initialiye the web server for the REST endpoints
    if (rest_ulfius_init(&ulfius_instance) < 0)
        syslog(LOG_CRIT, "Unable to start ulfius web service.");
    
    syslog(LOG_NOTICE, "GreenBubble daemon started.");

    //Get LD System information
    ld_sys_init();

    while (1)
    {
        //TODO: Insert daemon code here.
        ld_daily_routine();
        sleep (20);
//        break;
    }

    // Terminate the Daemon
    rest_ulfius_stop(&ulfius_instance);
    syslog(LOG_NOTICE, "GreenBubble daemon terminated.");
    closelog();

    return EXIT_SUCCESS;
}
