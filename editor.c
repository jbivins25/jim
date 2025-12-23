#include "data.h"
#include "editor.h"
#include "row.h"
#include "jimio.h"
#include "ur.h"
#include "palette.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

void exitSelect() {
	for (int i = (E.selected[0] - E.rowoff < 0) ? 0 : E.selected[0] - E.rowoff; i < E.selected[1]+1; i++) {
		redrawLine[i] = (redrawLine[i] == 2 || redrawLine[i] == 3 ) ? 3 : 1;
	}
	E.selected[0] = E.selected[1] = E.selected[2] = E.selected[3] = -1;
	E.mode = NORMAL;	
}

void editorHghlt(int c) {
	switch (c) {
        case CTRL_KEY('c'): {
	    free(E.cpbuffer);
	    size_t size = 0;
	    for (int i = E.selected[0]; i <= E.selected[1]; i++) {
		if (i == E.selected[0] && i == E.selected[1]) {
			size += E.selected[3] - E.selected[2] + 1;
		}
		else if (i == E.selected[0]) {
			size += E.row[i].size - E.selected[2] + 1;
		}
		else if (i > E.selected[0] && i < E.selected[1]) {
			size += E.row[i].size + 1;
		}
		else {
			size += E.selected[3] + 1;
		}
	    }
	    size += 1;
	    E.cpbuffer = malloc(size);
	    unsigned int ind = 0;
	    for (int i = E.selected[0]; i <=E.selected[1]; i++) {
		if (i == E.selected[0] && i != E.selected[1]) {
			for (int j = E.selected[2]; j < E.row[i].size; j++) {
				E.cpbuffer[ind++] = E.row[i].chars[j];
				if (j == E.row[i].size-1) {
					E.cpbuffer[ind++] = '\r';
				}
			}
		}
		else if (i == E.selected[0] && i == E.selected[1]) {
			for (int j = E.selected[2]; j <= E.selected[3]; j++) {
				E.cpbuffer[ind++] = E.row[i].chars[j];
			}
		}
		else if (i > E.selected[0] && i < E.selected[1]) {
			for (int j = 0; j < E.row[i].size; j++) {
				E.cpbuffer[ind++] = E.row[i].chars[j];
				if (j == E.row[i].size-1) {
					E.cpbuffer[ind++] = '\r';
				}
			}
			if (E.row[i].size == 0) E.cpbuffer[ind++] = '\r';
		} 
		else {
			for (int j = 0; j <= E.selected[3]; j++) {
				E.cpbuffer[ind++] = E.row[i].chars[j];
			}
		}
	    }
	    E.cpbuffer[ind] = '\0';
	    }
	    editorSetStatusMessage("Copied! Selected text: {%d,%d,%d,%d}", E.selected[0], E.selected[1], E.selected[2], E.selected[3]);
            break;

        case '\r':
	case CTRL_KEY('w'):
        case CTRL_KEY('q'):
        case CTRL_KEY('l'):
        case '\x1b': 
            //{ editorSetStatusMessage("E.selected: {%d,%d,%d,%d}", E.selected[0], E.selected[1], E.selected[2], E.selected[3]); }
	    exitSelect();
            break;

        case CTRL_KEY('f'):
	    break;
            
        case HOME_KEY:
            break;
            
        case END_KEY:            
            break;
            
	case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
		editorDelSelect();
		exitSelect();
		break;

        case PAGE_UP:
        case PAGE_DOWN:
            break;
        
        case ARROW_UP:
            if (E.cy == E.selected[0]-1 && E.cx < E.row[E.cy].size) {
                E.selected[0] = E.cy; //Case where we move up a line and are on text at the start
                E.selected[2] = E.cx;
            }
	    else if (E.cy == E.selected[0]-1 && E.cx == 0 && E.row[E.cy].size == 0) {
		E.selected[0] = E.cy;
		E.selected[2] = E.cx;
	    }
            else if (E.cy == E.selected[1]-1 && E.cx < E.row[E.cy].size) {
                E.selected[1] = E.cy; //Case where we move up a line while at the end
                E.selected[3] = E.cx;
            }
            else if (E.cy == E.selected[1]-1 && E.cx == 0 && E.row[E.cy].size == 0) {
		E.selected[1] = E.cy; //Case where the line is just empty space
		E.selected[3] = E.cx;
	    }
            if (E.selected[1] < E.selected[0] || (E.selected[1] == E.selected[0] && E.selected[3] < E.selected[2])) {
                int temp = E.selected[0];
                E.selected[0] = E.selected[1];
                E.selected[1] = temp;
                temp = E.selected[2];
                E.selected[2] = E.selected[3];
                E.selected[3] = temp;
            }
	    if (redrawLine[E.cy-E.rowoff] == 2 || redrawLine[E.cy-E.rowoff] == 3) redrawLine[E.cy-E.rowoff] = 3;
	    else redrawLine[E.cy-E.rowoff] = 1;
	    if (redrawLine[E.cy+1-E.rowoff] == 2 || redrawLine[E.cy+1-E.rowoff] == 3) redrawLine[E.cy+1-E.rowoff] = 3;
	    else redrawLine[E.cy+1-E.rowoff] = 1;
            break;
            
        case ARROW_DOWN:
            if (E.cy >= E.numrows) break;
            if (E.selected[0] != E.selected[1] && E.cy == E.selected[0]+1 && E.cx < E.row[E.cy].size) {
                E.selected[0] = E.cy; //Case where we move down a line and are on text at the start
                E.selected[2] = E.cx;
            }
	    else if (E.selected[0] != E.selected[1] && E.cy == E.selected[0]+1 && E.cx == 0 && E.row[E.cy].size == 0) {
		E.selected[0] = E.cy;
		E.selected[2] = E.cx;
	    }
            else if (E.cy == E.selected[1]+1 && E.cx < E.row[E.cy].size) {
                E.selected[1] = E.cy; //Case where we move down a line while at the end
                E.selected[3] = E.cx;
            }
	    else if (E.cy == E.selected[1]+1 && E.cx == 0 && E.row[E.cy].size == 0) {
		E.selected[1] = E.cy;
		E.selected[3] = E.cx;
	    }
                     
            if (E.selected[1] < E.selected[0] || (E.selected[1] == E.selected[0] && E.selected[3] < E.selected[2])) {
                int temp = E.selected[0];
                E.selected[0] = E.selected[1];
                E.selected[1] = temp;
                temp = E.selected[2];
                E.selected[2] = E.selected[3];
                E.selected[3] = temp;
            }
	    if (redrawLine[E.cy-E.rowoff] == 2 || redrawLine[E.cy-E.rowoff] == 3) redrawLine[E.cy-E.rowoff] = 3;
	    else redrawLine[E.cy-E.rowoff] = 1;
	    if (redrawLine[E.cy-1-E.rowoff] == 2 || redrawLine[E.cy-1-E.rowoff] == 3) redrawLine[E.cy-1-E.rowoff] = 3;
	    else redrawLine[E.cy-1-E.rowoff] = 1;
            break;
            
        case ARROW_LEFT:
            if (E.cy == E.selected[0] && E.cx == E.selected[2]-1) {
                E.selected[2] = E.cx; //Case where we are at the start of the selected area and simply move left
            }
            else if (E.cy == E.selected[1] && E.cx == E.selected[3]-1) {
                E.selected[3] = E.cx; //Case where we are the end of the selected area and simply move left
            }
            else if (E.cy == E.selected[0]-1 && E.cx == E.row[E.selected[0]-1].size-1) {
                E.selected[0] = E.cy; //Case where we move up a line and are on text at the start
                E.selected[2] = E.cx;
            }
	    else if (E.cy == E.selected[0]-1 && E.cx == 0 && E.row[E.cy].size == 0) {
		E.selected[0] = E.cy;
		E.selected[2] = E.cx;
	    }
            else if (E.cy == E.selected[1]-1 && E.cx == E.row[E.selected[1]-1].size-1) {
                E.selected[1] = E.cy; //Case where we move up a line while at the end
                E.selected[3] = E.cx;
            }
	    else if (E.cy == E.selected[1]-1 && E.cx == 0 && E.row[E.cy].size == 0) {
		E.selected[1] = E.cy;
		E.selected[3] = E.cx;
	    }
                     
            if (E.selected[1] < E.selected[0] || (E.selected[1] == E.selected[0] && E.selected[3] < E.selected[2])) {
                int temp = E.selected[0];
                E.selected[0] = E.selected[1];
                E.selected[1] = temp;
                temp = E.selected[2];
                E.selected[2] = E.selected[3];
                E.selected[3] = temp;
            }
	    if (redrawLine[E.cy-E.rowoff] == 2 || redrawLine[E.cy-E.rowoff] == 3) redrawLine[E.cy-E.rowoff] = 3;
	    else redrawLine[E.cy-E.rowoff] = 1;
            break;
            
        case ARROW_RIGHT:
            if (E.cy >= E.numrows) break;
            if (E.cy == E.selected[0] && E.cx == E.selected[2]+1) {
                E.selected[2] = E.cx; //Case where we are at the start of the selected area and simply move right
            }
            else if (E.cy == E.selected[1] && E.cx == E.selected[3]+1) {
                E.selected[3] = E.cx; //Case where we are the end of the selected area and simply move right
            }
            else if (E.cy == E.selected[0]+1 && E.cx == 0 && E.selected[0] != E.selected[1]) {
                E.selected[0] = E.cy; //Case where we move down a line and are on text at the start
                E.selected[2] = E.cx;
            }
            else if (E.cy == E.selected[1]+1 && E.cx == 0) {
                E.selected[1] = E.cy; //Case where we move down a line while at the end
                E.selected[3] = E.cx;
            }
                     
            if (E.selected[1] < E.selected[0] || (E.selected[1] == E.selected[0] && E.selected[3] < E.selected[2])) {
                int temp = E.selected[0];
                E.selected[0] = E.selected[1];
                E.selected[1] = temp;
                temp = E.selected[2];
                E.selected[2] = E.selected[3];
                E.selected[3] = temp;
            }
	    if (redrawLine[E.cy-E.rowoff] == 2 || redrawLine[E.cy-E.rowoff] == 3) redrawLine[E.cy-E.rowoff] = 3;
	    else redrawLine[E.cy-E.rowoff] = 1;
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
	redrawWholeScreen = 1;
}

void editorPaste() {
	size_t size = 0;
	while (E.cpbuffer[size] != '\0') {
		if (E.cy == E.numrows) {
			editorInsertRow(E.numrows,"",0);
		}
		editorRowInsertChar(&E.row[E.cy], E.cx, E.cpbuffer[size++]);
		E.cx++;
		while (E.cpbuffer[size] == '\r') {
			editorInsertNewline();
			size++;
		}
	}	
	redrawWholeScreen = 1;
}

void editorDelSelect() {
	E.cy = E.selected[0];
	E.cx = E.selected[2];
	int init_row_size = E.row[E.cy].size;
	if (E.selected[0] != E.selected[1] && E.row[E.selected[1]].size-1 == E.selected[3]) editorDelRow(E.selected[0]+1);
	else if (E.selected[0] != E.selected[1]) {
		for ( int i = E.selected[3]; i >= 0; i-- ) {
			editorRowDelChar(&E.row[E.selected[0]+1], i);
		}
		erow* row = &E.row[E.selected[1]];
		editorRowAppendString(&E.row[E.selected[0]], row->chars, row->size);
		editorDelRow(E.selected[1]);
	}
	for ( int i = E.selected[1]-1; i > E.selected[0]; i-- ) {
		editorDelRow(i);
	}
	if (E.selected[0] == E.selected[1]) {
		for ( int i = E.selected[3]; i >= E.selected[2]; i-- ) {
			editorRowDelChar(&E.row[E.selected[0]], i);
		}
	}
	else {
		for ( int i = init_row_size-1; i >= E.selected[2]; i--) {
			editorRowDelChar(&E.row[E.selected[0]], i);
		}
	}
	redrawWholeScreen = 1;
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
	if (redrawLine[E.cy-E.rowoff] == 2 || redrawLine[E.cy-E.rowoff] == 3) redrawLine[E.cy-E.rowoff] = 3;
	else redrawLine[E.cy-E.rowoff] = 1;
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
		editorUpdateRow(row);
	}
	E.cy++;
	E.cx = 0;
	int line = E.cy-1-E.rowoff;
	if (line < 0) return;
	if (line == 0) {redrawWholeScreen = 1; return;}
	for ( int i = line; i < E.screenrows; i++ ) {
		if (redrawLine[i] > 1) redrawLine[i] = 3;
		else redrawLine[i] = 1;
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
		editorUpdateRow(row);
	}
	E.cy++;
	E.cx = 0;
	int line = E.cy-1-E.rowoff;
	if (line < 0) return;
	if (line == 0) {redrawWholeScreen = 1; return;}
	for ( int i = line; i < E.screenrows; i++ ) {
		if (redrawLine[i] > 1) redrawLine[i] = 3;
		else redrawLine[i] = 1;
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
		if (redrawLine[E.cy-E.rowoff] == 2 || redrawLine[E.cy-E.rowoff] == 3) redrawLine[E.cy-E.rowoff] = 3;
		else redrawLine[E.cy-E.rowoff] = 1;
	}
	else {
		E.cx = E.row[E.cy-1].size;
		editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
		editorDelRow(E.cy);
		E.cy--;
		int line = E.cy-E.rowoff;
		if (line < 0) return;
		if (line == 0) {redrawWholeScreen = 1; return;}
		for ( int i = line; i < E.screenrows; i++ ) {
			if (redrawLine[i] > 1) redrawLine[i] = 3;
			else redrawLine[i] = 1;
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
		if (redrawLine[E.cy-E.rowoff] == 2 || redrawLine[E.cy-E.rowoff] == 3) redrawLine[E.cy-E.rowoff] = 3;
		else redrawLine[E.cy-E.rowoff] = 1;
	}
	else {
		E.cx = E.row[E.cy-1].size;
		editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
		editorDelRow(E.cy);
		E.cy--;
		int line = E.cy-E.rowoff;
		if (line < 0) return;
		if (line == 0) {redrawWholeScreen = 1; return;}
		for ( int i = line; i < E.screenrows; i++ ) {
			if (redrawLine[i] > 1) redrawLine[i] = 3;
			else redrawLine[i] = 1;
		}
	}
}

int isSeparator(int c) {
	return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[]{};", c) != NULL;
}

void editorUpdateSyntax(erow *row) {
	row->hl = realloc(row->hl, row->rsize);
	memset(row->hl, NORM, row->rsize);
	if (E.syn.filetype == NULL) return;

	size_t slc_len = E.syn.slComment ? strlen(E.syn.slComment) : 0;
	size_t mlcs_len = E.syn.mlCommentStart ? strlen(E.syn.mlCommentStart) : 0;
	size_t mlce_len = E.syn.mlCommentEnd ? strlen(E.syn.mlCommentEnd) : 0;	

	int in_comment = (row->ind > 0 && E.row[row->ind-1].hl_open_comment);
	int in_string = (row->ind > 0 && E.row[row->ind-1].hl_open_string);
	int prev_sep = 1;

	for (int i = 0; i < row->rsize; i++) {
		char c = row->render[i];
		unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : NORMAL;

		if (slc_len && !in_comment && !in_string) {
			if (!strncmp(&row->render[i],E.syn.slComment,slc_len)) {
				memset(&row->hl[i], COMMENT, row->rsize - i);
				break;
			}
		}

		if (mlcs_len && mlce_len && !in_string) {
			if (in_comment) {
				row->hl[i] = COMMENT;
				if (!strncmp(&row->render[i], E.syn.mlCommentEnd, mlce_len)) {
					memset(&row->hl[i], COMMENT, mlce_len);
					i += mlce_len-1;
					in_comment = 0;
					prev_sep = 1;
				}
				continue;
			}
			else {
				if (!strncmp(&row->render[i], E.syn.mlCommentStart, mlcs_len)) {
					memset(&row->hl[i], COMMENT, mlcs_len);
					i += mlcs_len-1;
					in_comment = 1;
					continue;
				}
			}
		}

		if (E.syn.flags & HGHLT_STRING) {
			if (in_string) {
				row->hl[i] = STRING;
				if (c == '\\' && i + 1 < row->rsize) {
					row->hl[i+1] = STRING;
					i++;
					continue;
				}
				if (i + 1 == row->rsize && (E.syn.flags & HGHLT_ML_STRINGS || c == '\\')) row->hl_open_string = 1;
				if (c == in_string) in_string = 0;
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

		if (E.syn.flags & HGHLT_NUM) {
			if ((isdigit(c) && (prev_sep || prev_hl == NUMBER)) || (c == '.' && prev_hl == NUMBER)) {
				row->hl[i] = NUMBER;
				prev_sep = 0;
				continue;
			}
		}
		
		if (prev_sep) {
			int found = 0;
			for (int j = 0; j < E.syn.keywordCount; j++) {
				int klen = strlen(E.syn.keywords[j]);
				if (!strncmp(&row->render[i],E.syn.keywords[j],klen)) {
					memset(&row->hl[i], KEYWORD, klen);
					found = 1;
					i += klen-1;
					prev_sep = 0;
					break;
				}
			}
			if (found) continue;
			for (int j = 0; j < E.syn.typeCount; j++) {
				int tlen = strlen(E.syn.types[j]);
				if(!strncmp(&row->render[i],E.syn.types[j],tlen)) {
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
	if (changed && row->ind + 1 < E.numrows)
	editorUpdateSyntax(&E.row[row->ind + 1]);
	redrawWholeScreen = 1;
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
