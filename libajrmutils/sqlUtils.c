// Utilities that deal with reading and writing to the mysql database
#define PROGRAM_NAME "Rockbus"
#include <my_global.h>
#include <mysql.h>
#include <string.h>
#include "rockbusDatabase.h"
#include "utils.h"
#include "sqlUtils.h"
#include "errorCodes.h"
void finish_with_error(MYSQL *con) {
    // Called if a MySQL error has occured, logs error and exits program.
    storeError(mySql, 0, (char*)mysql_error(con));
    fprintf(stderr, "SQL Error\n");
    fprintf(stderr, "%s\n", mysql_error(con));
    mysql_close(con);
    exit(EXIT_FAILURE);
}

void execSql (MYSQL *con, char sqlCommand[], int debugSQL) {
    // executes passed SQL command, then checks for error
    if ((debugSQL == LogChanges)||(debugSQL == LogAll)) {
        if(strncmp ("UPDATE",sqlCommand,6)==0) printf("%s\n", sqlCommand);
        if(strncmp ("INSERT",sqlCommand,6)==0) printf("%s\n", sqlCommand);
    }
    if (debugSQL == LogAll) {
        if(strncmp ("SELECT",sqlCommand,6)==0) printf("%s\n", sqlCommand);
    }
    if (mysql_query(con, sqlCommand)) finish_with_error(con);
}

void opendb(MYSQL *con) {
    static char *opt_host_name = NULL; /* server host (default=localhost) */
    static char *opt_user_name = NULL; /* username (default=login name) */
    static char *opt_password = NULL; /* password (default=none) */
    static unsigned int opt_port_num = 0; /* port number (use built-in value) */
    static char *opt_socket_name = NULL; /* socket name (use built-in value) */
    static char *opt_db_name = NULL; /* database name (default=none) */
    static unsigned int opt_flags = 0; /* connection flags (none) */
    mysql_options(con,MYSQL_READ_DEFAULT_GROUP,PROGRAM_NAME);
    if (mysql_real_connect (con, opt_host_name, opt_user_name, opt_password,
                            opt_db_name, opt_port_num, opt_socket_name, opt_flags) == NULL)
    {
        finish_with_error(con);
    }
}

void updateNumericDeviceData(MYSQL *con, char field[], int value, int deviceKey, int debugSQL) {
    char buf[COMMAND_SIZE];
    snprintf(buf, sizeof(buf),
             "UPDATE Devices SET Devices.%s=%i WHERE DeviceKey=%i;",
             field, value, deviceKey);
    execSql(con, buf, debugSQL);
}
void updateCharDeviceData(MYSQL *con, char field[], char value[], int deviceKey, int debugSQL) {
    char buf[COMMAND_SIZE];
    snprintf(buf, sizeof(buf),
             "UPDATE Devices SET Devices.%s='%s' WHERE DeviceKey=%i;",
             field, value, deviceKey);
    execSql(con, buf, debugSQL);
}

int getNumericDeviceData(MYSQL *con, char field[], int *value, int deviceKey, int debugSQL) {
    char buf[COMMAND_SIZE];
    snprintf(buf, sizeof(buf),
             "SELECT %s FROM Devices WHERE DeviceKey=%i LIMIT 1;",
             field, deviceKey);
    execSql(con, buf, debugSQL);

    MYSQL_RES *result = mysql_store_result(con);
    if (result == NULL) {
        finish_with_error(con);
    }
    if (mysql_num_rows(result) > 0) {
        MYSQL_ROW row;
        row = mysql_fetch_row(result);
        *value = convertToInteger(row[0]);
        mysql_free_result(result);
        return(UTILS_NO_ERROR);
    } else {
        mysql_free_result(result);
        return(UTILS_NOT_FOUND);
    }
}
int getCharDeviceData(MYSQL *con, char field[], char value[], int sizeOfValue, int deviceKey, int debugSQL) {
    char buf[COMMAND_SIZE];
    snprintf(buf, sizeof(buf),
             "SELECT %s FROM Devices WHERE DeviceKey=%i LIMIT 1;",
             field, deviceKey);
    execSql(con, buf, debugSQL);

    MYSQL_RES *result = mysql_store_result(con);
    if (result == NULL) {
        finish_with_error(con);
    }
    if (mysql_num_rows(result) > 0) {
        MYSQL_ROW row;
        row = mysql_fetch_row(result);
        cps(value, row[0], sizeOfValue);
        mysql_free_result(result);
        return(UTILS_NO_ERROR);
    } else {
        mysql_free_result(result);
        return(UTILS_NOT_FOUND);
    }
    return(UTILS_NO_ERROR);
}

int getDevice(MYSQL *con, int debugSQL) {
    // Retrieves the entire device record from database and stores it in global structure "d" for device
    char buf[COMMAND_SIZE];
    char *sqlCommand = "SELECT " \
                       "DeviceKey," \
                       "Name," \
                       "Description," \
                       "Activate-1," \
                       "Status-1," \
                       "StatusDescription," \
                       "RestartsPermitted," \
                       "RestartsAfterFailure," \
                       "ProcessID," \
                       "LastSynchToDevice," \
                       "LastSynchFromDevice," \
                       "ModbusAccess-1," \
                       "RefreshPeriod," \
                       "ModbusTimeout," \
                       "StartInputRegister," \
                       "NoInputRegisters," \
                       "StartRegister," \
                       "NoRegisters," \
                       "StartInputCoil," \
                       "NoInputCoils," \
                       "StartCoil," \
                       "NoCoils," \
                       "AddressBase," \
                       "Location-1,"
                       "InternalAddress," \
                       "InternalPort," \
                       "ExternalAddress," \
                       "ExternalPort," \
                       "Timestamped, " \
                       "DebugSQL-1, " \
                       "DebugModbus-1, " \
                       "DebugRockbus-1 " \
                       "FROM Devices WHERE DeviceKey=";
    /* subtract one from enums as SQL starts enums at 1, and C starts at 0 */
    snprintf(buf, sizeof(buf), "%s%i LIMIT 1;",sqlCommand,d.deviceKey);
    execSql(con, buf, debugSQL);
    MYSQL_RES *result = mysql_store_result(con);
    if (result == NULL) finish_with_error(con);
    int noRows = mysql_num_rows(result);
    if (noRows == 0) return(noRows);
    MYSQL_ROW row;
    row = mysql_fetch_row(result);
    int i=0;
    d.deviceKey=convertToInteger(row[i++]);
    cps(d.name, row[i++], sizeof(d.name));
    cps(d.description, row[i++], sizeof(d.description));
    d.activate	= convertToInteger(row[i++]);
    d.status = convertToInteger(row[i++]);
    cps(d.statusDescription, row[i++], sizeof(d.statusDescription));
    d.restartsPermitted = convertToInteger(row[i++]);
    d.restartsAfterFailure = convertToInteger(row[i++]);
    d.processID = convertToInteger(row[i++]);
    d.lastSynchToDevice = convertTimeFromSQL(row[i++]);
    d.lastSynchFromDevice = convertTimeFromSQL(row[i++]);
    d.modbusAccess = convertToInteger(row[i++]);
    d.refreshPeriod = convertToInteger(row[i++]);
    d.modbusTimeout = convertToInteger(row[i++]);
    d.startInputRegister= convertToInteger(row[i++]);
    d.noInputRegisters= convertToInteger(row[i++]);
    d.startRegister= convertToInteger(row[i++]);
    d.noRegisters= convertToInteger(row[i++]);
    d.startInputCoil= convertToInteger(row[i++]);
    d.noInputCoils= convertToInteger(row[i++]);
    d.startCoil= convertToInteger(row[i++]);
    d.noCoils= convertToInteger(row[i++]);
    d.addressBase= convertToInteger(row[i++]);
    d.location= convertToInteger(row[i++]);
    cps(d.internalAddress, row[i++], sizeof(d.internalAddress));
    d.internalPort= convertToInteger(row[i++]);
    cps(d.externalAddress, row[i++], sizeof(d.externalAddress));
    d.externalPort= convertToInteger(row[i++]);
    d.timestamped = convertTimeFromSQL(row[i++]);
    d.debugSQL = convertToInteger(row[i++]);
    d.debugModbus = convertToInteger(row[i++]);
    d.debugRockbus = convertToInteger(row[i++]);
    d.daysKeepLog = convertToInteger(row[i++]);
    mysql_free_result(result);
    return(noRows);
}

