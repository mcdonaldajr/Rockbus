#define PROGRAM_NAME "Zen V0.1"
#define CYCLE_TIME 5 // how long to sleep between each cycle
#define SLEEP_BEFORE_START 10 // how long to wait before starting to allow time for mysql to start up after boot
#define TIME_TO_SLEEP_AFTER_KILL 1 // give the child time to exit gracefully before updating the status
#define MAX_COMMAND_LENGTH 132
#define MAX_CHARS_FOR_INT 30
#define LAUNCH_ROCKBUS "/home/rockbus/rockbus"
#define ZEN_LOG "/home/rockbus/zen.log"
#define OLD_ZEN_LOG "/home/rockbus/oldzen.log"
#define TIMESTAMP_BUF_SIZE 20 // max size convertTimeToSQL will return
// Error Handling
struct debug_s {
    enum debugSQL_e SQL;
    enum bool_e Zen;
};
struct debug_s debug; //  Global structure to hold database parameters

enum errorCodes_e {
    ZEN_NO_ERROR,
    ZEN_SQL_ERROR,
    ZEN_INVALID_DEVICE_ID,
    ZEN_ERROR_IN_SCAN_DEVICES_TABLE,
    ZEN_FAILURE_TO_START_PROCESS,
    ZEN_NO_SIGNAL,
    ZEN_EXECL_FAIL,
    ZEN_MYSQL_INIT_FAIL,
    ZEN_NO_LOG
};

void zenErr(int zenErrno);
void logIt(char *message);
void debugLog(char *message, char *arg);

int keepRunning; // global variable that signal trap will set to cause shutdown
	FILE *lg;
