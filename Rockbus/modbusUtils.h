void startModbus(modbus_t *mb);
void getModbus(modbus_t *mb);
void checkForModbusChanges();
void putModbus(modbus_t *mb, int ttp, int address, int value);
void copyDeviceDataToModbusArray();
