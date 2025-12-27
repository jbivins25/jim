#include "data.h"
#include "jimio.h"
#include "window.h"
#include "editor.h"
#include "row.h"
#include "palette.h"
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
	memset(&E.win.syn,0,sizeof(editorSyntax));
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
	static int set = 1;
	static char def_fg[8];
	static int def_fg_len;
	static char def_bg[8];
	static int def_bg_len;
	static char inv_bg[8];
	static int inv_bg_len;
	static char inv_fg[8];
	static int inv_fg_len;
	if (set) {
		set = 0;
		def_fg_len = snprintf(def_fg, sizeof(def_fg), "\x1b[%dm", DEF_FG);
		def_bg_len = snprintf(def_bg, sizeof(def_fg), "\x1b[%dm", DEF_BG);
		inv_bg_len = snprintf(inv_bg, sizeof(inv_bg), "\x1b[%dm", INV_BG);
		inv_fg_len = snprintf(inv_fg, sizeof(inv_bg), "\x1b[%dm", INV_FG);
	}
	if (E.win.location == 1) {
		snprintf(buf, sizeof(buf), "\x1b[%d;%dH", y+1, E.screencols+1);
		abAppend(ab, buf, strlen(buf));
		abAppend(ab,inv_bg, inv_bg_len);
		abAppend(ab,inv_fg, inv_fg_len);
		abAppend(ab,"|",1);
		abAppend(ab,def_bg, def_bg_len);
		abAppend(ab,def_fg, def_fg_len);
	}
	else {
		snprintf(buf, sizeof(buf), "\x1b[%d;%dH\033[1K\r", y+1, E.win.screencols);
		abAppend(ab, buf, strlen(buf));
	}
	int length = E.win.header ? strlen(E.win.header) : 0;
	if (y == 0 &&  length < E.win.screencols ) {
		int diff = E.win.screencols - 1 - length;
		abAppend(ab,inv_bg, inv_bg_len);
		abAppend(ab,inv_fg, inv_fg_len);
		for (int i = 0; i < diff/2; i++) abAppend(ab," ",1);
		abAppend(ab, E.win.header, length);
		diff = diff-(diff/2);
		for (int i = 0; i < diff; i++) abAppend(ab," ",1);
		abAppend(ab,def_bg, def_bg_len);
		abAppend(ab,def_fg, def_fg_len);
	}
	else if (y == 0) {
		abAppend(ab,inv_bg, inv_bg_len);
		abAppend(ab,inv_fg, inv_fg_len);
		for (int i = 0; i < E.win.screencols - 1; i++ ) abAppend(ab," ",1);
		abAppend(ab,def_bg, def_bg_len);
		abAppend(ab,def_fg, def_fg_len);
	}
	else if ( (y-1 + E.win.yOffset) < E.win.numrows) {
		int filerow = y-1 + E.win.yOffset;
		int len = E.win.row[filerow].rsize - E.win.xOffset;
		if (len < 0) len = 0;
		if (len > E.win.screencols-1) len = E.win.screencols-1;
		int curr_color = DEF_FG;
		for (int j = 0; j < len; j++) {
			int color = editorSyntaxToColor(E.win.row[filerow].hl[E.win.xOffset + j]);
			if (curr_color != color) {
				char buf[16];
				int hlen;
				if (E.colorful == 0 || color == DEF_FG) hlen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
				else hlen = snprintf(buf, sizeof(buf), "\x1b[38;5;%dm", color);
				abAppend(ab, buf, hlen);
				curr_color = color;
			}
			abAppend(ab, &E.win.row[filerow].render[E.win.xOffset + j], 1);
		}
	}
	if (E.win.location == 0) {
		snprintf(buf, sizeof(buf), "\x1b[%d;%dH", y+1, E.win.screencols);
		abAppend(ab, buf, strlen(buf));
		if (y > E.win.numrows) abAppend(ab, "\x1b[1K", 4);
		abAppend(ab,inv_bg, inv_bg_len);
		abAppend(ab,inv_fg, inv_fg_len);
		abAppend(ab,"|",1);
		abAppend(ab,def_bg, def_bg_len);
		abAppend(ab,def_fg, def_fg_len);		
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
	editorUpdateRow(&E.win.row[row], &E.win.syn, WINDOW);
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
	editorUpdateRow(&E.win.row[row], &E.win.syn, WINDOW);
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
	if ( init_x != E.win.xOffset || init_y != E.win.yOffset ) {
		for (int i = 0; i < E.win.screenrows; i++) {
			redrawLine[i] |= REDRAW_WIN;
		}
	}
}

void windowLoadSyntax(char* filename) {
	char* ext = strrchr(filename, '.');
	if (!ext) return;
	size_t len = strlen(++ext);
	if (len < 1) return;
	const char* home = getenv("HOME");
	if (!home) home = "./";
	char file[512];
	snprintf(file, len+15+strlen(home), "%s/.jim/jim_%s.syn", home, ext);
	FILE* f = fopen(file,"r");
	if (f == NULL) return;
	E.win.syn.filetype = malloc(len+1);
	strcpy(E.win.syn.filetype,ext);
	char* line = NULL;
	size_t cap = 0;
	getline(&line, &cap, f);
	sscanf(line, "%d %d", &E.win.syn.keywordCount, &E.win.syn.typeCount);
	E.win.syn.keywords = malloc(sizeof(char*)*E.win.syn.keywordCount);
	char* line_t;
	for (int i = 0; i < E.win.syn.keywordCount; i++) {
		len = getline(&line, &cap, f);
		while (line[len-1] == '\n' || line[len-1] == '\r') len--;
		line_t = malloc(len+1);
		for (size_t j = 0; j < len+1; j++) line_t[j] = line[j];
		line_t[len] = '\0';
		E.win.syn.keywords[i] = line_t;
	}
	E.win.syn.types = malloc(sizeof(char*)*E.win.syn.typeCount);
	for (int i = 0; i < E.win.syn.typeCount; i++) {
		len = getline(&line, &cap, f);
		while (line[len-1] == '\n' || line[len-1] == '\r') len--;
		line_t = malloc(len+1);
		for (size_t j = 0; j < len+1; j++) line_t[j] = line[j];
		line_t[len] = '\0';
		E.win.syn.types[i] = line_t;
	}
	getline(&line, &cap, f);
	sscanf(line, "%d", &E.win.syn.flags);
	if (E.win.syn.flags & HGHLT_SL_CM) {
		len = getline(&line, &cap, f);
		while (line[len-1] == '\n' || line[len-1] == '\r') len--;
		line_t = malloc(len+1);
		for (size_t j = 0; j < len+1; j++) line_t[j] = line[j];
		line_t[len] = '\0';
		E.win.syn.slComment = line_t;
	}
	if (E.win.syn.flags & HGHLT_ML_CM) {
		len = getline(&line, &cap, f);
		while (line[len-1] == '\n' || line[len-1] == '\r') len--;
		line_t = malloc(len+1);
		for (size_t j = 0; j < len+1; j++) line_t[j] = line[j];
		line_t[len] = '\0';
		E.win.syn.mlCommentStart = line_t;
		len = getline(&line, &cap, f);
		while (line[len-1] == '\n' || line[len-1] == '\r') len--;
		line_t = malloc(len+1);
		for (size_t j = 0; j < len+1; j++) line_t[j] = line[j];
		line_t[len] = '\0';
		E.win.syn.mlCommentEnd = line_t;
	}
	free(line);
	fclose(f);
}
