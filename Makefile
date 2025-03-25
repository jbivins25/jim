CC = gcc
CFLAGS=-ggdb -Wall -Wextra -pedantic -std=c99

all: jim_term

jim_term: jim.c
	$(CC) $(CFLAGS) jim.c -o jim

clean:
	rm -f jim *.o
