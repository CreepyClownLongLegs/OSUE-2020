# @file Makefile
# @author [Dein Vornmae :)] [Dein Nachname :O] (matrikelnummer) <e11927096@student.tuwien.ac.at>
# @date 13.11.2023
#
# @brief compiles and linkes all the files
# 

all: mygrep

flags = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g
libs = -lm

mygrep.o: mygrep.c
	gcc $(flags) -c mygrep.c

mygrep: mygrep.o
	gcc -o mygrep mygrep.o $(libs)

clean:
	rm -f mygrep mygrep.o