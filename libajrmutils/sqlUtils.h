void finish_with_error(MYSQL *con);
void execSql (MYSQL *con, char sqlCommand[], int debugSQL);
void opendb(MYSQL *con);
void updateNumericDeviceData(MYSQL *con, char field[], int value, int deviceKey, int debugSQL);
void updateCharDeviceData(MYSQL *con, char field[], char value[], int deviceKey, int debugSQL);
int getNumericDeviceData(MYSQL *con, char field[], int *value, int deviceKey, int debugSQL);
int getCharDeviceData(MYSQL *con, char field[], char value[], int sizeOfValue, int deviceKey, int debugSQL);
int getDevice(MYSQL *con, int debugSQL);
