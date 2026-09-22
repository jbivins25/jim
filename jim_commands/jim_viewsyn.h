#ifndef JIM_VIEWSYN
#define JIM_VIEWSYN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../compat.h"
#include "../data.h"
#include "../window.h"
#include "../jimio.h"

void viewsynProcessKey(int c) {
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

int jim_viewsyn(const int argc, const char* args[]) {
	(void)argc;
	(void)args;
	if (E.syn.filetype == NULL) return -101;
	char* name = "Syntax Viewer";
	windowSetup(WINDOW_RIGHT, 10, 2, viewsynProcessKey, strdup(name));
	char* temp1 = "Keywords:";
	windowAddRow(temp1, E.win.numrows, strlen(temp1));
	for (int i = 0; i < E.syn.keywordCount; i++) {
		size_t linelen = strlen(E.syn.keywords[i]);
		windowAddRow(E.syn.keywords[i], E.win.numrows, linelen);
	}
	char* temp2 = "Types:";
	windowAddRow(temp2, E.win.numrows, strlen(temp2));
	for (int i = 0; i < E.syn.typeCount; i++) {
		size_t linelen = strlen(E.syn.types[i]);
		windowAddRow(E.syn.types[i], E.win.numrows, linelen);
	}
	char* temp3 = "Flags:";
	windowAddRow(temp3, E.win.numrows, strlen(temp3));
	char* temp4 = "Highlight Numbers";
	if (E.syn.flags & HGHLT_NUM) windowAddRow(temp4, E.win.numrows, strlen(temp4));
	temp4 = "Highlight Strings";
	if (E.syn.flags & HGHLT_STRING) windowAddRow(temp4, E.win.numrows, strlen(temp4));
	temp4 = "Highlight Single Line Comments";
	if (E.syn.flags & HGHLT_SL_CM) windowAddRow(temp4, E.win.numrows, strlen(temp4));
	temp4 = "Highlight Multiline Comments";
	if (E.syn.flags & HGHLT_ML_CM) windowAddRow(temp4, E.win.numrows, strlen(temp4));
	temp4 = "Highlight Multiline Strings";
	if (E.syn.flags & HGHLT_ML_STRINGS) windowAddRow(temp4, E.win.numrows, strlen(temp4));
	return 0;
}

#endif
