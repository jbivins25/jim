#ifndef JIM_DUMPTREE
#define JIM_DUMPTREE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "data.h"
#include "jimio.h"

void dumptreeProcessKey(int c) {
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

		case CTRL_KEY('f'): 
			editorFind();
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

void dumprecursivehelper(urBlock* temp) {
	char buf[80] = {0};
	int length = 0;
	if (temp == NULL) return;
	length = snprintf(buf, sizeof(buf), "ID: %p", (void*)temp);
	windowAddRow(buf, E.win.numrows, length);
	if (temp->type == WRITE) {memcpy(buf, "Type: WRITE", 11); length = 11;}
	else if (temp->type == DELETE_UR) {memcpy(buf, "Type: DELETE", 12); length = 12;}
	else {memcpy(buf, "Type: NULL", 10); length = 10;}
	windowAddRow(buf, E.win.numrows, length);
	if (temp->type == NULL_UR) {
		for (int i = 0; i < temp->childlen; i++) {
			dumprecursivehelper(temp->children[i]);
		}
		return;
	}
	length = snprintf(buf, sizeof(buf), "Parent ID: %p", (void*)temp->parent);
	windowAddRow(buf, E.win.numrows, length);
	length = snprintf(buf, sizeof(buf), "Start: %d, %d", temp->start[0], temp->start[1]);
	windowAddRow(buf, E.win.numrows, length);
	length = snprintf(buf, sizeof(buf), "End: %d, %d", temp->end[0], temp->end[1]);
	windowAddRow(buf, E.win.numrows, length);
	length = snprintf(buf, sizeof(buf), "Length: %d", temp->length);
	windowAddRow(buf, E.win.numrows, length);
	length = snprintf(buf, sizeof(buf), "Chars:");
	windowAddRow(buf, E.win.numrows, length);
	if (temp->length < 80) {
		length = temp->length;
		for ( int i = 0; i < temp->length; i++ ) {
			buf[i] = (temp->chars[i] == '\r') ? '$' : temp->chars[i];
		}
	}
	else {
		length = snprintf(buf, sizeof(buf), "Too long of edit: %d chars", temp->length);
	}
	windowAddRow(buf, E.win.numrows, length);
	buf[0] = '\0';
	windowAddRow(buf, E.win.numrows, 0);
	for (int i = 0; i < temp->childlen; i++) {
		dumprecursivehelper(temp->children[i]);
	}
	return;
}

int jim_dumptree(const int argc, const char* args[]) {
	(void)argc;
	(void)args;
	if (E.win.active == 1) clearWindow();
	windowSetup(WINDOW_RIGHT, 10, 2, dumptreeProcessKey, strdup("Tree Dump"));
	urBlock* temp = E.tree.root;
	dumprecursivehelper(temp);
	return 0;
}

#endif
