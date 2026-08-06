#include "data.h"
#include "terminal.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "compat.h"
#include <conio.h>

void die(const char *s) {
	disableRawMode();
	write(STDOUT_FILENO, "\x1b[2J", 4); //Clear up screen on error
	write(STDOUT_FILENO, "\x1b[H", 3);
	perror(s);
	fflush(stderr);
	exit(1);
}

void disableRawMode() {
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), E.orig_termios);
}

void enableRawMode() {
	HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
	GetConsoleMode(hin, &E.orig_termios);
	atexit(disableRawMode);
	DWORD raw = E.orig_termios & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
	SetConsoleMode(hin, raw);
	HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD outmode;
	GetConsoleMode(hout, &outmode);
	SetConsoleMode(hout, outmode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

int editorReadKey() {
	if (!_kbhit()) return KEY_NONE;
	int c = _getch();

	if (c == 0 || c == 224) {
		switch(_getch()) {
			case 72: return ARROW_UP;
			case 80: return ARROW_DOWN;
			case 77: return ARROW_RIGHT;
			case 75: return ARROW_LEFT;

			case 71: return HOME_KEY;
			case 79: return END_KEY;

			case 73: return PAGE_UP;
			case 81: return PAGE_DOWN;

			case 83: return DEL_KEY;
		}
	}

	return c;
}

int getCursorPosition(int *rows, int* cols) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	if (hOut == INVALID_HANDLE_VALUE)
		return -1;

	if (!GetConsoleScreenBufferInfo(hOut, &csbi))
		return -1;

	*cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	*rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

	return 0;
}

int getWindowSize(int* rows, int* cols) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) return -1;
	*cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	*rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	return 0;
}

void setupCrashHandler() {
	return;
}
