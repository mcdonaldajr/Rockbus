#include <my_sys.h>
#include <stdio.h>
#include <modbus.h>
#include "rockbusDatabase.h" 
#include "rockbus.h"
#include "utils.h"
#include "modbusUtils.h"

void modbus_finish_with_error() {
	// writes the modbus error to the error stream and exits
	storeError(Modbus, errno, (char*)modbus_strerror(errno));
    fprintf(stderr, "Modbus Error %i in program %s\n",
            errno, PROGRAM_NAME);
    fprintf(stderr, "%s\n", modbus_strerror(errno));
    exit(EXIT_FAILURE);
}

void startModbus(modbus_t *mb) {
	// starts up modbus and connects to device specified in device table record "d"
	
    if (d.debugModbus) modbus_set_debug(mb, TRUE);
    else modbus_set_debug(mb, FALSE);
    if(modbus_connect(mb)) modbus_finish_with_error();
    struct timeval old_response_timeout;
    struct timeval response_timeout;

    // Save original timeout
    modbus_get_response_timeout(mb, &old_response_timeout);
    if (d.debugModbus) printf("Modbus Timeout was: %i Setting it to %i\n",(int)old_response_timeout.tv_sec, d.modbusTimeout);

    // Define a new timeout
    response_timeout.tv_sec = d.modbusTimeout; // get from devices table
    response_timeout.tv_usec = 0;
    modbus_set_response_timeout(mb, &response_timeout);
}

void getModbus(modbus_t *mb) {
// gets data from modbus
	int ttg; // Type to get
    // InputRegister:
    ttg=InputRegister;
    if (m[ttg].noAddresses > 0) {
        int noRegsRead = modbus_read_input_registers(mb, m[ttg].startAddress-d.addressBase, m[ttg].noAddresses, m[ttg].mbData);
        if(noRegsRead==-1) modbus_finish_with_error();
        if(noRegsRead!=d.noRegisters) rbErr(RB_INCORRECT_NO_REGISTERS);
        checkForModbusChanges(ttg); // need to force these true for 1st time through!
    }
    // Register:
    ttg=Register;
    if (m[ttg].noAddresses > 0) {
        int noRegsRead = modbus_read_registers(mb, m[ttg].startAddress-d.addressBase, m[ttg].noAddresses, m[ttg].mbData);
        if(noRegsRead==-1) modbus_finish_with_error();
        if(noRegsRead!=d.noRegisters) rbErr(RB_INCORRECT_NO_REGISTERS);
        checkForModbusChanges(ttg);
    }
    // InputCoil:
    ttg=InputCoil;
    if (m[ttg].noAddresses > 0) {
        uint8_t coils[COILS];
        int noRegsRead = modbus_read_input_bits(mb, m[ttg].startAddress-d.addressBase, m[ttg].noAddresses, coils);
        if(noRegsRead==-1) modbus_finish_with_error();
        if(noRegsRead!=d.noRegisters) rbErr(RB_INCORRECT_NO_REGISTERS);
        for (int i=0; i<m[ttg].noAddresses; i++) m[ttg].mbData[i] = coils[i];
        checkForModbusChanges(ttg);
    }
    // Coil:
    ttg=Coil;
    if (m[ttg].noAddresses > 0) {
        uint8_t coils[COILS];
        int noRegsRead = modbus_read_bits(mb, m[ttg].startAddress-d.addressBase, m[ttg].noAddresses, coils);
        if(noRegsRead==-1) modbus_finish_with_error();
        if(noRegsRead!=d.noRegisters) rbErr(RB_INCORRECT_NO_REGISTERS);
        for (int i=0; i<m[ttg].noAddresses; i++) m[ttg].mbData[i] = coils[i];
        checkForModbusChanges(ttg);
    }

}
void checkForModbusChanges(int ttc) { // ttc is type to check
// scans through latest modbus data and flags those that have changed since last read
    if(m[ttc].noAddresses>0) {
        int arrayIndex = 0;
        for (int addressNo=m[ttc].startAddress; addressNo<m[ttc].startAddress+m[ttc].noAddresses; addressNo++) {
            if (m[ttc].mbData[arrayIndex] == m[ttc].mbDataPrev[arrayIndex]) {
                m[ttc].mbChanged[arrayIndex] = FALSE;
            } else {
/*                if (d.debugRockbus) printf(
                        "In routine: checkForModbusChanges checking Address %i, type %s was %i, is now %i\n",
                        addressNo, type_e_str[ttc], m[ttc].mbDataPrev[arrayIndex], m[ttc].mbData[arrayIndex]);
*/
                m[ttc].mbChanged[arrayIndex] = TRUE; // remember it has changed so we can change database
                m[ttc].mbDataPrev[arrayIndex] = m[ttc].mbData[arrayIndex]; // remember the value so we can check if it changes again
                m[ttc].timestamped[arrayIndex] = getLocalTime(); // remember the time it changed
            }
            arrayIndex++;
        }
    }
}

void putModbus(modbus_t *mb, int ttp, int address, int value) { // ttp is type to put
// writes out single modbus register or coil
    if (d.debugRockbus) printf(
            "In routine: putModbus writing Address %i, Type %s oldvalue %i newvalue %i\n",
            address, type_e_str[ttp], m[ttp].mbData[address - m[ttp].startAddress], value);
    switch (ttp) {
    case InputRegister:
        rbErr(RB_ATTEMPTED_WRITE_TO_INPUT_REGISTER);
        break;
    case Register:
        m[ttp].mbChanged[address - m[ttp].startAddress] = FALSE; // catch the change from db to modbus, not the other way!
        m[ttp].mbDataPrev[address - m[ttp].startAddress] = value; // ensure this database caused change is not recorded as a Modbus change
		m[ttp].timestamped[address - m[ttp].startAddress] = time(NULL);
        if(modbus_write_register(mb, address-d.addressBase, value)==-1) modbus_finish_with_error();
        break;
    case InputCoil:
        rbErr(RB_ATTEMPTED_WRITE_TO_INPUT_COIL);
        break;
    case Coil:
        m[ttp].mbChanged[address - m[ttp].startAddress] = FALSE; // catch the change from db to modbus, not the other way!
        m[ttp].mbDataPrev[address - m[ttp].startAddress] = value; // ensure this database caused change is not recorded as a Modbus change
		m[ttp].timestamped[address - m[ttp].startAddress] = time(NULL);
        if(modbus_write_bit(mb, address-d.addressBase, value)==-1) modbus_finish_with_error();
        break;
    }
}
void copyDeviceDataToModbusArray(){
    m[InputRegister].startAddress = d.startInputRegister;
    m[InputRegister].noAddresses = d.noInputRegisters;
    m[Register].startAddress = d.startRegister;
    m[Register].noAddresses = d.noRegisters;
    m[InputCoil].startAddress = d.startInputCoil;
    m[InputCoil].noAddresses = d.noInputCoils;
    m[Coil].startAddress = d.startCoil;
    m[Coil].noAddresses = d.noCoils;
}
