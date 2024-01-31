# @file Makefile
# @author[Ya know how it is, its your Name here] (your matrikelnummer) <[your matrikelnummer again but now with the e in front :O ]@student.tuwien.ac.at>
# @date 13.11.2023
#
# @brief compiles and linkes all the files
# 
 

all: forkFFT

flags = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g
libs = -lm

forkFFT.o: forkFFT.c
	gcc $(flags) -c forkFFT.c

forkFFT: forkFFT.o
	gcc -o forkFFT forkFFT.o $(libs)

clean:
	rm -f forkFFT forkFFT.o
