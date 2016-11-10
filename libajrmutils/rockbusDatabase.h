/*
#define SQL_SERVER "localhost"
#define SQL_ACCOUNT "root"
#define SQL_PASSWORD "hermes"
#define DATABASE_NAME "Rockbus"
*/
#define INT_BUF_SIZE 20 // max size convertToString will return
#define TIMESTAMP_BUF_SIZE 20 // max size convertTimeToSQL will return
#define COMMAND_SIZE 1024 // max length of SQL command
#define ERROR_TEXT_SIZE 80
#define LEN_ADDRESS 46 // for Ip and server addresses

enum debugSQL_e {LogNone, LogChanges, LogAll};
enum bool_e {False, True};
enum type_e {InputRegister, Register, InputCoil, Coil};
enum status_e {Stopped, Running, Failed, Hung};
enum location_e {Internal, External};
enum access_e {ReadOnly, ReadWrite};

struct devices_s {
    int deviceKey;
    char name[16];
    char description[46];
    enum bool_e activate;
    enum status_e status;
    char statusDescription[ERROR_TEXT_SIZE];
    int restartsPermitted;
    int restartsAfterFailure;
    int processID;
    time_t lastSynchToDevice;
    time_t lastSynchFromDevice;
    int modbusAccess;
    int refreshPeriod;
    int modbusTimeout;
    int startInputRegister;
    int noInputRegisters;
    int startRegister;
    int noRegisters;
    int startInputCoil;
    int noInputCoils;
    int startCoil;
    int noCoils;
    int addressBase;
    int location;
    char internalAddress[LEN_ADDRESS];
    int internalPort;
    char externalAddress[LEN_ADDRESS];
    int externalPort;
    time_t timestamped;
    enum debugSQL_e debugSQL;
    enum bool_e debugModbus;
    enum bool_e debugRockbus;
    int daysKeepLog;
};
struct devices_s d; // Global structure to hold current devices row from database
