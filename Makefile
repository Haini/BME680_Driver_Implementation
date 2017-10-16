CC=gcc
CFLAGS= -O3 -Wall
DEPS = bme680.h
OBJ = main.o bme680.o
LIB = libalgobsec.a
LIB_PATH = /root/BME680_Driver_Implementation/ 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(FLAGS)

all: $(OBJ) 
	$(CC) $(CFLAGS) -I$(LIB_PATH) -L$(LIB_PATH)  -o bme680 $^

.PHONY: clean

clean:
	rm *.o bme680 
