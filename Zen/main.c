/*
 * main.c
 *
 * Copyright 2016  Anthony McDonald
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * Description:
 * Zen - Controls the launching of zen processes based on the Devices table in the mysql database
 *
 * Pseudocode:
 *	get passed arguments (debug zen and debug sql)
 *	open database
 *	until signalled to stop
 * 		for each entry in Devices table where activate is true
 * 			switch (status)
 *  		case: Running
 * 				All OK, but check last updated time is recent to double check
 * 			case: Stopped
 * 				spawn a rockbus process
 * 			case: Failed
 * 				write debug data to ZenLog
 * 				set status to Stopped
 * 				increment counter for this device
 * 				if counter > maxTries
 * 					set activate to false
 * 				end if
 * 			end switch
 * 		end for
 * 		sleep a bit
 *	end until
 *
 * */
#include <my_global.h>
#include <stdio.h>
#include <mysql.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "rockbusDatabase.h"
#include "errors.h"
#include "main.h"
#include "zen.h"
#include "utils.h"
#include "sqlUtils.h"
#include "sqlFunctions.h"

#include <stdio.h>
#include <signal.h>
int keepRunning;

int main(int argc, char **argv) {

    initialise(argc, argv);

    MYSQL *con = mysql_init(NULL); // Initialise MYSQL
    if (con == NULL) zenErr(ZEN_MYSQL_INIT_FAIL);

    opendb(con); // Open Database

    keepRunning = TRUE; // set to false by sigHandler when OS requests shutdown

    // Catch SIGTERM signal and shutdown gracefully
    if (signal(SIGTERM, sigHandler) == SIG_ERR) zenErr(ZEN_NO_SIGNAL);

    //Main Loop
    debugLog("Starting main loop","");
    while(keepRunning) {
        scanDevicesTable(con);
        sleep(CYCLE_TIME); // sleep for a user defined period
    }
    // Shutdown
    mysql_close(con);
    exit(EXIT_SUCCESS); // exit via exit handler - see below
}

void initialise(int argc, char **argv) {
    // get the passed argument, initialise the debug variables, open the log
    debug.SQL = LogNone;
    debug.Zen = False;
    if (argc>1) {
        if(checkArgs(argc, argv, "LogAll")) debug.SQL=LogAll;
        if(checkArgs(argc, argv, "LogChanges")) debug.SQL=LogChanges;
        if(checkArgs(argc, argv, "DebugZen")) debug.Zen=True;
    }
    if(debug.SQL!=LogNone) printf("SQL Debugging enabled\n");
    debugLog("Zen Debugging enabled","");
    rename(ZEN_LOG, OLD_ZEN_LOG);
    lg=fopen(ZEN_LOG, "w"); // create a new log file
    if(lg==NULL) zenErr(ZEN_NO_LOG);
    fclose(lg); // we keep it closed to flush out the messages so we can read it while the program is still running
    logIt("Zen Starting...");

    atexit(exitHandler); // establish an error handler to log errors
    sleep(SLEEP_BEFORE_START); // give mysql time to start up
}

void zenErr(int zenErrno) {
    // exit with an internal error, write it to stderr and also to the log file
    storeError(Zen, zenErrno, zen_strerror[zenErrno]);
    char buf[ERROR_TEXT_SIZE];
    snprintf(buf, sizeof(buf), "Internal Error %i in program %s", zenErrno, PROGRAM_NAME);
    if(zenErrno !=ZEN_NO_LOG) logIt(buf); // we can't log the ZEN_NO_LOG error!
    fprintf(stderr, "%s\n", zen_strerror[zenErrno]);
    if(zenErrno !=ZEN_NO_LOG) logIt(zen_strerror[zenErrno]);
    exit(EXIT_FAILURE);
}
int checkArgs(int argc, char **argv, char checkStr[]) {
    // look for checkStr in the first two command line arguments
    if(argc==2) if(strcmp(argv[1],checkStr)==0) return(TRUE);
    if(argc==3) if((strcmp(argv[1],checkStr)==0)||(strcmp(argv[2],checkStr)==0)) return(TRUE);
    return(FALSE);
}

void sigHandler(int sigNo) {
    if (sigNo != SIGTERM) return;
    logIt("SIGTERM signalled, exiting");
    keepRunning = FALSE;
}

void exitHandler() {
    char chardateTime[TIMESTAMP_BUF_SIZE];
    getTime(chardateTime, sizeof(chardateTime));
    logIt("Zen Exiting via exit handler");
    char buf[ERROR_TEXT_SIZE];
    int utilsErrType = Success;
    int utilsErrno = 0;
    char errMsg[ERROR_TEXT_SIZE];
    getError(&utilsErrType, &utilsErrno, errMsg, sizeof(errMsg));
    switch (utilsErrType) {
    case Success:
        strlcpy(buf,"Normal exit, no error.",sizeof(buf));
        break;
    case mySql:
        strlcpy(buf,"mySQL error:",sizeof(buf));
        strlcat(buf, errMsg, sizeof(buf));
        break;
    case Modbus:
        strlcpy(buf,"Modbus error:",sizeof(buf));
        strlcat(buf, errMsg, sizeof(buf));
        break;
    case Rockbus:
        strlcpy(buf,"Rockbus error:",sizeof(buf));
        strlcat(buf, errMsg, sizeof(buf));
        break;
    }
    logIt(buf);
    fclose(lg);
}
void logIt(char *message) {
    lg=fopen(ZEN_LOG, "a");
    if(lg==NULL) zenErr(ZEN_NO_LOG);
    char chardateTime[TIMESTAMP_BUF_SIZE];
    getTime(chardateTime, sizeof(chardateTime));
    fprintf(lg, "%s %s\n", chardateTime, message);
    fclose(lg);
}

void debugLog(char *message, char *arg) {
    if(debug.Zen) {
        char buf[ERROR_TEXT_SIZE];
        snprintf(buf, sizeof(buf), message, arg);
        logIt(buf);
        printf("%s\n",buf);
    }
}





