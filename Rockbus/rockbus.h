#define PROGRAM_NAME "Rockbus V0.1"
#define REGISTERS 255 // Maximum number of Registers supported
#define COILS 255 // Maximum number of Coils supported
#define HIGH_VALUE 65535 // used to initialise previous values array, must be a value that can't be sent from Modbus

#define NO_TYPES 4

struct modBusData_s {
    uint16_t mbData[REGISTERS];
    uint16_t mbDataPrev[REGISTERS];
    int mbChanged[REGISTERS];
    time_t timestamped[REGISTERS];
    int startAddress;
    int noAddresses;
};
struct modBusData_s m[NO_TYPES];	// Global structure to hold modbus data from
//device, size is last item in enum type_e (i.e. Coils)
// Error Handling
enum errorCodes_e {
    RB_NO_ERROR,
    RB_NO_DEVICE_ID,
    RB_INVALID_DEVICE_ID,
    RB_NOT_ACTIVATED,
    RB_INCORRECT_NO_REGISTERS,
    RB_INCORRECT_NO_COILS,
    RB_ATTEMPTED_WRITE_TO_INPUT_REGISTER,
    RB_ATTEMPTED_WRITE_TO_INPUT_COIL,
    RB_NOTHING_TO_DO,
    RB_MODBUS_ERROR,
    RB_SQL_ERROR,
    RB_NO_SIGNAL
};

void rbErr(int rockbusErrno);
const char *type_e_str[4];
int keepRunning; // global variable that signal trap will set to cause shutdown
