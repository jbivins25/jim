#include "data.h"
#include "row.h"
#include "editor.h"
#include "syntax.h"
#include <string.h>

int editorRowCxToRx(erow* row, int cx) {
	if (row->tabs == 0) return cx;
	int rx = 0;
	for (int j = 0; j < cx; j++) {
		if (row->chars[j] == '\t') rx += (JIM_TAB_STOP - 1) - (rx % JIM_TAB_STOP);
		rx++;
	}
	return rx;
}

int editorRowRxToCx(erow *row, int rx) {
	if (row->tabs == 0) return rx;
	int cur_rx = 0;
	int cx;
	for (cx = 0; cx < row->size; cx++) {
		if (row->chars[cx] == '\t') cur_rx += (JIM_TAB_STOP - 1) - (cur_rx % JIM_TAB_STOP);
		cur_rx++;

		if (cur_rx > rx) return cx;
	}
	return cx;
}

void editorInsertRow(int at, char *s, size_t len) {
	if (at < 0 || at > E.numrows) return;
	if (E.numrows >= E.capacity) {
		E.capacity = E.capacity * 2;
		E.row = realloc(E.row, sizeof(erow) * E.capacity);
	}
	memmove(&E.row[at+1], &E.row[at], sizeof(erow) * (E.numrows - at));

	E.row[at].size = len;
	E.row[at].chars = malloc(len + 1);
	memcpy(E.row[at].chars, s, len);
	E.row[at].chars[len] = '\0';
	E.row[at].tabs = 0;
	for (size_t i = 0; i < len; i++) if (s[i] == '\t') E.row[at].tabs++;
	E.row[at].hl_open_comment = 0;
	E.row[at].hl_open_string = 0;
	editorUpdateSyntax(&E.row[at], at, &E.syn, NORMAL);
	E.numrows++;
	E.dirty = 1;
}

void editorFreeRow(erow *row) {
	if (row == NULL) return;
	free(row->chars);
	row->chars = NULL;
}

void editorDelRow(int at) {
	if (at < 0 || at >= E.numrows) return;
	editorFreeRow(&E.row[at]);
	memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1)); //Move everything after up
	E.numrows--;
	E.dirty = 1;
}

void editorDelRows(int start, int end) {
	if (start < 0 || start >= E.numrows) return;
	if (end < 0 || end >= E.numrows) return;
	if (end < start) return;
	for (int i = start; i <= end; i++) editorFreeRow(&E.row[i]);
	memmove(&E.row[start], &E.row[end + 1], sizeof(erow) * (E.numrows - end - 1));
	E.numrows -= (end - start + 1);
	E.dirty = 1;
}

void editorRowInsertChar(erow *row, int at, int c) {
	if (at < 0 || at > row->size) at = row->size;
	row->chars = realloc(row->chars, row->size + 2);
	memmove(&row->chars[at+1], &row->chars[at], row->size - at + 1);
	row->size++;
	row->chars[at] = c;
	if (c == '\t') row->tabs++;
	int ind = row - E.row;
	editorUpdateSyntax(row, ind, &E.syn, NORMAL);
	E.dirty = 1;
}

void editorRowInsertChars(erow* row, int at, char* text, size_t len) {
	if (at < 0 || at > row->size) at = row->size;
	row->chars = realloc(row->chars, row->size + len + 1);
	memmove(&row->chars[at+len], &row->chars[at], row->size - at + 1);
	memcpy(&row->chars[at], text, len);
	row->size += len;
	for (size_t i = 0; i < len; i++) if (text[i] == '\t') row->tabs++;
	int ind = row - E.row;
	editorUpdateSyntax(row, ind, &E.syn, NORMAL);
	E.dirty = 1;
}

void editorRowAppendString(erow* row, char* s, size_t len) {
	row->chars = realloc(row->chars, row->size + len + 1);
	memcpy(&row->chars[row->size], s, len);
	row->size += len;
	row->chars[row->size] = '\0';
	for (size_t i = 0; i < len; i++) if (s[i] == '\t') row->tabs++;
	int ind = row - E.row;
	editorUpdateSyntax(row, ind, &E.syn, NORMAL);
	E.dirty = 1;
}

void editorRowDelChar(erow *row, int at) {
	if (at < 0 || at >= row->size) return;
	if (row->chars[at] == '\t') row->tabs--;
	memmove(&row->chars[at], &row->chars[at + 1], row->size - at); //Just shift row over from at+1 onto at
	row->size--;
	int ind = row - E.row;
	editorUpdateSyntax(row, ind, &E.syn, NORMAL);
	E.dirty = 1;
}

void editorRowDelRange(erow* row, int start, int end) {
	if (start < 0 || end > row->size) return;
	for (int i = start; i < end; i++) if (row->chars[i] == '\t') row->tabs--;
	memmove(&row->chars[start], &row->chars[end], row->size - end);
	row->size -= (end-start);
	int ind = row - E.row;
	editorUpdateSyntax(row, ind, &E.syn, NORMAL);
	E.dirty = 1;
}
