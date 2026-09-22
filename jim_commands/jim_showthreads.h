#ifndef JIM_SHOWTHREADS
#define JIM_SHOWTHREADS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data.h"
#include "window.h"
#include "jimio.h"

void showThreadsProcessKey(int c) {
	static int quit_times = JIM_QUIT_TIMES;
	switch(c) {
		case CTRL_KEY('w'):
			E.mode = NORMAL;
			break;

		case CTRL_KEY('q'):
			if (E.dirty && quit_times > 0) {
				editorSetStatusMessage("Warning: Unsaved changes. Press Ctrl-Q %d more times to quit.", quit_times);
				quit_times--;
				return;
			}
			write(STDOUT_FILENO, "\x1b[2J", 4); //Clear up screen on exit
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;

		case '\x1b':
			clearWindow();
			E.mode = NORMAL;
			break;

		case '\r':
			editorCommand();
			break;

		case ARROW_LEFT:
		case ARROW_RIGHT:
		case ARROW_UP:
		case ARROW_DOWN:
			windowPageScroll(c);
			break;
		default:
			break;
	}
	quit_times = JIM_QUIT_TIMES;
}

int jim_showthreads(const int argc, const char* args[]) {
	(void)argc;
	(void)args;
	windowSetup(WINDOW_RIGHT, 10, 2, showThreadsProcessKey, strdup("Threads"));
	THREAD_LOCK(T.threadLock);
	char* threadHeader = "Thread Slot %d:\n";
	char data[256];
	for (int i = 0; i < MAX_THREADS; i++) {
		size_t size = snprintf(data, 256, threadHeader, i);
		windowAddRow(data, E.win.numrows, size);
		size = snprintf(data, 256, "\tState: %s\n", T.slot[i].state == THREAD_UNUSED ? "THREAD_UNUSED" : (T.slot[i].state == THREAD_ACTIVE ? "THREAD_ACTIVE" : "THREAD_FINISHED"));
		windowAddRow(data, E.win.numrows, size);
		if (T.slot[i].state != THREAD_ACTIVE) continue;
		size = snprintf(data, 256, "\tWrite Enabled: %d\n", T.slot[i].writeEnabled);
		windowAddRow(data, E.win.numrows, size);
		size = snprintf(data, 256, "\tWindow Id: %ud\n", T.slot[i].windowId);
		windowAddRow(data, E.win.numrows, size);
	}
	THREAD_UNLOCK(T.threadLock);
	return 0;
}

#endif
