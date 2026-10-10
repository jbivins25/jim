#include "data.h"
#include "find.h"
#include "row.h"
#include "jimio.h"
#include <string.h>
#include <stdlib.h>

void editorFindCallback(char* query, int key) {
	static int last_match = -1;
	static int direction = 1;

	if (key == '\r' || key == '\x1b') {
		last_match = -1;
		direction = 1;
		markRedraw(0, E.win.screenrows, E.mode == NORMAL ? REDRAW_DEF : REDRAW_WIN);
		return;
	}
	else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
		direction = 1;
	}
	else if (key == ARROW_LEFT || key == ARROW_UP) {
		direction = -1;
	}
	else {
		last_match = -1;
		direction = 1;
	}
	
	if (last_match == -1) direction = 1;	
	int current = last_match;
	int numrows = (E.mode == NORMAL) ? E.numrows : E.win.numrows;
	int len = strlen(query);
	for (int i = 0; i < numrows; i++) {
		current += direction;
		if (current == -1) current = numrows - 1;
		else if (current == numrows) current = 0;

		erow* row = (E.mode == NORMAL) ? &E.row[current] : &E.win.row[current];
		char *match = strstr(row->chars, query);
		if (match) {
			last_match = current;
			if (E.mode == NORMAL) {
				E.cy = current;
				E.cx = match - row->chars;
				E.rowoff = E.numrows;
			}
			else {
				E.win.yOffset = current;
				E.win.xOffset = (match+len - row->chars < E.win.screencols) ? 0 : match - row->chars;
				markRedraw(0, E.win.screenrows, REDRAW_WIN);
			}
			break;
		}
	}
}

void editorFind() {
	int saved_cx;
	int saved_cy;
	int saved_coloff;
	int saved_rowoff;
	if (E.mode == NORMAL) {
		saved_cx = E.cx;
		saved_cy = E.cy;
		saved_coloff = E.coloff;
		saved_rowoff = E.rowoff;
	}
	else if (E.mode == WINDOW) {
		(void) saved_cx;
		(void) saved_cy;
		saved_coloff = E.win.xOffset;
		saved_rowoff = E.win.yOffset;
	}
	else return;
	char* query = editorPrompt("Search: %s", editorFindCallback);

	if (query) free(query);
	else {
		if (E.mode == NORMAL) {
			E.cx = saved_cx;
			E.cy = saved_cy;
			E.coloff = saved_coloff;
			E.rowoff = saved_rowoff;
		}
		else {
			E.win.xOffset = saved_coloff;
			E.win.yOffset = saved_rowoff;
		}
	}
}
