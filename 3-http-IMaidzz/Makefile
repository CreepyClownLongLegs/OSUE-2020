# @file Makefile
# @author Maida Horozovic <e11927096@student.tuwien.ac.at>
# @date 15.01.2024
#
# @brief The Makefile for the server.

CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS) -fdiagnostics-color=always
LDFLAGS = -lrt

SERVER_OBJECTS = server.o

.PHONY: all clean release 
all: server

server: $(SERVER_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o server server.tgz

# Create the archive to submit 
release:
	tar -cvzf server.tgz Makefile server.c
