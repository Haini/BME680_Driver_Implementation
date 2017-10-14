all: main.c bme680.c bme680.h
	gcc bme680.c main.c -o bme680
