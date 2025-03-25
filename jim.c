#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>

#define CTRL_KEY(k) ((k) & 0x1f)

struct editorConfig {
	struct termios orig_termios;
};

struct editorConfig E;

void die(const char *s) {
	write(STDOUT_FILENO, "\x1b[2J", 4); //Clear up screen on error
	write(STDOUT_FILENO, "\x1b[H", 3);
	perror(s);
	exit(1);
}

void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
		die("tcsetattr");
	}
}

void enableRawMode() {
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");  //Get terminal attributes
	atexit(disableRawMode);

	struct termios raw = E.orig_termios;
	raw.c_iflag &= ~(IXON | ICRNL | ISTRIP | INPCK | BRKINT); //Legacy stuff (plus disabling a few controls)
	raw.c_cflag |= (CS8); //Set char size to 8
	raw.c_oflag &= ~(OPOST); //Removes implicit '\r' from '\n' 
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN); //Turns off echo and some controls
	raw.c_cc[VMIN] = 0; //Sets min number of bytes before read can return
	raw.c_cc[VTIME] = 1; //Sets timeout time for read (1/10 of a second)
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

char editorReadKey() {
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN) die("read"); //To allow for Cygwin we check for EAGAIN
	}
	return c;
}

void editorDrawRows() {
	for (int y = 0; y < ; y++) {
		write(STDOUT_FILENO, "~\r", 2);
		if (y < - 1) write(STDOUT_FILENO, "\n", 1);
	}
}

void editorRefreshScreen() {
	write(STDOUT_FILENO, "\x1b[2J", 4); //Using the escape character (\x1b) we clear entire screen
	write(STDOUT_FILENO, "\x1b[H", 3); //Positions cursor at top left
	editorDrawRos();
	write(STDOUT_FILENO,"\x1b[H",3);
}

void editorProcessKeypress() {
	char c = editorReadKey();

	switch(c) {
		case CTRL_KEY('q'):
			write(STDOUT_FILENO, "\x1b[2J", 4); //Clear up screen on exit
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;
	}
}

int main() {
	enableRawMode();

	char c;
	while (1) {
		editorRefreshScreen();
		editorProcessKeypress();
	}
	return 0;
}
