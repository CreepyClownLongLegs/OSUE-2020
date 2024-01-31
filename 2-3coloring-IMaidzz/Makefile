# @file Makefile
# @author Name Surname ?!?! <matrikelnummer@student.tuwien.ac.at>
# @date 11.12.2023
# @brief Makefile which compiles files

CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS) -fdiagnostics-color=always
LDFLAGS = -lrt -pthread

all: generator supervisor

all-slow: CFLAGS += -DSLOW_ALGO
all-slow: all

generator: generator.o
	$(CC) $(LDFLAGS) -o $@ $^

supervisor: supervisor.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o generator supervisor 3coloring.tgz

release:
	tar -cvzf 3coloring.tgz Makefile *.c *.h
