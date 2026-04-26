#include "data.h"
#include "editor.h"
#include "row.h"
#include "jimio.h"
#include "ur.h"
#include "palette.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

enum {
	LEFT = 0,
	RIGHT
};

void exitSelect() {
	for (int i = (E.selected[0] - E.rowoff < 0) ? 0 : E.selected[0] - E.rowoff; i < E.selected[1]+1; i++) {
		redrawLine[i] |= REDRAW_DEF;
	}
	E.selected[0] = E.selected[1] = E.selected[2] = E.selected[3] = -1;
	E.mode = NORMAL;	
}

void selectMoveCursor(int key) {
	static int sticky = 0;
	sticky = (sticky > E.rx) ? sticky : E.rx;
	erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
	switch (key) {
		case ARROW_LEFT:
			if (E.cx != 0) {
				E.cx--;
			}
			else if (E.cy > 0) {
				E.cy--;
				E.cx = E.row[E.cy].size-1;
			}
			sticky = editorRowCxToRx(row,E.cx);
			break;
		case ARROW_RIGHT:
			if (row && E.cx < row->size-1) {
				E.cx++;
			}
			else if (row) {
				E.cy++;
				E.cx = 0;
			}
			sticky = editorRowCxToRx(row,E.cx);
			break;
		case ARROW_UP:
			if (E.cy != 0) {
				E.cy--;
			}
			break;
		case ARROW_DOWN:
			if (row && E.cy < E.numrows-1) {
				E.cy++;
			}
			break;
	}
	row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
	int rowlen = row ? row->size-1 : 0;
	if (key == ARROW_UP || key == ARROW_DOWN) {
		if (E.rx < sticky) E.rx = sticky;
		E.cx = row ? editorRowRxToCx(row, E.rx) : 0;
	}
	if (E.cx > rowlen) { //Snap cursor back to end of line
		E.cx = rowlen;
	}
	E.rx = row ? editorRowCxToRx(row, E.cx) : 0;
}

void editorHghlt(int c) {
	static char hl_dir = LEFT;
	switch (c) {
        case CTRL_KEY('c'): {
	    free(E.cpbuffer);
	    size_t size = 0;
	    for (int i = E.selected[0]+1; i < E.selected[1]; i++) {
		size += E.row[i].size + 1;
	    }
	    if (E.selected[0] == E.selected[1]) size += E.selected[3] - E.selected[2] + 1;
	    else {
		size += E.row[E.selected[0]].size - E.selected[2] + 1;
		size += E.selected[3] + 1;
	    }
	    size += 1;
	    E.cpbuffer = malloc(size);
	    unsigned int ind = 0;
	    for (int i = E.selected[0]; i <=E.selected[1]; i++) {
		int start = (i == E.selected[0]) ? E.selected[2] : 0;
		int end = (i == E.selected[1]) ? E.selected[3]+1 : E.row[i].size;
		for ( int j = start; j < end; j++ ) {
			E.cpbuffer[ind++] = E.row[i].chars[j];
		}
		if (i < E.selected[1]) E.cpbuffer[ind++] = '\r';
	    }
	    E.cpbuffer[ind] = '\0';
	    }
	    editorSetStatusMessage("Copied! Selected text: {%d,%d,%d,%d}", E.selected[0], E.selected[1], E.selected[2], E.selected[3]);
            break;

	case CTRL_KEY('v'):
	    if (E.cpbuffer == NULL) break;
	    editorDelSelect();
	    exitSelect();
	    editorPaste();
	    break;

        case '\r':
	case CTRL_KEY('e'):
        case CTRL_KEY('q'):
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
	    selectMoveCursor(c);
            if (hl_dir == LEFT) {
		E.selected[0] = E.cy;
		E.selected[2] = E.cx;
	    }
	    else {
		E.selected[1] = E.cy;
		E.selected[3] = E.cx;
	    }
	    if (E.selected[0] == E.selected[1] && E.selected[2] > E.selected[3]) {
		int temp = E.selected[2];
		E.selected[2] = E.selected[3];
		E.selected[3] = temp;
		hl_dir = !hl_dir;
	    }
	    else if (E.selected[0] > E.selected[1]) {
		int temp = E.selected[0];
		E.selected[0] = E.selected[1];
		E.selected[1] = temp;
		temp = E.selected[2];
		E.selected[2] = E.selected[3];
		E.selected[3] = temp;
		hl_dir = !hl_dir;
	    }
	    redrawLine[E.cy-E.rowoff] |= REDRAW_DEF;
	    if (c == ARROW_UP && E.cy+1-E.rowoff < E.screenrows) redrawLine[E.cy+1-E.rowoff] |= REDRAW_DEF;
	    else if (E.cy-1-E.rowoff >= 0) redrawLine[E.cy-1-E.rowoff] |= REDRAW_DEF;
            break;
            
        case ARROW_LEFT:
	    selectMoveCursor(c);
     	    if (hl_dir == LEFT) {    
		if (E.cy < E.selected[0]) E.selected[0] = E.cy;
		E.selected[2] = E.cx;
	    }
	    else {
		if (E.cy < E.selected[1]) E.selected[1] = E.cy;
		E.selected[3] = E.cx;
	    }
	    if (E.selected[0] == E.selected[1] && E.selected[2] > E.selected[3]) {
		int temp = E.selected[2];
		E.selected[2] = E.selected[3];
		E.selected[3] = temp;
		hl_dir = !hl_dir;
	    }
	    if (E.cy+1-E.rowoff < E.screenrows) redrawLine[E.cy+1-E.rowoff] |= REDRAW_DEF;
	    redrawLine[E.cy-E.rowoff] |= REDRAW_DEF;
            break;
            
        case ARROW_RIGHT:
	    selectMoveCursor(c);
	    if (hl_dir == RIGHT) {
        	if (E.cy > E.selected[1]) E.selected[1] = E.cy;
		E.selected[3] = E.cx;
	    }
	    else {
		if (E.cy > E.selected[0]) E.selected[0] = E.cy;
		E.selected[2] = E.cx;
	    }
	    if (E.selected[0] == E.selected[1] && E.selected[2] > E.selected[3]) {
		int temp = E.selected[2];
		E.selected[2] = E.selected[3];
		E.selected[3] = temp;
		hl_dir = !hl_dir;
	    }
	    if (E.cy-1-E.rowoff > 0) redrawLine[E.cy-1-E.rowoff] |= REDRAW_DEF; 
	    redrawLine[E.cy-E.rowoff] |= REDRAW_DEF;
            break;

	default:
		break;
	}
}

void editorMoveLine() {
	char* query = editorPrompt("Line number: %s", NULL);
	int line = atoi(query);
	if (line > 0 && line <= E.numrows) {
		E.cx = 0;
		E.cy = line-1;
	}	
	for (int i = 0; i < E.screenrows; i++) {
		redrawLine[i] |= REDRAW_DEF;
	}
}

void editorPaste() {
	if (E.cpbuffer == NULL) return;
	size_t size = 0;
	int prev_cy = E.cy;
	if (E.cy == E.numrows) editorInsertRow(E.numrows,"",0);
	while (E.cpbuffer[size] != '\0') {
		while(E.cpbuffer[size] == '\r') {
			editorInsertNewline();
			size++;
		}
		editorRowInsertChar(&E.row[E.cy], E.cx, E.cpbuffer[size++]);
		E.cx++;
	}	
	int start = prev_cy - E.rowoff;
	if (start < 0) start = 0;
	int end = E.cy - E.rowoff + 1;
	for (int i = start; i < end; i++) {
		redrawLine[i] |= REDRAW_DEF;
	}
}

void editorDelSelect() {
	int prev_cy = E.cy;
	E.cy = E.selected[0];
	E.cx = E.selected[2];
	for ( int i = E.selected[1]-1; i > E.selected[0]; i-- ) {
		editorDelRow(i);
		E.selected[1]--;
	}
	if (E.selected[0] == E.selected[1]) {
		for ( int i = E.selected[3]; i >= E.selected[2]; i-- ) editorRowDelChar(&E.row[E.selected[0]], i);
	}
	else {
		for ( int i = E.row[E.selected[0]].size; i >= E.selected[2]; i-- ) {
			editorRowDelChar(&E.row[E.selected[0]], i);
		}
		for ( int i = E.selected[3]; i >= 0; i-- ) {
			editorRowDelChar(&E.row[E.selected[1]], i);
		}
		erow* row = &E.row[E.selected[1]];
		editorRowAppendString(&E.row[E.selected[0]], row->chars, row->size);
		editorDelRow(E.selected[1]);
	}
	int start = E.cy - E.rowoff;
	int end = (prev_cy == E.cy) ? E.cy + 1 : E.screenrows;
	for (int i = start; i < end; i++) {
		redrawLine[i] |= REDRAW_DEF;
	}
}

void editorInsertChar(int c) {
	if (E.cy == E.numrows) {
		editorInsertRow(E.numrows,"", 0);
	}
	editorRowInsertChar(&E.row[E.cy], E.cx, c);
	long sec, nsec;
	if (E.tree.curr != NULL) {
		struct timespec timestamp;
		clock_gettime(CLOCK_MONOTONIC, &timestamp);
		sec = timestamp.tv_sec - E.tree.curr->timestamp.tv_sec;
		nsec = timestamp.tv_nsec - E.tree.curr->timestamp.tv_nsec;
		sec = sec * 1000 + nsec / 1000000;
	}
	else sec = 0;
	if (E.urType == DELETE || sec > UNDO_TIMEOUT || E.urType == NULL_UR) addNode(WRITE, E.cx, E.cy, c);
	else appendUrChar(c);
	E.urType = WRITE;
	E.cx++;
	if (E.cy-E.rowoff < 0) return;
	redrawLine[E.cy-E.rowoff] |= REDRAW_DEF;
}

void editorInsertNewline() {
	long sec, nsec;
	if (E.tree.curr != NULL) {
		struct timespec timestamp;
		clock_gettime(CLOCK_MONOTONIC, &timestamp);
		sec = timestamp.tv_sec - E.tree.curr->timestamp.tv_sec;
		nsec = timestamp.tv_nsec - E.tree.curr->timestamp.tv_nsec;
		sec = sec * 1000 + nsec / 1000000;
	}
	else sec = 0;
	if (E.urType == DELETE || sec > UNDO_TIMEOUT || E.urType == NULL_UR) addNode(WRITE, E.cx, E.cy, '\r');
	else appendUrChar('\r');
	if (E.cx == 0) {
		editorInsertRow(E.cy, "", 0);
	}
	else {
		erow* row = &E.row[E.cy];
		editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
		row = &E.row[E.cy];
		row->size = E.cx;
		row->chars[row->size] = '\0';
		editorUpdateRow(row, &E.syn, NORMAL);
	}
	E.cy++;
	E.cx = 0;
	int line = E.cy-1-E.rowoff;
	if (line < 0) return;
	for ( int i = line; i < E.screenrows; i++ ) {
		redrawLine[i] |= REDRAW_DEF;
	}
	E.urType = WRITE;
}

void editorInsertNewlineUR() {
	if (E.cx == 0) {
		editorInsertRow(E.cy, "", 0);
	}
	else {
		erow* row = &E.row[E.cy];
		editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
		row = &E.row[E.cy];
		row->size = E.cx;
		row->chars[row->size] = '\0';
		editorUpdateRow(row, &E.syn, NORMAL);
	}
	E.cy++;
	E.cx = 0;
	int line = E.cy-1-E.rowoff;
	if (line < 0) return;
	for ( int i = line; i < E.screenrows; i++ ) {
		redrawLine[i] |= REDRAW_DEF;
	}
}

void editorDelChar() {
	if (E.cy == E.numrows) return;
	if (E.cx == 0 && E.cy == 0) return;
	erow *row = &E.row[E.cy];
	long sec, nsec;
	if (E.tree.curr != NULL) {
		struct timespec timestamp;
		clock_gettime(CLOCK_MONOTONIC, &timestamp);
		sec = timestamp.tv_sec - E.tree.curr->timestamp.tv_sec;
		nsec = timestamp.tv_nsec - E.tree.curr->timestamp.tv_nsec;
		sec = sec * 1000 + nsec / 1000000;
	}
	else sec = 0;
	if (E.cx > 0) {
		if (E.urType == WRITE || sec > UNDO_TIMEOUT || E.urType == NULL_UR) addNode(DELETE, E.cx, E.cy, row->chars[E.cx-1]);
		else appendUrChar(row->chars[E.cx-1]);
		editorRowDelChar(row, E.cx - 1);
		E.cx--;
		if (E.cy-E.rowoff < 0) return;
		redrawLine[E.cy-E.rowoff] |= REDRAW_DEF;
	}
	else {
		E.cx = E.row[E.cy-1].size;
		editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
		editorDelRow(E.cy);
		E.cy--;
		int line = E.cy-E.rowoff;
		if (line < 0) return;
		for ( int i = line; i < E.screenrows; i++ ) {
			redrawLine[i] |= REDRAW_DEF;
		}
		if (E.urType == WRITE || sec > UNDO_TIMEOUT || E.urType == NULL_UR) addNode(DELETE, E.cx, E.cy, '\r');
		else appendUrChar('\r');
	}
	E.urType = DELETE;
}

void editorDelCharUR() {
	if (E.cy == E.numrows) return;
	if (E.cx == 0 && E.cy == 0) return;
	erow *row = &E.row[E.cy];
	if (E.cx > 0) {
		editorRowDelChar(row, E.cx - 1);
		E.cx--;
		if (E.cy-E.rowoff < 0) return;
		redrawLine[E.cy-E.rowoff] |= REDRAW_DEF;
	}
	else {
		E.cx = E.row[E.cy-1].size;
		editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
		editorDelRow(E.cy);
		E.cy--;
		int line = E.cy-E.rowoff;
		if (line < 0) return;
		for ( int i = line; i < E.screenrows; i++ ) {
			redrawLine[i] |= REDRAW_DEF;
		}
	}
}

int isSeparator(int c) {
	return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[]{};:", c) != NULL;
}

void editorUpdateSyntax(erow *row, editorSyntax* syn, char mode) {
	row->hl = realloc(row->hl, row->rsize);
	memset(row->hl, NORM, row->rsize);
	if (syn->filetype == NULL) return;

	size_t slc_len = syn->slComment ? strlen(syn->slComment) : 0;
	size_t mlcs_len = syn->mlCommentStart ? strlen(syn->mlCommentStart) : 0;
	size_t mlce_len = syn->mlCommentEnd ? strlen(syn->mlCommentEnd) : 0;	

	int in_comment = (row->ind > 0 && (row-1)->hl_open_comment);
	int in_string = (row->ind > 0 && (row-1)->hl_open_string);
	int prev_sep = 1;

	for (int i = 0; i < row->rsize; i++) {
		char c = row->render[i];
		unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : NORMAL;

		if (slc_len && !in_comment && !in_string) {
			if (!strncmp(&row->render[i],syn->slComment,slc_len)) {
				memset(&row->hl[i], COMMENT, row->rsize - i);
				break;
			}
		}

		if (mlcs_len && mlce_len && !in_string) {
			if (in_comment) {
				row->hl[i] = COMMENT;
				if (!strncmp(&row->render[i], syn->mlCommentEnd, mlce_len)) {
					memset(&row->hl[i], COMMENT, mlce_len);
					i += mlce_len-1;
					in_comment = 0;
					prev_sep = 1;
				}
				continue;
			}
			else {
				if (!strncmp(&row->render[i], syn->mlCommentStart, mlcs_len)) {
					memset(&row->hl[i], COMMENT, mlcs_len);
					i += mlcs_len-1;
					in_comment = 1;
					continue;
				}
			}
		}

		if (syn->flags & HGHLT_STRING) {
			if (in_string) {
				row->hl[i] = STRING;
				if (c == '\\' && i + 1 < row->rsize) {
					row->hl[i+1] = STRING;
					i++;
					continue;
				}
				if (i + 1 == row->rsize && (syn->flags & HGHLT_ML_STRINGS || c == '\\')) row->hl_open_string = 1;
				if (c == in_string) {
					in_string = 0;
					row->hl_open_string = 0;
				}
				prev_sep = 1;
				continue;
			}
			else {
				if (c == '"' || c == '\'') {
					in_string = c;
					row->hl[i] = STRING;
					continue;
				}
			}
		}

		if (syn->flags & HGHLT_NUM) {
			if ((isdigit(c) && (prev_sep || prev_hl == NUMBER)) || (c == '.' && prev_hl == NUMBER) || (c == 'f' && prev_hl == NUMBER)) {
				row->hl[i] = NUMBER;
				prev_sep = 0;
				continue;
			}
		}
		
		if (prev_sep) {
			int found = 0;
			for (int j = 0; j < syn->keywordCount; j++) {
				int klen = strlen(syn->keywords[j]);
				if (!strncmp(&row->render[i],syn->keywords[j],klen) && (i+klen <= row->rsize && isSeparator(row->render[i+klen]))) {
					memset(&row->hl[i], KEYWORD, klen);
					found = 1;
					i += klen-1;
					prev_sep = 0;
					break;
				}
			}
			if (found) continue;
			for (int j = 0; j < syn->typeCount; j++) {
				int tlen = strlen(syn->types[j]);
				if(!strncmp(&row->render[i],syn->types[j],tlen) && (i+tlen <= row->rsize && isSeparator(row->render[i+tlen]))) {
					memset(&row->hl[i], TYPE, tlen);
					found = 1;
					i += tlen-1;
					prev_sep = 0;
					break;
				}
			}
			if (found) continue;
		}

		prev_sep = isSeparator(c);		
	}
	int changed = (row->hl_open_comment != in_comment);
	row->hl_open_comment = in_comment;
	if (changed && row->ind + 1 < (mode ? E.win.numrows : E.numrows)) {
		redrawLine[row->ind - (mode ? E.win.yOffset : E.rowoff)] |= (mode ? REDRAW_WIN : REDRAW_DEF);
		editorUpdateSyntax(row+1, syn, mode);
	}
}

int editorSyntaxToColor(int hl) {
	if (E.colorful) {
		switch (hl) {
			case COMMENT: return CL_COMMENT;
			case MATCH: return CL_MATCH;
			case TYPE: return CL_TYPE;
			case KEYWORD: return CL_KEYWORD;
			case STRING: return CL_STRING;
			case NUMBER: return CL_NUMBER;
			default: return 97;
		}
	}	
	switch (hl) {
		case COMMENT: return 36;
		case MATCH: return 35;
		case TYPE: return 34;
		case KEYWORD: return 33;
		case STRING: return 32;
		case NUMBER: return 31;
		default: return 97;
	}
}
