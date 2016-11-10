void getChangesFromDb(modbus_t *mb, MYSQL *con);
void makeChangesToDb(MYSQL *con);
int isRecordInDb(MYSQL *con, int addressNo, int ttc, int *value);

