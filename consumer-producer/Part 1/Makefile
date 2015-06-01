# ECE254 lab4 Linux Port
# Makefile: build mqueue sender and receiver example code

CC=gcc
CFLAGS=-Wall -g
LD=gcc
LDFLAGS=-g -lrt -lm -pthread


OBJS1=starter.o

all: produce

produce: $(OBJS1) 
	$(LD) -o $@ $(OBJS1) $(LDFLAGS)

starter.o: starter.c queue.h
	$(CC) $(CFLAGS) -c starter.c 

.PHONY: clean
clean:
	rm -f *.o *.out produce
