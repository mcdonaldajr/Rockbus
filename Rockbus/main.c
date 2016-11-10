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
 * Description: Rockbus - a service that synchronises a MySQL database with one or more
 * Modbus devices
 *
 * one parameter, the number of the primary key in the Devices tables
 * indicating which modbus device to connect to.
 *
 * PseudoCode
 *
 * open the database and get operating parameters from devices table
 * connect to the modbus device
 * until told to stop
 *		get data from Modbus device
 * 		get changes from the database (using the timestamp on each record) and send to modbus device
 * 		get changes from modbus and update the database
 * 		sleep
 * 		get the latest parameters again from devices table
 * end until
 * shut down
 *
 *
 * ToDo:
 * modbus_report_slave_id in devices
 * add bounds checking for all numeric fields in database (i.e. positive!)
 *
 * Notes:
 * 65535 is maximum modbus address
 *
 * four types of memory:
 * Coils, read write
 * Discrete Inputs (Coils), read only
 * Holding Registers, read write
 * Input Registers, read only
 * */
#include <my_global.h>
#include <stdio.h>
#include <mysql.h>
#include <string.h>
#include <modbus.h>
#include <signal.h>
#include <unistd.h>
#include "rockbusDatabase.h"  // stored in libajrmutils directory
#include "rockbus.h"
#include "errors.h"
#include "main.h"
#include "utils.h"  // stored in libajrmutils directory
#include "sqlUtils.h"
#include "sqlFunctions.h"
#include "modbusUtils.h"

int main(int argc, char **argv) {

    initialise(argc, argv); // get the passed argument and initialise the MYSQL variables

    MYSQL *con = mysql_init(NULL); // Initialise MYSQL
    if (con == NULL)
    {
        fprintf(stderr, "mysql_init() failed\n");
        exit(EXIT_FAILURE);
    }

    opendb(con); // Open Database
    atexit(exitHandler); // establish an error handler to write errors to the database
    //d.debugSQL = LogAll;
    getDevice(con, d.debugSQL); // Get the device record from the database
    if(d.activate) {
        copyDeviceDataToModbusArray();
        initialiseStructures(con); // put the data from the database into the structures and vice versa

        // Read the ip address of the device from the database
        char ipAddress[LEN_ADDRESS];
        int mbPort;
        if (d.location==Internal) { // Internal IP address
            strlcpy(ipAddress, d.internalAddress, sizeof(ipAddress));
            mbPort = d.internalPort;
            if(d.debugRockbus) printf("Attempting to connect to %s, Port %i\n", ipAddress, mbPort);
        } else { // Device is external to local network
            hostname_to_ip(d.externalAddress,ipAddress); // translate the external address to an ip address
            mbPort = d.externalPort;
            if(d.debugRockbus) printf("Attempting to connect to %s -> %s, Port %i\n", d.externalAddress, ipAddress, mbPort);
        }
        modbus_t *mb = modbus_new_tcp(ipAddress, mbPort); // set up modbus
        startModbus(mb); // start the link to the modbus device
        if(d.debugRockbus) printf("Looping, listening for changes from Modbus or in Database\n");

        // Catch SIGTERM signal and shutdown gracefully
        if (signal(SIGTERM, sigHandler) == SIG_ERR) rbErr(RB_NO_SIGNAL);

        //Main Loop
        keepRunning = TRUE; // set by signal handler
        while(d.activate && keepRunning) { // Until told to stop by the flag in the database
            getModbus(mb); // get the data from the device
            if(d.modbusAccess==ReadWrite) getChangesFromDb(mb, con); // do this second so user initiated changes take precedence
            makeChangesToDb(con); // make changes from mudbus device to the database
            sleep(d.refreshPeriod); // sleep for a user defined period
            getDevice(con, d.debugSQL); // get the user data again from device table
            copyDeviceDataToModbusArray();
        }
        if(d.debugRockbus) printf("Rockbus exiting...\n");
        // Shutdown
        modbus_close(mb);
        modbus_free(mb);
        terminate(con); // Log that we are stopping in the database
    }
    mysql_close(con);
    exit(EXIT_SUCCESS);
}

void initialise(int argc, char **argv) {
    // get the passed argument and initialise the MYSQL variables
    if (argc==1) rbErr(RB_NO_DEVICE_ID);
    d.deviceKey = convertToInteger(argv[1]);
}
void terminate(MYSQL *con) {
    // Log that we are stopping in the database
    updateNumericDeviceData(con,"Status", Stopped+1, d.deviceKey, d.debugSQL);
    updateNumericDeviceData(con,"ProcessID", 0, d.deviceKey, d.debugSQL);
    updateNumericDeviceData(con,"RestartsAfterFailure", 0, d.deviceKey, d.debugSQL);
    updateCharDeviceData(con,"StatusDescription", "Normal system exit.", d.deviceKey, d.debugSQL);
}
void exitHandler() {
    // Have to attach to SQL and database again, as Exit handlers can't be passed the SQL connector variable
    MYSQL *con2 = mysql_init(NULL);
    if (con2 == NULL)
    {
        fprintf(stderr, "mysql_init() failed\n");
        exit(EXIT_FAILURE);
    }
    opendb(con2); // Open Database
    char buf[ERROR_TEXT_SIZE];
    enum status_e exitStatus;
    int utilsErrType = Success;
    int utilsErrno = 0;
    char errMsg[ERROR_TEXT_SIZE];
    getError(&utilsErrType, &utilsErrno, errMsg, sizeof(errMsg));
    switch (utilsErrType) {
    case Success:
        exitStatus = Stopped;
        strlcpy(buf,"Normal exit, no error.",sizeof(buf));
        break;
    case mySql:
        exitStatus = Failed;
        strlcpy(buf,"mySQL error:",sizeof(buf));
        strlcat(buf, errMsg, sizeof(buf));
        break;
    case Modbus:
        exitStatus = Failed;
        strlcpy(buf,"Modbus error:",sizeof(buf));
        strlcat(buf, errMsg, sizeof(buf));
        break;
    case Rockbus:
        exitStatus = Failed;
        strlcpy(buf,"Rockbus error:",sizeof(buf));
        strlcat(buf, errMsg, sizeof(buf));
        break;
    }
    // Update the database with the final status
    updateNumericDeviceData(con2,"Status", exitStatus+1, d.deviceKey, d.debugSQL);
    updateCharDeviceData(con2,"StatusDescription", buf, d.deviceKey, d.debugSQL);
    mysql_close(con2);
}
void initialiseStructures(MYSQL *con) {
    // Mostly debugging
    if(d.debugSQL) printf("SQL Debugging enabled\n");
    if(d.debugModbus) printf("Modbus Debugging enabled\n");
    if(d.debugRockbus) printf("Rockbus Debugging enabled\n");
    if(!d.activate) rbErr(RB_NOT_ACTIVATED);
    if(d.noInputRegisters+d.noRegisters+d.noInputCoils+d.noCoils==0) rbErr(RB_NOTHING_TO_DO);
    if(d.debugRockbus) {
        char timestampStr[TIMESTAMP_BUF_SIZE];
        convertTimeToSQL(getLocalTime(), timestampStr, sizeof(timestampStr));
        printf("%s Device to connect to: %s - %s\n", timestampStr, d.name, d.description);
        printf("Refresh period: %i\n" \
               "Starting address of input registers: %i\n" \
               "Number of input registers: %i\n" \
               "Starting address of registers: %i\n" \
               "Number of registers: %i\n" \
               "Starting address of input coils: %i\n" \
               "Number of input coils: %i\n" \
               "Starting address of coils: %i\n" \
               "Number of coils: %i\n" \
               "Address Base (whether to start registers at 0 or 1): %i\n"
               ,d.refreshPeriod, d.startInputRegister, d.noInputRegisters,
               d.startRegister, d.noRegisters, d.startInputCoil, d.noInputCoils,
               d.startCoil, d.noCoils, d.addressBase);
    }
    updateNumericDeviceData(con,"Status", Running+1,  d.deviceKey, d.debugSQL);
    updateCharDeviceData(con,"StatusDescription", "All systems functioning nominally, Captain",  d.deviceKey, d.debugSQL);

    // Initialise the previous values array with data that can't have been sent from Modbus
    // so that it treats the first values received as a "New Value".
    for (int i=0; i<NO_TYPES; i++) {
        for (int j=0; j<m[i].noAddresses; j++) {
            m[i].mbDataPrev[j] = HIGH_VALUE;
        }
    }
}

void rbErr(int rockbusErrno) {
    // exit with an internal error
    storeError(Rockbus, rockbusErrno, rockbus_strerror[rockbusErrno]);
    fprintf(stderr, "Internal Error %i in program %s\n",
            rockbusErrno, PROGRAM_NAME);
    fprintf(stderr, "%s\n", rockbus_strerror[rockbusErrno]);
    exit(EXIT_FAILURE);
}

void sigHandler(int sigNo){
		if (sigNo != SIGTERM) return;
	    if(d.debugRockbus) printf("SIGTERM signalled, exiting\n");
	    keepRunning = FALSE;
}

