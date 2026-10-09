#ifndef JIM_EDITORSTATE
#define JIM_EDITORSTATE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compat.h"
#include "data.h"
#include "window.h"
#include "jimio.h"
#include "find.h"

void editorstateProcessKey(int c) {
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

void printEditorState() {
	char buf[128];
	size_t len = snprintf(buf, sizeof(buf), "Editor cursor: cx:%d cy:%d rx:%d", E.cx, E.cy, E.rx);
	if (E.win.numrows < 1) windowAddRow(buf, E.win.numrows, len);
	else windowSetRow(buf, 0, len);
	len = snprintf(buf, sizeof(buf), "Line data: size:%d", E.cy < E.numrows ? E.row[E.cy].size : 0);
	if (E.win.numrows < 2) windowAddRow(buf, E.win.numrows, len);
	else windowSetRow(buf, 1, len);
	len = snprintf(buf, sizeof(buf), "Screensize: R%d;C%d", E.screenrows,E.screencols);	
	if (E.win.numrows < 3) windowAddRow(buf, E.win.numrows, len);
	else windowSetRow(buf, 2, len);
	len = snprintf(buf, sizeof(buf), "Text rows: Size:%d Cap:%d",E.numrows, E.capacity);
	if (E.win.numrows < 4) windowAddRow(buf, E.win.numrows, len);
	else windowSetRow(buf, 3, len);
	len = snprintf(buf, sizeof(buf), "Dirty: %s", E.dirty ? "true" : "false");
	if (E.win.numrows < 5) windowAddRow(buf, E.win.numrows, len);
	else windowSetRow(buf, 4, len);
	len = snprintf(buf, sizeof(buf), "Selected: sx:%d sy:%d ex:%d ey:%d", E.selected[SELECTED_STARTX], E.selected[SELECTED_STARTY], E.selected[SELECTED_ENDX], E.selected[SELECTED_ENDY]);
	if (E.win.numrows < 6) windowAddRow(buf, E.win.numrows, len);
	else windowSetRow(buf, 5, len);
	len = snprintf(buf, sizeof(buf), "Lines: %s Relative: %s", E.linenum ? "on" : "off", E.relative ? "on" : "off");
	if (E.win.numrows < 7) windowAddRow(buf, E.win.numrows, len);
	else windowSetRow(buf, 6, len);
	markRedraw(0, (E.win.screenrows < E.win.numrows - E.win.yOffset) ? E.win.screenrows : (E.win.numrows - E.win.yOffset), REDRAW_WIN);
}

int jim_editorstate(const int argc, const char* args[]) {
	(void)argc;
	(void)args;
	windowSetup(WINDOW_RIGHT | SET_CALLBACK, 10, 2, editorstateProcessKey, strdup("Editor State"));
	E.keypressCallback = printEditorState;
	printEditorState();
	return 0;
}

#endif
