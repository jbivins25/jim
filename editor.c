#include "data.h"
#include "editor.h"
#include "row.h"
#include "jimio.h"
#include <string.h>
#include <stdio.h>

void exitSelect() {
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
}

void editorInsertChar(int c) {
	if (E.cy == E.numrows) {
		editorInsertRow(E.numrows,"", 0);
	}
	editorRowInsertChar(&E.row[E.cy], E.cx, c);
	E.cx++;
}

void editorInsertNewline() {
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
}

void editorDelChar() {
	if (E.cy == E.numrows) return;
	if (E.cx == 0 && E.cy == 0) return;
	erow *row = &E.row[E.cy];
	if (E.cx > 0) {
		editorRowDelChar(row, E.cx - 1);
		E.cx--;
	}
	else {
		E.cx = E.row[E.cy-1].size;
		editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
		editorDelRow(E.cy);
		E.cy--;
	}
}
