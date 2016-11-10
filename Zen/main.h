void initialise(int argc, char **argv);
void initialiseStructures(MYSQL *con);
void terminate(MYSQL *con);
void sigHandler(int sigNo);
void exitHandler();
int checkArgs(int argc, char **argv, char checkStr[]);
const char *type_e_str[] = {"InputRegister", "Register", "InputCoil", "Coil"};

