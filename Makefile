CFLAGS = -Wall -g 
CC = gcc

all: server subscriber

server: server.c LinkedList.c

subscriber: subscriber.c LinkedList.c
	${CC} ${CFLAGS} subscriber.c LinkedList.c -o subscriber -lm

clean:
	rm -f server subscriber

