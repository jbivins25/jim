#include "data.h"
#include "row.h"
#include "editor.h"
#include <string.h>

int editorRowCxToRx(erow* row, int cx) {
	int rx = 0;
	for (int j = 0; j < cx; j++) {
		if (row->chars[j] == '\t') rx += (JIM_TAB_STOP - 1) - (rx % JIM_TAB_STOP);
		rx++;
	}
	return rx;
}

int editorRowRxToCx(erow *row, int rx) {
	int cur_rx = 0;
	int cx;
	for (cx = 0; cx < row->size; cx++) {
		if (row->chars[cx] == '\t') cur_rx += (JIM_TAB_STOP - 1) - (cur_rx % JIM_TAB_STOP);
		cur_rx++;

		if (cur_rx > rx) return cx;
	}
	return cx;
}

void editorUpdateRow(erow *row, editorSyntax* syn, char mode) {
	int tabs = 0;
	int j;
	for (j = 0; j < row->size; j++) {
		if (row->chars[j] == '\t') tabs++;
	}
	free(row->render);
	row->render = malloc(row->size + tabs*(JIM_TAB_STOP - 1) + 1);	
	int idx = 0;
	for (j = 0; j < row->size; j++) {
		if (row->chars[j] == '\t') {
			row->render[idx++] = ' ';
			while (idx % JIM_TAB_STOP != 0) row->render[idx++] = ' ';
		}
		else {
			row->render[idx++] = row->chars[j];
		}
	}
	row->render[idx] = '\0';
	row->rsize = idx;
	int ind = row-E.row;
	editorUpdateSyntax(row, ind, syn, mode);
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
	E.row[at].rsize = 0; //This line and next are for rendering tabs and extra characters
	E.row[at].render = NULL;
	E.row[at].hl = NULL;
	E.row[at].hl_open_comment = 0;
	E.row[at].hl_open_string = 0;
	editorUpdateRow(&E.row[at], &E.syn, NORMAL);
	E.numrows++;
	E.dirty = 1;
}

void editorFreeRow(erow *row) {
	if (row == NULL) return;
	free(row->render);
	free(row->chars);
	free(row->hl);
	row->render = NULL;
	row->chars = NULL;
	row->hl = NULL;
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
	editorUpdateRow(row, &E.syn, NORMAL);
	E.dirty = 1;
}

void editorRowInsertChars(erow* row, int at, char* text, size_t len) {
	if (at < 0 || at > row->size) at = row->size;
	row->chars = realloc(row->chars, row->size + len + 1);
	memmove(&row->chars[at+len], &row->chars[at], row->size - at + 1);
	memcpy(&row->chars[at], text, len);
	row->size += len;
	editorUpdateRow(row, &E.syn, NORMAL);
	E.dirty = 1;
}

void editorRowAppendString(erow* row, char* s, size_t len) {
	row->chars = realloc(row->chars, row->size + len + 1);
	memcpy(&row->chars[row->size], s, len);
	row->size += len;
	row->chars[row->size] = '\0';
	editorUpdateRow(row, &E.syn, NORMAL);
	E.dirty = 1;
}

void editorRowDelChar(erow *row, int at) {
	if (at < 0 || at >= row->size) return;
	memmove(&row->chars[at], &row->chars[at + 1], row->size - at); //Just shift row over from at+1 onto at
	row->size--;
	editorUpdateRow(row, &E.syn, NORMAL);
	E.dirty = 1;
}

void editorRowDelRange(erow* row, int start, int end) {
	if (start < 0 || end > row->size) return;
	memmove(&row->chars[start], &row->chars[end], row->size - end);
	row->size -= (end-start);
	editorUpdateRow(row, &E.syn, NORMAL);
	E.dirty = 1;
}
