// Utilities that deal with reading and writing to the mysql database
#include <my_global.h>
#include <mysql.h>
#include <string.h>
#include <modbus.h>
#include "rockbusDatabase.h"
#include "zen.h"
#include "utils.h"
#include "sqlUtils.h"
#include "sqlFunctions.h"

void scanDevicesTable(MYSQL *con) {
    /*
    * PseudoCode
    * for each record in Devices where activate=true
    * 		switch (status)
    *  		case: Running
    * 			if lastSynchFromDevice recent then
    * 				All OK
    * 			else
    * 				set status to Hung
    * 		case: Stopped
    * 			spawn a rockbus process
    * 		case: Failed
    * 			set status to Stopped
    * 			increment counter for this device
    * 			if counter > maxTries
    * 				set activate to false
    * 			end if
    * 		case: Hung
    * 			kill process
    * 			set PID to null and status to Failed
    * 		end switch
    * next record
    */
    debugLog("Scanning device table","");
    char buf[COMMAND_SIZE];
    snprintf(buf, sizeof(buf),
             "SELECT DeviceKey " \
             "FROM Devices WHERE Activate='True';"
            );
    execSql(con, buf, debug.SQL);

    MYSQL_RES *result = mysql_store_result(con);
    if (result == NULL) {
        finish_with_error(con);
    }
    if (mysql_num_rows(result) > 0) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            int i=0;
            d.deviceKey = convertToInteger(row[i++]);
            // Get the device record from the database
            if(getDevice(con, debug.SQL)==0) zenErr(ZEN_INVALID_DEVICE_ID);
            debugLog("Checking device %s", d.name);
            switch(d.status) {
            case Running:
                debugLog("status Running.","");
                if (difftime (getLocalTime(), d.lastSynchFromDevice) > d.refreshPeriod + d.modbusTimeout) {
                    debugLog("%s seems to have hung",d.name);
                    updateNumericDeviceData(con,"Status", Hung+1, d.deviceKey, debug.SQL);
                } else {
                    debugLog("All OK.","");
                }
                break;
            case Stopped:
                // spawn process
                debugLog("Status Stopped, so will start %s",d.name);
                char deviceKeyString[MAX_CHARS_FOR_INT];
                convertToString(d.deviceKey, deviceKeyString, sizeof(deviceKeyString));

                int childPid=fork();
                if (childPid==0) { // then we are the child
                    debugLog("Zen execl: %s", d.name);
                    execl(LAUNCH_ROCKBUS, d.name, deviceKeyString, (char*)NULL);
                    zenErr(ZEN_EXECL_FAIL);
                } else {// we are the parent
                    char childPidString[MAX_CHARS_FOR_INT];
                    convertToString(childPid, childPidString, sizeof(childPidString));
                    debugLog("Child process created, ID: %s",childPidString);
                    d.processID=childPid;
                    updateNumericDeviceData(con,"ProcessID", childPid, d.deviceKey, debug.SQL);
                }
                break;

            case Failed:
                if (d.restartsAfterFailure < d.restartsPermitted) {
                    updateNumericDeviceData(con,"RestartsAfterFailure", ++d.restartsAfterFailure, d.deviceKey, debug.SQL);
                    debugLog("Status Failed, setting to Stopped to enable auto Restart.","");
                    updateNumericDeviceData(con,"Status", Stopped+1, d.deviceKey, debug.SQL);
                } else {
                    updateNumericDeviceData(con,"Activate", False+1, d.deviceKey, debug.SQL);
                    debugLog("Status Failed, Exceeded Restarts Permitted.\n " \
                             "Device %s will not start until reActivated", d.name);
                }
                break;
            case Hung:
                debugLog("%s Hung",d.name);
                if(kill(d.processID, SIGTERM)) {
                    updateCharDeviceData(con,"StatusDescription", "Failed to kill process, probably did not exist", d.deviceKey, d.debugSQL);
                    debugLog("Failed to kill process %s",d.name);

                } else {
                    debugLog("Killed child process %s",d.name);
                }
                sleep(TIME_TO_SLEEP_AFTER_KILL); // give the child time to exit gracefully before updating the status
                updateNumericDeviceData(con,"Status", Failed+1, d.deviceKey, debug.SQL);
                break;
            }
        }
    }
    mysql_free_result(result);
}





