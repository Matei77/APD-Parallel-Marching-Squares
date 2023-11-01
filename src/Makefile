# Copyright: Ionescu Matei-Stefan - 333CAb - 2023-2024
CC = gcc
CFLAGS = -Wall -Wextra
LFLAGS = -lm -lpthread

#-------------------------------------------------------------------------------

.PHONY: build clean

#-------------------------------------------------------------------------------

build: tema1_par

#-------------------------------------------------------------------------------

tema1_par: tema1_par.o parallel_march.o utils.o helpers.o
	$(CC) -o $@ $^ $(CFLAGS) $(LFLAGS)

#-------------------------------------------------------------------------------

tema1_par.o: tema1_par.c types.h
	$(CC) -o $@ -c $< $(CFLAGS)

parallel_march.o: parallel_march.c parallel_march.h types.h
	$(CC) -o $@ -c $< $(CFLAGS)

utils.o: utils.c utils.h types.h
	$(CC) -o $@ -c $< $(CFLAGS)

helpers.o: helpers.c helpers.h
	$(CC) -o $@ -c $< $(CFLAGS)

#-------------------------------------------------------------------------------

clean:
	rm -f tema1_par tema1_par.o parallel_march.o utils.o helpers.o

#-------------------------------------------------------------------------------
