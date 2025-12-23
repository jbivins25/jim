#include "data.h"
#include "jimio.h"
#include "window.h"
#include "editor.h"
#include "row.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


void windowSetup(char location, int minCols, int divider, void (*winHandler)(int c), char* header) {
	if ((E.screencols+E.win.screencols)/divider < minCols) return;
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
	redrawWholeScreen = 1;
	if (E.win.handler) E.mode = WINDOW;
}

void clearWindow() {
	E.screencols += E.win.screencols;
	for ( int i = 0; i < E.win.numrows; i++ ) editorFreeRow(&E.win.row[i]);
	free(E.win.row);
	free(E.win.header);
	memset(&E.win, 0, sizeof(windowConfig));
	redrawWholeScreen = 1;
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
	else {
		snprintf(buf, sizeof(buf), "\x1b[%d;%dH\033[1K\r", y+1, E.win.screencols);
		abAppend(ab, buf, strlen(buf));
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
	else if (y == 0) {
		abAppend(ab, "\x1b[7m", 4);
		for (int i = 0; i < E.win.screencols - 1; i++ ) abAppend(ab," ",1);
		abAppend(ab, "\x1b[m", 3);
	}
	else if ( (y-1 + E.win.yOffset) < E.win.numrows) {
		int filerow = y-1 + E.win.yOffset;
		int len = E.win.row[filerow].rsize - E.win.xOffset;
		if (len < 0) len = 0;
		if (len > E.win.screencols-1) len = E.win.screencols-1;
		for (int j = 0; j < len; j++) {abAppend(ab, &E.win.row[filerow].render[E.win.xOffset + j], 1);}
	}
	if (E.win.location == 0) {
		snprintf(buf, sizeof(buf), "\x1b[%d;%dH", y+1, E.win.screencols);
		abAppend(ab, buf, strlen(buf));
		if (y > E.win.numrows) abAppend(ab, "\x1b[1K", 4);
		abAppend(ab, "\x1b[7m", 4);
		abAppend(ab,"|",1);
		abAppend(ab, "\x1b[m", 3);		
	}
	else abAppend(ab, "\x1b[K", 3);
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
	E.win.row[row].hl = NULL;
	E.win.row[row].hl_open_comment = 0;
	E.win.row[row].hl_open_string = 0;
	editorUpdateRow(&E.win.row[row]);
	E.win.numrows++;
	return 0;
}

void windowDelRow(int row) {
	if (row < 0 || row >= E.win.numrows) return;
	editorFreeRow(&E.win.row[row]);
	memmove(&E.win.row[row], &E.row[row + 1], sizeof(erow)*(E.win.numrows-row-1));
	E.win.numrows--;
}

void windowSetRow(char* text, int row, size_t len) {
	if (row < 0 || row >= E.win.numrows) return;
	E.win.row[row].size = len;
	free(E.win.row[row].chars);
	E.win.row[row].chars = text;
	E.win.row[row].chars[len] = '\0';
	E.win.row[row].rsize = 0;
	E.win.row[row].render = NULL;
	editorUpdateRow(&E.win.row[row]);
}

int maxLineSize() {
	int max = 0;
	for ( int i = E.win.yOffset; i < (E.win.numrows <= E.win.screenrows-1 ? E.win.numrows : E.win.screenrows-1 + E.win.yOffset); i++ ) {
		if (E.win.row[i].rsize > max) max = E.win.row[i].rsize;
	}
	return max;
}

void windowPageScroll(int c) {
	int init_x, init_y;
	init_x = E.win.xOffset;
	init_y = E.win.yOffset;
	switch(c) {
		case ARROW_UP:
			if (E.win.yOffset > 0) E.win.yOffset--;
			break;

		case ARROW_DOWN:
			if (E.win.yOffset + E.win.screenrows-1 <= E.win.numrows) E.win.yOffset++;
			break;

		case ARROW_LEFT:
			if (E.win.xOffset > 0) E.win.xOffset--;
			break;

		case ARROW_RIGHT:
			if (E.win.xOffset + E.win.screencols-1 <= maxLineSize()) E.win.xOffset++;

		default:
			break;
	}
	if ( init_x != E.win.xOffset || init_y != E.win.yOffset ) memset(redrawLine,2,E.win.screenrows);
}
