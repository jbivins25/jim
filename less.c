#include <stdio.h>
#include <ncurses.h>

int main(int argc, char** argv) {
	initscr();	
	FILE* f = fopen(argv[1], "r");
	int c;
	while ((c = getc(f)) != EOF) putc(c, stdout);
	fclose(f);
	while (c != 'q') {
		c = getc(stdin);
		if (c == '\x1b') break;
	}
	endwin();
	return 0;
}
