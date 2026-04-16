#include "data.h"
#include "editor.h"
#include "fileio.h"
#include "jimio.h"
#include "find.h"
#include "command.h"
#include "row.h"
#include "terminal.h"
#include "window.h"
#include "ur.h"
#include "palette.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>


void editorScroll() {
	int init_rowoff, init_coloff;
	init_rowoff = E.rowoff;
	init_coloff = E.coloff;
	if (E.cy < E.numrows) {
		E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
	}

	if (E.cy < E.rowoff) { //Scroll up
		E.rowoff = E.cy;
	}
	if (E.cy >= E.rowoff + E.screenrows) { //Scroll down
		E.rowoff = E.cy - E.screenrows + 1;
	}
	if (E.rx < E.coloff) { //Scroll left
		E.coloff = E.rx;
	}
	if (E.rx >= E.coloff + E.screencols) { //Scroll right
		E.coloff = E.rx - E.screencols + 1;
	}
	if (E.rowoff != init_rowoff || E.coloff != init_coloff) {
		for (int i = 0; i < E.screenrows; i++) {
			redrawLine[i] |= REDRAW_DEF;
		}
	}
}

void editorDrawRows(struct abuf* ab) {
	char buf[32];
	static int set = 1;
	static char def_fg[8];
	static int def_fg_len;
	static char def_bg[8];
	static int def_bg_len;
	static char hl_bg[8];
	static int hl_bg_len;
	if (set) {
		set = 0;
		def_fg_len = snprintf(def_fg, sizeof(def_fg), "\x1b[%dm", DEF_FG);
		def_bg_len = snprintf(def_bg, sizeof(def_bg), "\x1b[%dm", DEF_BG);
		hl_bg_len = snprintf(hl_bg, sizeof(hl_bg), "\x1b[%dm", HL_BG);
	}
	for (int y = 0; y < E.screenrows; y++) { 
		if ( redrawWholeScreen || redrawLine[y] ) {
			abAppend(ab, def_fg, def_fg_len);
			abAppend(ab, def_bg, def_bg_len);
			snprintf(buf, sizeof(buf), "\x1b[%d;%dH", y+1, ((!redrawWholeScreen || redrawLine[y] == REDRAW_DEF) && E.win.active && E.win.location == 0) ? E.win.screencols+1 : 0);
			abAppend(ab, buf, strlen(buf));
			if ( (redrawWholeScreen || redrawLine[y] & REDRAW_WIN) && (E.win.active && E.win.location == 0) ) drawWindow(ab, y);
			if ( redrawWholeScreen || redrawLine[y] & REDRAW_DEF ) {
				if (E.win.active && E.win.location == 1 && redrawLine[y] == 1) {
					snprintf(buf, sizeof(buf), "\x1b[%d;%dH", y+1, E.screencols);
					abAppend(ab, buf, strlen(buf));
					abAppend(ab, "\x1b[1K", 4);
					snprintf(buf, sizeof(buf), "\x1b[%d;%dH", y+1, 0);
					abAppend(ab, buf, strlen(buf));
				}
				int filerow = y + E.rowoff;
				if ( filerow >= E.numrows) {
					abAppend(ab, "~", 1);
				}
				else {
					if (filerow > E.selected[0] && filerow < E.selected[1]) abAppend(ab, hl_bg, hl_bg_len); //If between the start/end automatically highlight everything
					if (filerow == E.selected[1] && E.selected[0] != E.selected[1] && E.coloff <= editorRowCxToRx(&E.row[filerow],E.selected[3])) abAppend(ab, hl_bg, hl_bg_len); //If we are rendering the final row and we haven't gotten to the end of highlighting 
					int len = E.row[filerow].rsize - E.coloff;
					if (len < 0) len = 0;
					if (len > E.screencols) len = E.screencols;
					int current_color = DEF_FG; //default color
					unsigned char* hl = &E.row[filerow].hl[E.coloff];
					for (int j = 0; j < len; j++) {
						int color = editorSyntaxToColor(hl[j]);
						if (filerow == E.selected[0] && j + E.coloff == editorRowCxToRx(&E.row[filerow], E.selected[2])) {
							abAppend(ab, hl_bg, hl_bg_len);
						}
						if (color != current_color) {
							char buf[16];
							int hlen;
							if (E.colorful == 0 || color == DEF_FG) hlen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
							else hlen = snprintf(buf, sizeof(buf), "\x1b[38;5;%dm", color);
							abAppend(ab, buf, hlen);
							current_color = color;
						}
						abAppend(ab, &E.row[filerow].render[E.coloff + j], 1);
						if (filerow == E.selected[1] && j < len-1 && j + E.coloff == editorRowCxToRx(&E.row[filerow], E.selected[3])) {
							abAppend(ab,def_bg,def_bg_len);
						}
					}
				}
			
				abAppend(ab,def_fg,def_fg_len); //Sets default text color
				abAppend(ab,def_bg,def_bg_len); //Sets default background color
				if (!E.win.active || !(E.win.location == 1) || redrawLine[y] != 1) abAppend(ab, "\x1b[K", 3); //Erases part of the line to the right
			}
			if ( (redrawWholeScreen || redrawLine[y] & REDRAW_WIN) && (E.win.active && E.win.location == 1) ) drawWindow(ab, y);
			abAppend(ab, "\r\n", 2);
			redrawLine[y] = 0;
		}
	}
	redrawWholeScreen = 0;
}

void editorDrawStatusBar(struct abuf *ab) {
	char buf[32];
	int len;
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.screenrows+1, 0);
	abAppend(ab, buf, strlen(buf));
	char* statusMode[3] = {"Normal","Select","Window"};
	len = snprintf(buf, sizeof(buf), "\x1b[%dm", INV_FG);
	abAppend(ab, buf, len); //Print with inverted colors
	len = snprintf(buf, sizeof(buf), "\x1b[%dm", INV_BG);
	abAppend(ab, buf, len);
	char status[80], rstatus[80];
	len = snprintf(status, sizeof(status), "%.20s - %d lines %s", E.filename ? E.filename : "[No name]", E.numrows, E.dirty ? "(modified)" : "");
	int rlen = snprintf(rstatus, sizeof(rstatus), "Mode: %s  %d/%d", statusMode[E.mode], E.cy + 1, E.numrows);
	if (len > E.screencols + E.win.screencols) len = E.screencols + E.win.screencols;
	abAppend(ab, status, len);
	while (len < E.screencols + E.win.screencols) {
		if (E.screencols + E.win.screencols - len == rlen) {
			abAppend(ab, rstatus, rlen);
			break;
		} 
		else {
			abAppend(ab, " ", 1);
			len++;
		}
	}
	len = snprintf(buf, sizeof(buf), "\x1b[%dm", DEF_FG);
	abAppend(ab, buf, len); //Reset to normal
	len = snprintf(buf, sizeof(buf), "\x1b[%dm", DEF_BG);
	abAppend(ab, buf, len);
	abAppend(ab, "\r\n", 2);
}

void editorDrawMessageBar(struct abuf *ab) {
	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.screenrows+2, 0);
	abAppend(ab, buf, strlen(buf));
	abAppend(ab, "\x1b[K", 3);
	int msglen = strlen(E.statusmsg);
	if (msglen > E.screencols + E.win.screencols) msglen = E.screencols + E.win.screencols;
	if (msglen && time(NULL) - E.statusmsg_time < 5) abAppend(ab, E.statusmsg, msglen);
}

void editorRefreshScreen() {
	editorScroll();

	struct abuf ab = ABUF_INIT;
	abAppend(&ab, "\x1b[?25l", 6); //Hide cursor
	abAppend(&ab, "\x1b[H", 3); //Positions cursor at top left
	editorDrawRows(&ab);
	editorDrawStatusBar(&ab);
	editorDrawMessageBar(&ab);
	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1 + (E.win.active && E.win.location == 0 ? E.win.screencols : 0) ); //Add one to deal with terminal cursor indexing
	abAppend(&ab, buf, strlen(buf));
	abAppend(&ab, "\x1b[?25h", 6); //View cursor

	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
	va_end(ap);
	E.statusmsg_time = time(NULL);
}

char* editorPrompt(char* prompt, void (*callback)(char *, int)) {
	size_t bufsize = 128;
	char *buf = malloc(bufsize);

	size_t buflen = 0;
	buf[0] = '\0';
	while(1) {
		editorSetStatusMessage(prompt, buf);
		editorRefreshScreen();
		
		int c = editorReadKey();
		
		if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
			if (buflen != 0) buf[--buflen] = '\0';
		}
		else if (c == '\x1b') {
			editorSetStatusMessage("");
			if (callback) callback(buf, c);
			free(buf);
			return NULL;
		}
		else if (c == '\r') {
			if (buflen != 0) {
				editorSetStatusMessage("");
				if (callback) callback(buf, c);
				return buf;
			}
		}
		else if (!iscntrl(c) && c < 128) {
			if (buflen == bufsize - 1) {
				bufsize *= 2;
				buf = realloc(buf, bufsize);
			}
			buf[buflen++] = c;
			buf[buflen] = '\0';
		}
		if (callback) callback(buf,c);
	}
}

void editorMatchMark() {
	char init = E.row[E.cy].chars[E.cx];
	char* currchars = E.row[E.cy].chars;
	int stack = 1;
	char charStack = 0;
	char prevChar = 0;
	switch (init) {
		case '(':
			while (stack > 0) {
				E.cx++;
				if (E.cx == E.row[E.cy].size && E.cy < E.numrows-1) {
					while(++E.cy < E.numrows && E.row[E.cy].size == 0);
					if (E.row[E.cy].size == 0) {E.cx = 0; break;}
					E.cx = 0;
				}
				else if (E.cx == E.row[E.cy].size && E.cy == E.numrows-1) break;
				currchars = E.row[E.cy].chars;
				if (currchars[E.cx] == ')' && !charStack) stack--;
				else if (currchars[E.cx] == '(' && !charStack) stack++;
				else if ((currchars[E.cx] == '\"' || currchars[E.cx] == '\'') && !charStack) {
					charStack = 1;
					prevChar = currchars[E.cx];
				}
				else if (charStack && currchars[E.cx] == prevChar && ( E.cx == 0 ? 1 : E.cx == 1 ? currchars[E.cx-1] != '\\' : currchars[E.cx-1] != '\\' && currchars[E.cx-2] != '\\')) {
					charStack = prevChar = 0;
				}
			}
		break;

		case ')':
			while (stack != 0) {
				E.cx--;
				if (E.cx < 0 && E.cy > 0) {
					while (--E.cy > 0 && E.row[E.cy].size == 0);
					if (E.row[E.cy].size == 0) {E.cx = 0; break;}
					E.cx = E.row[E.cy].size-1;
				}
				else if (E.cy == 0 && E.cx == 0) {
					E.cx = 0;
					break;
				}
				currchars = E.row[E.cy].chars;
				if (E.row[E.cy].chars[E.cx] == '(' && !charStack) stack--;
				else if (E.row[E.cy].chars[E.cx] == ')' && !charStack) stack++;
				else if ((E.row[E.cy].chars[E.cx] == '\"' || E.row[E.cy].chars[E.cx] == '\'') && !charStack) {
					charStack = 1;
					prevChar = E.row[E.cy].chars[E.cx];
				}
				else if (charStack && currchars[E.cx] == prevChar && ( E.cx == 0 ? 1 : E.cx == 1 ? currchars[E.cx-1] != '\\' : currchars[E.cx-1] != '\\' && currchars[E.cx-2] != '\\')) {
					charStack = prevChar = 0;
				}
			}
			break;

		case '[':
			while (stack != 0) {
				E.cx++;
				if (E.cx == E.row[E.cy].size && E.cy < E.numrows-1) {
					while(++E.cy < E.numrows && E.row[E.cy].size == 0);
					if (E.row[E.cy].size == 0) {E.cx = 0; break;}
					E.cx = 0;
				}
				else if (E.cx == E.row[E.cy].size && E.cy == E.numrows-1) break;
				currchars = E.row[E.cy].chars;
				if (E.row[E.cy].chars[E.cx] == ']' && !charStack) stack--;
				else if (E.row[E.cy].chars[E.cx] == '[' && !charStack) stack++;
				else if ((E.row[E.cy].chars[E.cx] == '\"' || E.row[E.cy].chars[E.cx] == '\'') && !charStack) {
					charStack = 1;
					prevChar = E.row[E.cy].chars[E.cx];
				}
				else if (charStack && currchars[E.cx] == prevChar && ( E.cx == 0 ? 1 : E.cx == 1 ? currchars[E.cx-1] != '\\' : currchars[E.cx-1] != '\\' && currchars[E.cx-2] != '\\')) {
					charStack = prevChar = 0;
				}
			}	
			break;

		case ']':
			while (stack != 0) {
				E.cx--;
				if (E.cx < 0 && E.cy > 0) {
					while (--E.cy > 0 && E.row[E.cy].size == 0);
					if (E.row[E.cy].size == 0) {E.cx = 0; break;}
					E.cx = E.row[E.cy].size-1;
				}
				else if (E.cy == 0 && E.cx == 0) {
					E.cx = 0;
					break;
				}
				currchars = E.row[E.cy].chars;
				if (E.row[E.cy].chars[E.cx] == '[' && !charStack) stack--;
				else if (E.row[E.cy].chars[E.cx] == ']' && !charStack) stack++;
				else if ((E.row[E.cy].chars[E.cx] == '\"' || E.row[E.cy].chars[E.cx] == '\'') && !charStack) {
					charStack = 1;
					prevChar = E.row[E.cy].chars[E.cx];
				}
				else if (charStack && currchars[E.cx] == prevChar && ( E.cx == 0 ? 1 : E.cx == 1 ? currchars[E.cx-1] != '\\' : currchars[E.cx-1] != '\\' && currchars[E.cx-2] != '\\')) {
					charStack = prevChar = 0;
				}
			}
			break;

		case '{':
			while (stack != 0) {
				E.cx++;
				if (E.cx == E.row[E.cy].size && E.cy < E.numrows-1) {
					while(++E.cy < E.numrows && E.row[E.cy].size == 0);
					if (E.row[E.cy].size == 0) {E.cx = 0; break;}
					E.cx = 0;
				}
				else if (E.cx == E.row[E.cy].size && E.cy == E.numrows-1) break;
				currchars = E.row[E.cy].chars;
				if (E.row[E.cy].chars[E.cx] == '}' && !charStack) stack--;
				else if (E.row[E.cy].chars[E.cx] == '{' && !charStack) stack++;
				else if ((E.row[E.cy].chars[E.cx] == '\"' || E.row[E.cy].chars[E.cx] == '\'') && !charStack) {
					charStack = 1;
					prevChar = E.row[E.cy].chars[E.cx];
				}
				else if (charStack && currchars[E.cx] == prevChar && ( E.cx == 0 ? 1 : E.cx == 1 ? currchars[E.cx-1] != '\\' : currchars[E.cx-1] != '\\' && currchars[E.cx-2] != '\\')) {
					charStack = prevChar = 0;
				}
			}
			break;
					
		case '}':
			while (stack != 0) {
				E.cx--;
				if (E.cx < 0 && E.cy > 0) {
					while (--E.cy > 0 && E.row[E.cy].size == 0);
					if (E.row[E.cy].size == 0) {E.cx = 0; break;}
					E.cx = E.row[E.cy].size-1;
				}
				else if (E.cy == 0 && E.cx == 0) {
					E.cx = 0;
					break;
				}
				currchars = E.row[E.cy].chars;
				if (currchars[E.cx] == '{' && !charStack) stack--;
				else if (E.row[E.cy].chars[E.cx] == '}' && !charStack) stack++;
				else if ((E.row[E.cy].chars[E.cx] == '\"' || E.row[E.cy].chars[E.cx] == '\'') && !charStack) {
					charStack = 1;
					prevChar = E.row[E.cy].chars[E.cx];
				}
				else if (charStack && currchars[E.cx] == prevChar && ( E.cx == 0 ? 1 : E.cx == 1 ? currchars[E.cx-1] != '\\' : currchars[E.cx-1] != '\\' && currchars[E.cx-2] != '\\')) {
						charStack = prevChar = 0;
				}
			}
			break;

		default:
			break;
	}
}

void editorMoveCursor(int key) {
	erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
	switch (key) {
		case ARROW_LEFT:
			if (E.cx != 0) {
				E.cx--;
			}
			else if ( E.cy > 0 && E.mode == NORMAL ) {
				E.cy--;
				E.cx = E.row[E.cy].size;
			}
			else if ( E.cy > 0 && E.mode == SELECT ) {
				E.cy--;
				E.cx = E.row[E.cy].size-1;
				if (E.cx < 0) E.cx = 0;
			}
			break;
		case ARROW_RIGHT:
			if (row && E.cx < row->size && E.mode == NORMAL) {
				E.cx++;
			}
			else if (row && E.cx < row->size-1 && E.mode == SELECT) {
				E.cx++;
			}
			else if (row && E.cx == row->size && E.mode == NORMAL) {
				E.cy++;
				E.cx = 0;
			}
			else if (row && (E.cx == row->size-1 || (E.cx == 0 && E.cx == row->size)) && E.mode == SELECT) {
				E.cy++;
				E.cx = 0;
			}
			break;
		case ARROW_UP:
			if (E.cy != 0) {
				E.cy--;
			}
			break;
		case ARROW_DOWN:
			if (E.cy < E.numrows && E.mode == NORMAL) {
				E.cy++;
			}
			else if (E.cy < E.numrows-1 && E.mode == SELECT) {
				E.cy++;
			}
			break;
	}
	row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
	int rowlen = row ? row->size : 0;
	if (E.cx > rowlen && E.mode == NORMAL) { //Snap cursor back to end of line
		E.cx = rowlen;
	}
	else if (E.cx > rowlen && E.mode == SELECT) {
		if (rowlen > 0) {
			E.cx = rowlen-1;
		}
		else E.cx = rowlen;
	}
	E.rx = row ? editorRowCxToRx(row, E.cx) : 0;
}

void editorProcessKeypress() {
	static int quit_times = JIM_QUIT_TIMES;
	int c = editorReadKey();

	if (E.win.active && E.mode == WINDOW && E.win.handler) { E.win.handler(c); quit_times = JIM_QUIT_TIMES; return;}
	//if (E.mode == SELECT) {editorHghlt(c); quit_times = JIM_QUIT_TIMES; return;}

	switch(c) {
		case '\r': // CTRL_KEY('m') maps to this
			if (E.mode == SELECT) editorHghlt(c);
			editorInsertNewline();
			break;

		case CTRL_KEY('q'):
			if (E.mode == SELECT) editorHghlt(c);
			if (E.dirty && quit_times > 0) {
				editorSetStatusMessage("Warning: Unsaved changes. Press Ctrl-Q %d more times to quit.", quit_times);
				quit_times--;
				return;
			}
			write(STDOUT_FILENO, "\x1b[2J", 4); //Clear up screen on exit
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;

		case CTRL_KEY('s'):
			editorSave();
			break;

		case CTRL_KEY('w'):
			if (E.mode != WINDOW && E.win.active && E.win.handler) E.mode = WINDOW;
			else if (E.win.active && !E.win.handler) clearWindow();
			else E.mode = NORMAL;
			break;

		case CTRL_KEY('e'):
			if (E.mode == SELECT) {editorHghlt(c); break;}
			E.mode = SELECT;
			if (E.cx == E.row[E.cy].size && E.cx > 0) E.cx = E.cx-1;
			for (int i = 0; i < 4; i++) {
				if (i == 0 || i == 1)  E.selected[i] = E.cy;
				else E.selected[i] = E.cx;
			}
			break;

		case CTRL_KEY('a'):
			if (E.mode != SELECT) E.mode = SELECT;
			E.selected[0] = 0;
			E.selected[1] = E.numrows-1;
			E.selected[2] = 0;
			E.selected[3] = E.row[E.numrows-1].size-1;
			E.cx = E.row[E.numrows-1].size-1;
			E.cy = E.numrows-1;
			break;

		case CTRL_KEY('y'):
			if (E.mode == SELECT) break;
			redo();
			break; //Todo: redo

		case CTRL_KEY('z'):
			if (E.mode == SELECT) break;
			undo();
			break; //Todo: undo

		case CTRL_KEY('c'):
			if (E.mode == SELECT) editorHghlt(c);
			break;

		case CTRL_KEY('v'):
			if (E.cpbuffer == NULL) break;
			if (E.mode == SELECT) {
				editorDelSelect();
				exitSelect();
			}
			editorPaste();
			break;

		case CTRL_KEY('p'): 
			if (E.mode == SELECT) editorHghlt(c);
			editorMatchMark();
			break;

		case CTRL_KEY('f'):
			if (E.mode == SELECT) break;
			editorFind();
			break;

		case CTRL_KEY('l'):
			if (E.mode == SELECT) break;
			editorMoveLine();
			break;

		case CTRL_KEY('t'):
			//break;
			if ( E.win.active && !strcmp(E.win.header, "Tree")) clearWindow();
			else {
				windowSetup(0, 20, 5, treeProcessKey, strdup("Tree"));
				drawTree();
				if (E.win.screencols < E.win.minCols) clearWindow();
			}
			break;

		case CTRL_KEY('b'):
		case CTRL_KEY('d'):
		case CTRL_KEY('g'):
		case CTRL_KEY('j'):
		case CTRL_KEY('k'):
		case CTRL_KEY('n'):
		case CTRL_KEY('o'):
		case CTRL_KEY('r'):
		case CTRL_KEY('u'):
		case CTRL_KEY('x'):
			break;
            
		case HOME_KEY:
			if (E.mode == SELECT) editorHghlt(c);
			E.cx = 0;
			break;

		case END_KEY:
			if (E.cy < E.numrows) {
				if (E.mode == SELECT) editorHghlt(c);
				E.cx = E.row[E.cy].size;
			}
			break;

		case BACKSPACE:
		case CTRL_KEY('h'):
		case DEL_KEY: 
			if (E.mode == SELECT) {editorHghlt(c); break;}
			if (c == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
			editorDelChar();
			break;

		case PAGE_UP:
		case PAGE_DOWN:
			if (E.mode == SELECT) break; 
			{ //Need local scope to declare variable
				if (c == PAGE_UP) {
					E.cy = E.rowoff;
				}
				else if (c == PAGE_DOWN) {
					E.cy = E.rowoff + E.screenrows - 1;
					if (E.cy > E.numrows) E.cy = E.numrows;
				}

				int times = E.screenrows;
				while (times--)
					editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
			}
			break;
		
		case ARROW_UP:
		case ARROW_DOWN:
		case ARROW_LEFT:
		case ARROW_RIGHT:
			editorMoveCursor(c);
			if (E.mode == SELECT) editorHghlt(c);
			break;

		case '\x1b':
			if (E.mode == SELECT) editorHghlt(c);
			else editorCommand();
			break;

		default:
			if (E.mode == SELECT) break;
			editorInsertChar(c);
			break;
	}
	quit_times = JIM_QUIT_TIMES;
}
