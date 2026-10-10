#include "data.h"
#include "editor.h"
#include "row.h"
#include "jimio.h"
#include "ur.h"
#include "palette.h"
#include "compat.h"
#include "syntax.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

void exitSelect() {
	int start = (E.selected[SELECTED_STARTY] - E.rowoff < 0) ? 0 : E.selected[SELECTED_STARTY] - E.rowoff;
	int end = (E.selected[SELECTED_ENDY] - E.rowoff + 1 < E.screenrows) ? E.selected[SELECTED_ENDY] - E.rowoff + 1 : E.screenrows;
	markRedraw(start, end, REDRAW_DEF);
	E.selected[SELECTED_STARTY] = E.selected[SELECTED_ENDY] = E.selected[SELECTED_STARTX] = E.selected[SELECTED_ENDX] = -1;
	E.mode = NORMAL;	
}

void selectMoveCursor(int key) {
	static int anch_y = 0, anch_x = 0;
	if (key == DEFAULT_KEY) { anch_y = 0; anch_x = 0; return; }
	int selYS = E.selected[SELECTED_STARTY], selYE = E.selected[SELECTED_ENDY], selXS = E.selected[SELECTED_STARTX], selXE = E.selected[SELECTED_ENDX];
	if (selYS == selYE && selXS == selXE) { //Guarantees the static anchors get set each time select is enabled without ctrl-a
		anch_y = selYS;
		anch_x = selXS;
	}
	erow *row = &E.row[E.cy];
	int startY = E.cy;
	switch (key) {
		case ARROW_LEFT:
			if (E.cx > 0) {
				E.cx--;
			}
			else if (E.cy > 0) {
				E.cy--;
				E.cx = E.row[E.cy].size == 0 ? 0 : E.row[E.cy].size-1;
				row = &E.row[E.cy];
			}
			E.sticky = editorRowCxToRx(row,E.cx);
			break;
		case ARROW_RIGHT:
			if (row && E.cx < row->size-1) {
				E.cx++;
			}
			else if (row && E.cy < E.numrows-1) {
				E.cy++;
				E.cx = 0;
				row = &E.row[E.cy];
			}
			E.sticky = editorRowCxToRx(row,E.cx);
			break;
		case ARROW_UP:
			if (E.cy > 0) {
				E.cy--;
				row = &E.row[E.cy];
			}
			break;
		case ARROW_DOWN:
			if (E.cy < E.numrows-1) {
				E.cy++;
				row = &E.row[E.cy];
			}
			break;
	}
	int rowlen = row->size == 0 ? 0 : row->size-1;
	if (key == ARROW_UP || key == ARROW_DOWN) {
		if (E.rx < E.sticky) E.rx = E.sticky;
		E.cx = editorRowRxToCx(row, E.rx);
	}
	if (E.cx > rowlen) { //Snap cursor back to end of line
		E.cx = rowlen;
	}
	E.rx = editorRowCxToRx(row, E.cx);
	
	//Selected bounds configuring
	if (E.cy < anch_y || (E.cy == anch_y && E.cx < anch_x)) { //If new cursor position to the "left" of anchor, start becomes cursor, end becomes anchor
		E.selected[SELECTED_STARTY] = E.cy;
		E.selected[SELECTED_STARTX] = E.cx;
		E.selected[SELECTED_ENDY] = anch_y;
		E.selected[SELECTED_ENDX] = anch_x;
	}
	else { //If new cursor position to the "right" of anchor (or ontop of anchor), start becomes anchor, end becomes cursor
		E.selected[SELECTED_STARTY] = anch_y;
		E.selected[SELECTED_STARTX] = anch_x;
		E.selected[SELECTED_ENDY] = E.cy;
		E.selected[SELECTED_ENDX] = E.cx;
	}

	
	markRedrawRow((E.cy-E.rowoff), REDRAW_DEF);
	int oldRow = startY - E.rowoff;
	if (E.cy != startY && ((oldRow) < E.screenrows && (oldRow) >= 0)) markRedrawRow(oldRow, REDRAW_DEF);
}

void editorSelectKeypress(int c) {
	static int quit_times = JIM_QUIT_TIMES;
	switch (c) {
        case CTRL_KEY('c'):
	    editorCopy();
            break;

	case CTRL_KEY('v'):
	    if (E.cpbuffer == NULL) break;
	    editorDelSelect();
	    exitSelect();
	    editorPaste();
	    break;

	case CTRL_KEY('q'):
	    if (E.dirty && quit_times > 0) {
		THREAD_LOCK(T.setMessageLock);
		editorSetStatusMessage("Warning: Unsaved changes. Press Ctrl-Q %d more times to quit.", quit_times);
		THREAD_UNLOCK(T.setMessageLock);
		quit_times--;
		return;
	    }
	    write(STDOUT_FILENO, "\x1b[2J", 4);
	    write(STDOUT_FILENO, "\x1b[H", 3);
	    exit(0);
	    break;

        case '\r':
	case CTRL_KEY('e'):
        case '\x1b': 
            //{ editorSetStatusMessage("E.selected: {%d,%d,%d,%d}", E.selected[0], E.selected[1], E.selected[2], E.selected[3]); }
	    exitSelect();
            break;
            
	case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
		editorDelSelect();
		exitSelect();
		break;
        
        case ARROW_UP:
	case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
	case DEFAULT_KEY:
	    selectMoveCursor(c);
	    break;

	default:
		break;
	}
	if (E.keypressCallback) E.keypressCallback();
	quit_times = JIM_QUIT_TIMES;
}

void editorMoveLine() {
	char* query = editorPrompt("Line number: %s", NULL);
	if (query == NULL) return;
	int line = atoi(query);
	if (line > 0 && line <= E.numrows) {
		E.cx = 0;
		E.cy = line-1;
	}	
	free(query);
	markRedraw(0, E.screenrows, REDRAW_DEF);
}

void editorPaste() {
	if (E.cpbuffer == NULL) return;
	size_t size = 0;
	int prev_cy = E.cy;
	int start = 0;
	if (E.cy == E.numrows) editorInsertRow(E.numrows,"",0);
	while (E.cpbuffer[size] != '\0') {
		if (E.cpbuffer[size] != '\r' && E.cpbuffer[size] != '\0') size++;
		else {
			if ((size_t)start != size) editorInsertCharRange(&E.cpbuffer[start], size-start);
			editorInsertNewline(); 
			size++;
			start = size;
		}
	}	
	if ((size_t)start != size) editorInsertCharRange(&E.cpbuffer[start], size-start);
	start = prev_cy - E.rowoff;
	if (start < 0) start = 0;
	int end = E.cy - E.rowoff + 1;
	markRedraw(start, end, REDRAW_DEF);
}

void editorCopy() {
	if (E.mode != SELECT) return;
	free(E.cpbuffer);
	size_t size = 0;
	for (int i = E.selected[0]+1; i < E.selected[1]; i++) {
		size += E.row[i].size + 1;
	}
	if (E.selected[0] == E.selected[1]) size += E.selected[3] - E.selected[2] + 1;
	else {
		size += E.row[E.selected[0]].size - E.selected[2] + 1;
		if (E.row[E.selected[1]].size > 0) size += E.selected[3] + 1;
	}
	size += 1;
	E.cpbuffer = malloc(size);
	unsigned int ind = 0;
	for (int i = E.selected[0]; i <=E.selected[1]; i++) {
		int start = (i == E.selected[0]) ? E.selected[2] : 0;
		int end = (i == E.selected[1]) ? ((E.selected[3] == E.row[i].size) ? E.selected[3] : E.selected[3]+1) : E.row[i].size;
		for ( int j = start; j < end; j++ ) {
			E.cpbuffer[ind++] = E.row[i].chars[j];
		}
		if (i < E.selected[1]) E.cpbuffer[ind++] = '\r';
	}
	E.cpbuffer[ind] = '\0';
	THREAD_LOCK(T.setMessageLock);
	editorSetStatusMessage("Copied! Selected text: {%d,%d,%d,%d}", E.selected[0], E.selected[1], E.selected[2], E.selected[3]);
	THREAD_UNLOCK(T.setMessageLock);
}

void editorDelSelect() {
	int sel_starty = E.selected[SELECTED_STARTY], sel_startx = E.selected[SELECTED_STARTX], sel_endy = E.selected[SELECTED_ENDY], sel_endx = E.selected[SELECTED_ENDX];
	editorDelRowsRange(sel_starty, sel_endy, sel_startx, sel_endx);
	E.cy = sel_starty;
	E.cx = sel_startx;
	int start = (E.cy - E.rowoff < 0) ? 0 : E.cy - E.rowoff;
	markRedraw(start, E.screenrows, REDRAW_DEF);
}

void editorInsertChar(int c) {
	if (E.cy == E.numrows) {
		editorInsertRow(E.numrows,"", 0);
	}
	editorRowInsertChar(&E.row[E.cy], E.cx, c);
	long sec, nsec;
	if (E.urMode) {
		if (E.tree.curr != NULL) {
			struct timespec timestamp;
			clock_gettime(CLOCK_MONOTONIC, &timestamp);
			sec = timestamp.tv_sec - E.tree.curr->timestamp.tv_sec;
			nsec = timestamp.tv_nsec - E.tree.curr->timestamp.tv_nsec;
			sec = sec * 1000 + nsec / 1000000;
		}
		else sec = 0;
		if (E.urType == DELETE_UR || sec > UNDO_TIMEOUT || E.urType == NULL_UR) addNode(WRITE, E.cx, E.cy, c);
		else appendUrChar(c, E.cx + 1, E.cy);
		E.urType = WRITE;
	}
	E.cx++;
	E.sticky = editorRowCxToRx(&E.row[E.cy],E.cx);
	if (E.cy-E.rowoff >= 0) markRedrawRow(E.cy-E.rowoff, REDRAW_DEF);
}

void editorInsertCharRange(char* text, size_t len) {
	if (text == NULL || len == 0) return;
	editorRowInsertChars(&E.row[E.cy], E.cx, text, len);
	long sec, nsec;
	if (E.urMode) {
		if (E.tree.curr != NULL) {
			struct timespec timestamp;
			clock_gettime(CLOCK_MONOTONIC, &timestamp);
			sec = timestamp.tv_sec - E.tree.curr->timestamp.tv_sec;
			nsec = timestamp.tv_nsec - E.tree.curr->timestamp.tv_nsec;
			sec = sec * 1000 + nsec / 1000000;
		}
		else sec = 0;
		if (E.urType == DELETE_UR || sec > UNDO_TIMEOUT || E.urType == NULL_UR) {
			addNode(WRITE, E.cx, E.cy, text[0]);
			appendUrCharRange(text+1, len-1, E.cx+len, E.cy);
		}
		else {
			appendUrCharRange(text, len, E.cx+len, E.cy);
		}
		E.urType = WRITE;
	}
	E.cx += len;
	E.sticky = editorRowCxToRx(&E.row[E.cy], E.cx);
	if (E.cy-E.rowoff >= 0) markRedrawRow(E.cy-E.rowoff, REDRAW_DEF);
}

void editorInsertNewline() {
	long sec, nsec;
	if (E.urMode) {
		if (E.tree.curr != NULL) {
			struct timespec timestamp;
			clock_gettime(CLOCK_MONOTONIC, &timestamp);
			sec = timestamp.tv_sec - E.tree.curr->timestamp.tv_sec;
			nsec = timestamp.tv_nsec - E.tree.curr->timestamp.tv_nsec;
			sec = sec * 1000 + nsec / 1000000;
		}
		else sec = 0;
		if (E.urType == DELETE_UR || sec > UNDO_TIMEOUT || E.urType == NULL_UR) addNode(WRITE, E.cx, E.cy, '\r');
		else {
			appendUrChar('\r', 0, E.cy+1);
		}
		E.urType = WRITE;
	}
	if (E.cx == 0) {
		editorInsertRow(E.cy, "", 0);
	}
	else {
		erow* row = &E.row[E.cy];
		editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
		row = &E.row[E.cy];
		row->size = E.cx;
		row->tabs = 0;
		for (int i = 0; i < row->size; i++) if (row->chars[i] == '\t') row->tabs++;
		row->chars[row->size] = '\0';
		int ind = row - E.row;
		editorUpdateSyntax(row, ind, &E.syn, NORMAL);
	}
	E.cy++;
	E.cx = 0;
	int line = E.cy-1-E.rowoff;
	if (line < 0) line = 0;
	markRedraw(line, E.screenrows, REDRAW_DEF);
}

void editorDelChar() {
	if (E.cy == E.numrows) return;
	if (E.cx == 0 && E.cy == 0) return;
	erow *row = &E.row[E.cy];
	long sec, nsec;
	if (E.urMode) {
		if (E.tree.curr != NULL) {
			struct timespec timestamp;
			clock_gettime(CLOCK_MONOTONIC, &timestamp);
			sec = timestamp.tv_sec - E.tree.curr->timestamp.tv_sec;
			nsec = timestamp.tv_nsec - E.tree.curr->timestamp.tv_nsec;
			sec = sec * 1000 + nsec / 1000000;
		}
		else sec = 0;
	}
	if (E.cx > 0) {
		if (E.urMode) {
			if (E.urType == WRITE || sec > UNDO_TIMEOUT || E.urType == NULL_UR) addNode(DELETE_UR, E.cx, E.cy, row->chars[E.cx-1]);
			else {
				appendUrChar(row->chars[E.cx-1], E.cx-1, E.cy);
			}
			E.urType = DELETE_UR;
		}
		editorRowDelChar(row, E.cx - 1);
		E.cx--;
		if (E.cy-E.rowoff >= 0) markRedrawRow(E.cy-E.rowoff, REDRAW_DEF);
	}
	else {
		if (E.urMode) {
			if (E.urType == WRITE || sec > UNDO_TIMEOUT || E.urType == NULL_UR) addNode(DELETE_UR, E.cx, E.cy, '\r');
			else {
				appendUrChar('\r', E.row[E.cy-1].size, E.cy-1);
			}
			E.urType = DELETE_UR;
		}
		E.cx = E.row[E.cy-1].size;
		if (row->size > 0) editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
		editorDelRow(E.cy);
		E.cy--;
		int line = E.cy-E.rowoff;
		if (line < 0) line = 0;
		markRedraw(line, E.screenrows, REDRAW_DEF);
	}
	row = &E.row[E.cy];
	E.sticky = editorRowCxToRx(row, E.cx);
}

static inline void* revmemcpy(void* dest, const void* src, size_t len) {
	if (len == 0) return dest;

	char* d = (char*)dest + len - 1;
	const char* s = (const char*)src;

	while (len--) {
		*d-- = *s++;
	}
	return dest;
}

void editorDelRowsRange(int startrow, int endrow, int startrow_x, int endrow_x) { //Deletes all content from E.row[startrow].chars[startrow_x] through E.row[endrow].chars[endrow_x], inclusive
	if (startrow < 0 || endrow >= E.numrows) return;
	if (endrow < startrow) return;
	if (startrow_x < 0 || startrow_x > E.row[startrow].size) return;
	if (endrow_x < 0 || endrow_x > E.row[endrow].size) return;
	if (startrow == endrow && startrow_x >= endrow_x) return;
	long sec, nsec;
	if (E.urMode) {
		size_t len = 0;
		for (int i = endrow; i >= startrow; i--) {
			if (i == endrow && i == startrow) len += endrow_x - startrow_x + 1;
			else if (i == endrow) len += endrow_x + 2;
			else if (i == startrow) len += E.row[i].size - startrow_x;
			else len += E.row[i].size + 1;
		}
		char* text = malloc(len);
		size_t written = 0;
		for (int i = endrow; i >= startrow; i--) {
			if (i == endrow && i == startrow) { revmemcpy(text, E.row[i].chars + startrow_x, len); continue; }
			else if (i == endrow) { revmemcpy(text + written, E.row[i].chars, endrow_x + 1); written += endrow_x + 1; } 
			else if (i == startrow) { revmemcpy(text + written, E.row[i].chars + startrow_x, E.row[i].size - startrow_x); continue; }
			else { revmemcpy(text + written, E.row[i].chars, E.row[i].size); written += E.row[i].size; }
			text[written++] = '\r';
		}
		if (E.tree.curr != NULL) {
			struct timespec timestamp;
			clock_gettime(CLOCK_MONOTONIC, &timestamp);
			sec = timestamp.tv_sec - E.tree.curr->timestamp.tv_sec;
			nsec = timestamp.tv_nsec - E.tree.curr->timestamp.tv_nsec;
			sec = sec * 1000 + nsec / 1000000;
		}
		else sec = 0;
		if (E.urType == WRITE || sec > UNDO_TIMEOUT || E.urType == NULL_UR) {
			addNode(DELETE_UR, endrow_x+1, endrow, text[0]);
			appendUrCharRange(text+1, len-1, startrow_x, startrow);
		}
		else {
			appendUrCharRange(text, len, startrow_x, startrow);
		}
		free(text);
		E.urType = DELETE_UR;
	}
	if (startrow == endrow) editorRowDelRange(&E.row[startrow], startrow_x, endrow_x+1);
	else {
		editorRowDelRange(&E.row[startrow], startrow_x, E.row[startrow].size);
		editorRowDelRange(&E.row[endrow], 0, endrow_x+1);
		editorRowAppendString(&E.row[startrow], E.row[endrow].chars, E.row[endrow].size);
		editorDelRows(startrow+1, endrow);
	}
	E.cx = startrow_x;
	E.cy = startrow;
	E.sticky = editorRowCxToRx(&E.row[E.cy], E.cx);	
}

