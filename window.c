#include "data.h"
#include "jimio.h"
#include "window.h"
#include "editor.h"
#include "row.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


void windowSetup(char location, int minCols, int divider, void (*winHandler)(char c), char* header) {
	if (E.screencols/divider < minCols) return;
	clearWindow();
	E.win.active = 1;
	E.win.location = location;
	E.win.minCols = minCols;
	if ( divider < 2 ) E.win.divider = 2;
	else E.win.divider = divider;
	E.win.handler = winHandler;
	E.win.screenrows = E.screenrows;
	E.win.screencols = E.screencols/divider;
	E.screencols -= E.win.screencols;
	E.win.row = NULL;
	E.win.numrows = 0;
	E.win.header = header;
}

void clearWindow() {
	E.screencols += E.win.screencols;
	for ( int i = 0; i < E.win.numrows; i++ ) editorFreeRow(&E.win.row[i]);
	free(E.win.row);
	free(E.win.header);
	memset(&E.win, 0, sizeof(windowConfig));
}

void drawWindow(struct abuf* ab, int y) {
	char buf[32];
	if (E.win.location == 1) {
		snprintf(buf, sizeof(buf), "\x1b[%d;%dH", y+1, E.screencols+1);
		abAppend(ab, buf, strlen(buf));
		abAppend(ab, "\x1b[7m", 4);
		abAppend(ab,"|",1);
		abAppend(ab, "\x1b[m", 3);
	}
	int length = E.win.header ? strlen(E.win.header) : 0;
	if (y == 0 &&  length < E.win.screencols ) {
		int diff = E.win.screencols - 1 - length;
		abAppend(ab, "\x1b[7m", 4);
		for (int i = 0; i < diff/2; i++) abAppend(ab," ",1);
		abAppend(ab, E.win.header, length);
		diff = diff-(diff/2);
		for (int i = 0; i < diff; i++) abAppend(ab," ",1);
		abAppend(ab, "\x1b[m", 3);
	}
	if (E.win.location == 0) {
		if ( y > E.win.numrows ) {
			snprintf(buf, sizeof(buf), "\x1b[%d;%dH", y+1, E.win.screencols);
			abAppend(ab, buf, strlen(buf));
			abAppend(ab, "\x1b[1K", 4);
		}
		abAppend(ab, "\x1b[7m", 4);
		abAppend(ab,"|",1);
		abAppend(ab, "\x1b[m", 3);		
	}
}

int windowAddRow(char* text, int row, size_t len) {
	if (row < 0 || row > E.win.numrows || text == NULL) return -1;
	E.win.row = realloc(E.win.row, sizeof(erow) * (E.win.numrows + 1));
	if (E.win.row == NULL) return -1;
	memmove(&E.win.row[row+1], &E.win.row[row], sizeof(erow) * (E.win.numrows - row));

	E.win.row[row].size = len;
	E.win.row[row].chars = malloc(len + 1);
	memcpy(E.win.row[row].chars, text, len);
	E.win.row[row].chars[len] = '\0';
	E.win.row[row].rsize = 0;
	E.win.row[row].render = NULL;
	editorUpdateRow(&E.win.row[row]);
	E.win.numrows++;
	return 0;
}

void windowDelRow(int row) {

}

void windowSetRow(char* text, int row, size_t len) {

}
