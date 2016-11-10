// Functions that deal with reading and writing to the mysql database
#include <my_global.h>
#include <mysql.h>
#include <string.h>
#include <modbus.h>
#include "rockbusDatabase.h" 
#include "rockbus.h"
#include "utils.h"
#include "sqlUtils.h"
#include "sqlFunctions.h"
#include "modbusUtils.h"

void getChangesFromDb(modbus_t *mb, MYSQL *con) {
    /*
     * PseudoCode
     * for each record in ModbusExchange that has changed since last time we looked
     * 	if the timestamp on the database record is later than the timestamp on the matching modbus address data
     * 	then send the change to the device over modbus
     * 	end if
     * next record
     * update LastSynchToDevice time to now
     */
    char buf[COMMAND_SIZE];
    snprintf(buf, sizeof(buf),
             "SELECT Address, Type-1, Value, Timestamped " \
             "FROM ModbusExchange WHERE DeviceKey=%i " \
             "AND (TYPE='Register' OR TYPE = 'Coil')" \
             "AND TimeStamped > (Select LastSynchToDevice from Devices where DeviceKey=%i)" \
             "ORDER BY Timestamped;",
             d.deviceKey, d.deviceKey);
    execSql(con, buf, d.debugSQL);

    MYSQL_RES *result = mysql_store_result(con);
    if (result == NULL) {
        finish_with_error(con);
    }
    if (mysql_num_rows(result) > 0) {
        MYSQL_ROW row;
        int changesMade = FALSE;
        time_t maxTimestamp;
        while ((row = mysql_fetch_row(result))) {
            int i=0;
            int addressNo = convertToInteger(row[i++]);
            int theType = convertToInteger(row[i++]);
            int value = convertToInteger(row[i++]);
            time_t dbTimestamp = convertTimeFromSQL(row[i++]);

            if(dbTimestamp > m[theType].timestamped[addressNo - m[theType].startAddress]) {
				// if the timestamp on the database > timestamp on the modbus register
                maxTimestamp = dbTimestamp; //  record each one, the last recorded will be the latest
                if ((d.debugRockbus)) {
                    char modbusTime[TIMESTAMP_BUF_SIZE];
                    convertTimeToSQL(m[theType].timestamped[addressNo - m[theType].startAddress],modbusTime,sizeof(modbusTime));
                    char dbTime[TIMESTAMP_BUF_SIZE];
                    convertTimeToSQL(dbTimestamp,dbTime,sizeof(dbTime));
                    printf("In routine getChangesFromDb, Address %i has value %i on db of type %s db timestamp %s Modbus %s\n",
                           addressNo, value, type_e_str[theType], dbTime, modbusTime);
                }
                putModbus(mb, theType, addressNo, value); // update the modbus register
                changesMade = TRUE;
            }
        }
        mysql_free_result(result);
        if(changesMade) { // then record the time of the latest entry on the database in Devices.LastSynchToDevice
            char maxTimestampStr[TIMESTAMP_BUF_SIZE];
            convertTimeToSQL(maxTimestamp, maxTimestampStr, sizeof(maxTimestampStr));
            snprintf(buf, sizeof(buf),
                     "UPDATE Devices SET Devices.LastSynchToDevice='%s' WHERE DeviceKey=%i;",
                     maxTimestampStr, d.deviceKey);
            execSql(con, buf, d.debugSQL);
        }
    } else {
        mysql_free_result(result);
    }
}

void makeChangesToDb(MYSQL *con) {
    /*
     * PseudoCode:
     * for each device type ("InputRegister", "Register", "InputCoil", "Coil")
     * 	 for each modbus address
     * 		if modbus register or coil data has changed
     * 			if the matching record is in the database
     * 				if the modbus data is different to the database data
     * 					update the database data
     *              endif
     * 			else
     * 				insert a new record in the database with the new modbus data
     * 			endif
     * 			update the timestamp on the modbus data to now
     * 		end if
     *	next modbus address
     * update LastSynchFromDevice to now
     */
    char buf[COMMAND_SIZE];
    for (int ttc=0; ttc<NO_TYPES; ttc++) { // ttc is type to check- coil, register etc
        int arrayIndex = 0;
        for (int addressNo=m[ttc].startAddress; addressNo<=m[ttc].startAddress+m[ttc].noAddresses; addressNo++) { // for each modbus register
            if (m[ttc].mbChanged[arrayIndex]) { // if it has changed since last checked
                int value=0;
                if (isRecordInDb(con, addressNo, ttc, &value)) { // if record exists update it
                    if(value != m[ttc].mbData[arrayIndex]) {
                        // update database
                        if(d.debugRockbus) printf("In makeChangesToDb, address %i type %s updated from %i to %i\n",
                                                      addressNo, type_e_str[ttc], value, m[ttc].mbData[arrayIndex]);
                        snprintf(buf, sizeof(buf),
                                 "UPDATE ModbusExchange SET ModbusExchange.Value=%i WHERE Address=%i AND Type='%s' AND DeviceKey=%i;",
                                 m[ttc].mbData[arrayIndex], addressNo, type_e_str[ttc], d.deviceKey);
                        execSql(con, buf, d.debugSQL);
                    }
                } else { // insert into database
                    if(d.debugRockbus) printf("In makeChangesToDb, address %i type %s value %i inserted\n",
                                                  addressNo, type_e_str[ttc], m[ttc].mbData[arrayIndex]);
                    snprintf(buf, sizeof(buf),
                             "INSERT INTO ModbusExchange VALUES (NULL,%i,%i,'%s',%i,NULL) ON DUPLICATE KEY UPDATE Value=%i;",
                             d.deviceKey, addressNo, type_e_str[ttc], m[ttc].mbData[arrayIndex], m[ttc].mbData[arrayIndex]);
                    execSql(con, buf, d.debugSQL);
                }
                m[ttc].timestamped[arrayIndex] = getLocalTime(); // Record the time updated
            }
            arrayIndex++;
        }
    }
    // then record the time in Devices.LastSynchFromDevice
    char currentTime[TIMESTAMP_BUF_SIZE];
    getTime(currentTime, sizeof(currentTime));
    snprintf(buf, sizeof(buf),
             "UPDATE Devices SET Devices.LastSynchFromDevice='%s' WHERE DeviceKey=%i;",
             currentTime, d.deviceKey);
    execSql(con, buf, d.debugSQL);
}

int isRecordInDb(MYSQL *con, int addressNo, int ttc, int *value) { // ttc is type to check, returns value
    // looks up the modbusExchange record on the database and returns TRUE if it exists, also returns value
    int valueFound = FALSE;
    char buf[COMMAND_SIZE]="";
    //  if (d.debugRockbus) printf("In routine isRecordInDb, searching in db for Device %i Address %i type %s\n", d.deviceKey, addressNo, type_e_str[ttc]);
    snprintf(buf, sizeof(buf),
             "SELECT Value FROM ModbusExchange where Address=%i AND Type='%s' AND DeviceKey=%i;",
             addressNo, type_e_str[ttc], d.deviceKey);
    execSql(con, buf, d.debugSQL);

    MYSQL_RES *result = mysql_store_result(con);
    if (result == NULL)
    {
        finish_with_error(con);
    }

    MYSQL_ROW row;

    while ((row = mysql_fetch_row(result)))
    {
        *value = convertToInteger(row[0]);
        valueFound = TRUE;
        //      if (d.debugRockbus) printf("In routine isRecordInDb, Address %i type %s has value %i\n", addressNo, type_e_str[ttc], *value);
    }
    mysql_free_result(result);

    return(valueFound);
}

