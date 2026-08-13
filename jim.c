#include "data.h"
#define WIN_SIG
#include "terminal.h"
#include "row.h"
#include "jimio.h"
#include "fileio.h"
#include "window.h"
#include "ur.h"
#include "syntax.h"
#include <signal.h>
#include <stdio.h>
#include "compat.h"
#include <string.h>

struct editorConfig E;
editorThreads T = {0};
char redrawLine[SCREEN_ROW_MAX] = {0};
int redrawWholeScreen = 1;
#ifndef _WIN32
int eventPipe[2];
#else
HANDLE eReadPipe, eWritePipe;
HANDLE hStdin, pipeEvent;
HANDLE hEvents[2];
OVERLAPPED ol = {0};
#endif

void freeEditor() {
	if (E.win.active) clearWindow();
	for (int i = 0; i < E.numrows; i++) {
		editorFreeRow(&E.row[i]);
	}
	free(E.row);	
	free(E.filename);
	free(E.cpbuffer);	
	for (int i = 0; i < E.win.numrows; i++) editorFreeRow(&E.win.row[i]);
	free(E.win.row);
	freeTree(&E.tree);
	freeSyntax(&E.syn);
	#ifndef _WIN32
	close(eventPipe[0]);
	close(eventPipe[1]);
	#else
	CloseHandle(eReadPipe);
	CloseHandle(eWritePipe);
	CloseHandle(pipeEvent);
	#endif
	write(STDOUT_FILENO, "\x1b[?1049l", 8);
}

void freeLocks() {
	DELETE_LOCK(T.threadLock);
	DELETE_LOCK(T.redrawLock);
	DELETE_LOCK(T.windowThreadLock);
	DELETE_LOCK(T.setMessageLock);
	DELETE_LOCK(T.eventPipeLock);
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
	E.dirty = 0;
	E.mode = NORMAL;
	E.cpbuffer = NULL;
	E.filename = NULL;
	E.statusmsg[0] = '\0';
	E.statusmsg_time = 0;
	if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
	E.screenrows -= 2;
	memset(&E.win,0,sizeof(windowConfig));
	memset(&E.syn,0,sizeof(editorSyntax));
	initTree(&E.tree);
	E.urType = NULL_UR;
	E.urMode = 1;
	E.capacity = STARTING_CAPACITY;
	E.row = malloc(sizeof(erow)*E.capacity);
	const char *term = getenv("TERM");
	if (!term) E.colorful =  0;
	else E.colorful = strstr(term, "256color") != NULL;
	E.sticky = 0;
	E.keypressCallback = NULL;
	#ifndef _WIN32
	if (pipe(eventPipe) == -1) die("Pipe");
	#else
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;
	if(!CreatePipe(&eReadPipe, &eWritePipe, &saAttr, 0)) die("Pipe");
	eReadPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\eventPipe"), PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 0, 0, 0, &saAttr);
	eWritePipe = CreateFile(TEXT("\\\\.\\pipe\\eventPipe"), GENERIC_WRITE, 0, &saAttr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (eReadPipe == INVALID_HANDLE_VALUE || eWritePipe == INVALID_HANDLE_VALUE) die("Pipe creation failed");
	hStdin = GetStdHandle(STD_INPUT_HANDLE);
	pipeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	ol.hEvent = pipeEvent;
	hEvents[0] = hStdin;
	hEvents[1] = pipeEvent;
	char buf[8];
	DWORD bytesRead;
	ReadFile(eReadPipe, buffer, sizeof(buffer), &bytesRead, &ol);
	#endif
	write(STDOUT_FILENO, "\x1b[?1049h", 8);
}

void initLocks() {
	CREATE_LOCK(T.threadLock);
	CREATE_LOCK(T.redrawLock);
	CREATE_LOCK(T.windowThreadLock);
	CREATE_LOCK(T.setMessageLock);
	CREATE_LOCK(T.eventPipeLock);
}

int main(int argc, char *argv[]) {
	enableRawMode();
	setupCrashHandler();
	initEditor();
	atexit(freeEditor);
	initLocks();
	atexit(freeLocks);
	#ifndef _WIN32
	signal(SIGWINCH, win_sighandler);
	#endif
	write(STDOUT_FILENO, "\x1b[2J", 4); //Clear up screen on start
	write(STDOUT_FILENO, "\x1b[H", 3);
	if (argc >= 2) {
		loadSyntax(argv[1], &E.syn);
		editorOpen(argv[1]);
	}

	THREAD_LOCK(T.setMessageLock);
	editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");
	THREAD_UNLOCK(T.setMessageLock);

	while (1) {
		editorRefreshScreen();
		editorWaitEvent();
		editorJoinThread();
	}
	return 0;
}
