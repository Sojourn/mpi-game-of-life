#
#	File:		Makefile
#	Authors:	Joel Rausch && Scott Connell
#
#	This file made for CprE 426, Programming Assignment 3.
#	Iowa State University
#
#	Date:		17 April 2013 at 19:43
#

CC=mpicxx
PROG=life

######################
NP=8
IFILE=input.txt
OFILE=output.txt
######################

CFILES= Main.cpp		\
	LifeUtil.cpp		\
	AsyncIO.cpp


all:	${CFILES}
	${CC} -o ${PROG} ${CFILES}
	g++ -o serial Serial.cpp LifeUtil.cpp

clean: 
	rm -f *.o
	rm -f ${PROG}
	rm -f serial

#
# command line arguments to run:
#		mpirun -np <np> <executable> <input_file> <output_file>
#
run:
	mpirun -np ${NP} ${PROG} ${IFILE} ${OFILE}
