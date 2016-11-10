CC=gcc
INCLUDES = ${shell mysql_config --include} 
CFLAGS=-std=c99 -c -Wall -I/usr/include/modbus -I"/home/pi/Documents/Rockbus/libajrmutils" `mysql_config --cflags` 
LDFLAGS= -lmodbus `mysql_config --libs` -L"/home/pi/Documents/Rockbus/libajrmutils/" -lajrmutils 

OBJECTS = main.o \
          sqlFunctions.o \
          modbusUtils.o

main : $(OBJECTS)
	$(CC) -o main   $(OBJECTS) $(LDFLAGS)

main.o: main.h sqlFunctions.h modbusUtils.h errors.h rockbus.h
sqlFunctions.o: sqlFunctions.h modbusUtils.h rockbus.h errors.h
modbusUtils.o: modbusUtils.h rockbus.h

.PHONY: clean
clean:
	rm main $(OBJECTS)
