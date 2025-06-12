#include "data.h"
#include "terminal.h"
#include "row.h"
#include "jimio.h"
#include "fileio.h"
#include <unistd.h>
#include <signal.h>

struct editorConfig E;

void freeEditor() {
	for (int i = 0; i < E.numrows; i++) {
		editorFreeRow(&E.row[i]);
	}
	free(E.row);	
	free(E.filename);
	free(E.cpbuffer);	
	write(STDOUT_FILENO, "\x1b[?1049l", 8);
}

void initEditor() {
	E.cx = 0;
	E.cy = 0;
	E.rx = 0;
	E.rowoff = 0;
	E.numrows = 0;
	E.coloff = 0;
	for (int i = 0; i < 4; i++) {
		E.selected[i] = -1;
	}
	E.row = NULL;
	E.dirty = 0;
	E.mode = NORMAL;
	E.cpbuffer = NULL;
	E.filename = NULL;
	E.statusmsg[0] = '\0';
	E.statusmsg_time = 0;
	if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
	E.screenrows -= 2;
	write(STDOUT_FILENO, "\x1b[?1049h", 8);
}

static void win_sighandler(int sig) {
	if (SIGWINCH == sig) {
		write(STDOUT_FILENO, "\x1b[2J", 4);
		if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
		E.screenrows -= 2;
		editorRefreshScreen();
	}
}

int main(int argc, char *argv[]) {
	enableRawMode();
	initEditor();
	atexit(freeEditor);
	signal(SIGWINCH, win_sighandler);
	write(STDOUT_FILENO, "\x1b[2J", 4); //Clear up screen on start
	write(STDOUT_FILENO, "\x1b[H", 3);
	if (argc >= 2) {
		editorOpen(argv[1]);
	}

	editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");

	while (1) {
		editorRefreshScreen();
		editorProcessKeypress();
	}
	return 0;
}
