# ECE254 lab4 Linux Port
# Makefile: build mqueue sender and receiver example code

CC=gcc
CFLAGS=-Wall -g
LD=gcc
LDFLAGS=-g -lrt -lm


OBJS1=lab5part2.o
OBJS2=consumer.o
OBJS3=producer.o

all: produce consumer.out producer.out

produce: $(OBJS1) 
	$(LD) -o $@ $(OBJS1) $(LDFLAGS)

lab5part2.o: lab5part2.c
	$(CC) $(CFLAGS) -c lab5part2.c 

consumer.out: $(OBJS2) 
	$(LD) -o $@ $(OBJS2) $(LDFLAGS)

consumer.o: consumer.c
	$(CC) $(CFLAGS) -c consumer.c 

producer.out: $(OBJS3) 
	$(LD) -o $@ $(OBJS3) $(LDFLAGS)

producer.o: producer.c
	$(CC) $(CFLAGS) -c producer.c 

.PHONY: clean
clean:
	rm -f *.o *.out produce
