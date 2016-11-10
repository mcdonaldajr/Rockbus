CC=gcc
INCLUDES = ${shell mysql_config --include}
CFLAGS=-std=c99 -c -Wall `mysql_config --cflags`

OBJECTS = utils.o sqlUtils.o

libajrmutils : $(OBJECTS)
	$(CC) -c $(OBJECTS)
	  ar rc libajrmutils.a $(OBJECTS)
	  ranlib libajrmutils.a
utils.o: utils.h errors.h errorCodes.h
sqlUtils.o: utils.h errors.h errorCodes.h

.PHONY: clean
clean:
	rm libajrmutils.a $(OBJECTS)
