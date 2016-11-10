// basic utilities like type conversion, time handling etc
// Note length (or siz)  should be passed as the sizeof(StringVariableToReturn).
// This helps stop buffer overflows.
#define ERROR_TEXT_SIZE 80
#define PROGRAM_NAME "ajrmutils lib Utilities V0.1"
#define ERROR_TYPE 0

#include <my_global.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "errors.h"
#include "errorCodes.h"

#include<stdlib.h> //for exit(0);
#include<sys/socket.h>
#include<errno.h> //For errno - the error number
#include<netdb.h>	//hostent
#include<arpa/inet.h>
static int storedUtilsErrType=Success;
static int storedUtilsErrno=0;
static char storedErrMsg[ERROR_TEXT_SIZE];
void storeError(int utilsErrType, int utilsErrno, char errMsg[]){
	storedUtilsErrType = utilsErrType;
	storedUtilsErrno = utilsErrno;
	strlcpy(storedErrMsg, errMsg, sizeof(storedErrMsg));
}
int getError(int *utilsErrType, int *utilsErrno, char errMsg[], int sizeErrMsg){
	*utilsErrType = storedUtilsErrType;
	*utilsErrno = storedUtilsErrno;
	strlcpy(errMsg, storedErrMsg, sizeErrMsg);
	return(storedUtilsErrno);
}
void utilErr(int utilsErrno) {
// exit with an internal error
    storeError(Utility, utilsErrno, errors_strerror[utilsErrno]);
    fprintf(stderr, "Internal Error %i in %s\n", utilsErrno, PROGRAM_NAME);
    fprintf(stderr, "%s\n", errors_strerror[utilsErrno]);
    exit(EXIT_FAILURE);
}
int hostname_to_ip(char *hostname, char *ip){
	// convert the passed hostname in string form to an IP address
	struct hostent *he;
	struct in_addr **addr_list;
	int i;	
	if ( (he = gethostbyname( hostname ) ) == NULL) 
	{
		// get the host info
		herror("gethostbyname");
		return 1;
	}
	addr_list = (struct in_addr **) he->h_addr_list;
	for(i = 0; addr_list[i] != NULL; i++) 
	{
		//Return the first one;
		strcpy(ip , inet_ntoa(*addr_list[i]) );
		return 0;
	}
	return 1;
}

void getTime(char dateTimeRet[], int length) {
// put the local time into the passed char variable.
    struct tm *local;
    time_t t;

    t = time(NULL);
    local = localtime(&t);
    strftime (dateTimeRet,length,"%F %T",local);
}
time_t getLocalTime(){
	// return the local time in time_t format
	struct tm *local;
	time_t t;
	t = time(NULL);
	local = localtime(&t);
	t = timegm(local);
	if (t == -1) utilErr(UTILS_GETLOCALTIME_FAIL);
	return(t);
}

time_t convertTimeFromSQL(char passedDateTime[]) {
	// convert date and time in mysql format to time_t format
    time_t t;
    struct tm result;
    if(passedDateTime==NULL) utilErr(UTILS_CONVERT_NULL_DATE);

    //  date format from SQL is: 2000-01-01 00:00:00
    if (strptime (passedDateTime, "%Y-%m-%d %T", &result) == NULL)
    utilErr(UTILS_STRPTIME_FAIL);

    t = timegm(&result);
    if (t == -1) utilErr(UTILS_TIMEGM_FAIL);
    return(t);
}

void convertTimeToSQL(time_t passedDateTime, char dateTimeRet[], int lenDateTimeRet) {
	// reverse of convertTimeFromSQL
    struct tm *tmp;
    tmp = gmtime(&passedDateTime);
    if (strftime (dateTimeRet, lenDateTimeRet, "%Y-%m-%d %T", tmp) == 0) utilErr(UTILS_STRFTIME_FAIL);
}

void convertToString(int passedValue, char retString[], size_t siz){
	// converts integer to string
	snprintf(retString, siz, "%d", passedValue);
}

int convertToInteger(char passedString[]){
	// converts string to integer, catches NULL values as error
	if(passedString==NULL) utilErr(UTILS_INTEGER_NULL);
	return(atoi(passedString));
}

time_t maxTime(time_t arg1, time_t arg2){
	// returns latest date time 
	return(arg1 > arg2 ? arg1:arg2);
}

size_t cps(char *dst, const char *src, size_t siz) {
	// copy string, return "" if src NULL
	if(src==NULL){
		return(strlcpy(dst, "", siz));
	} else {
		return(strlcpy(dst, src, siz));
	}
}

size_t cat(char *dst, const char *src, size_t siz) {
	// concatenate string, return "" if src NULL
	if(src==NULL){
		return(strlcat(dst, "", siz));
	} else {
		return(strlcat(dst, src, siz));
	}
}
size_t strlcpy(char *dst, const char *src, size_t siz) {
	// copies string, uses siz variable to catch buffer overflows
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}
	return(s - src - 1);	/* count does not include NUL */
}

size_t strlcat(char *dst, const char *src, size_t siz){
	// as per strlcpy but concatenates
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (*d != '\0' && n-- != 0)
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}
