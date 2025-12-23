CC = gcc
CFLAGS=-ggdb -std=c99 -Wall -Wno-strict-prototypes -Wextra -pedantic

local: jim_loc

debug: ab.o editor.o fileio.o find.o jim.o jimio.o row.o terminal.o command.o window.o ur.o
	gcc -g -fsanitize=address -o jim *.o

jim_loc: ab.o editor.o fileio.o find.o jim.o jimio.o row.o terminal.o command.o window.o ur.o
	$(CC) $(CFLAGS) *.o -o jim

jim: setup_env ab.o editor.o fileio.o find.o jim.o jimio.o row.o terminal.o command.o window.o ur.o
	$(CC) $(CFLAGS) *.o -o ~/bin/jim
	@echo Done!

ab.o: ab.c ab.h
	$(CC) $(CFLAGS) -c ab.c

command.o: command.c command.h config.h
	$(CC) $(CFLAGS) -c command.c

editor.o: editor.c editor.h data.h row.h jimio.h palette.h
	$(CC) $(CFLAGS) -c editor.c

fileio.o: fileio.c fileio.h data.h jimio.h row.h
	$(CC) $(CFLAGS) -c fileio.c

find.o: find.c find.h data.h jimio.h row.h
	$(CC) $(CFLAGS) -c find.c

jim.o: jim.c data.h fileio.h jimio.h row.h terminal.h
	$(CC) $(CFLAGS) -c jim.c

jimio.o: jimio.c jimio.h data.h ab.h editor.h fileio.h find.h row.h terminal.h window.h
	$(CC) $(CFLAGS) -c jimio.c

row.o: row.c row.h data.h
	$(CC) $(CFLAGS) -c row.c

terminal.o: terminal.c terminal.h data.h
	$(CC) $(CFLAGS) -c terminal.c

ur.o: ur.c ur.h data.h
	$(CC) $(CFLAGS) -c ur.c

window.o: window.c window.h data.h
	$(CC) $(CFLAGS) -c window.c

setup_env:
	@echo Installing jim...
	@echo Setting up environment...
	mkdir -p ~/bin
	mkdir -p ~/.jim
	cp jim_*.syn ~/.jim/.
	@echo Compiling...

clean:
	rm -f jim *.o
