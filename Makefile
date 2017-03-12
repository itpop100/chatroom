# Makefile for COMP4981 Assignment#3
#

CC=g++
CFLAGS=-c -Wall -pedantic
LDFLAGS=-std=c++11

all: chatclnt chatsrv

chatclnt: chatclnt.o
		${CC} ${LDFLAGS} chatclnt.o -o chatclnt

chatsrv: chatsrv.o
		${CC} ${LDFLAGS} chatsrv.o -o chatsrv

chatclnt.o: chatclnt.c
		  ${CC} ${CFLAGS} chatclnt.c

chatsrv.o: chatsrv.c
		  ${CC} ${CFLAGS} chatsrv.c

clean:
		rm -rf *.o chatclnt chatsrv
