void initialise(int argc, char **argv);
void initialiseStructures(MYSQL *con);
void terminate(MYSQL *con);
void exitHandler();
void sigHandler(int sigNo);
const char *type_e_str[] = {"InputRegister", "Register", "InputCoil", "Coil"};
