#include "data.h"
#include "terminal.h"
#include "row.h"
#include "jimio.h"
#include "fileio.h"
#include "window.h"
#include "ur.h"
#include "compat.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define SYN_PATH "%s\\jim\\jim_%s.syn"
#else
#define SYN_PATH "%s/.jim/jim_%s.syn"
#endif

struct editorConfig E;
char redrawLine[SCREEN_ROW_MAX] = {0};
int redrawWholeScreen = 1;

void loadSyntax(char* filename) {
	char* ext = strrchr(filename, '.');
	if (!ext) return;
	size_t len = strlen(++ext);
	if (len < 1) return;
	#ifndef _WIN32
	const char *home = getenv("HOME");
	#else
	const char *home = getenv("APPDATA");
	#endif
	if (!home) die("Couldn't find home");
	char file[512];
	snprintf(file, len+16+strlen(home), "%s/.jim/jim_%s.syn", home, ext);
	FILE* f = fopen(file,"r");
	if (f == NULL) return;
	E.syn.filetype = malloc(len+1);
	strcpy(E.syn.filetype,ext);
	char* line = NULL;
	size_t cap = 0;
	getline(&line, &cap, f);
	sscanf(line, "%d %d", &E.syn.keywordCount, &E.syn.typeCount);
	E.syn.keywords = malloc(sizeof(char*)*E.syn.keywordCount);
	E.syn.keywordLen = malloc(sizeof(char)*E.syn.keywordCount);
	char* line_t;
	for ( int i = 0; i < E.syn.keywordCount; i++ ) {
		len = getline(&line, &cap, f);
		while (line[len-1] == '\n' || line[len-1] == '\r') len--;
		line_t = malloc(len+1);
		for (size_t j = 0; j < len+1; j++) line_t[j] = line[j];
		line_t[len] = '\0';
		E.syn.keywords[i] = line_t;
		E.syn.keywordLen[i] = len;
	}
	E.syn.types = malloc(sizeof(char*)*E.syn.typeCount);
	for ( int i = 0; i < E.syn.typeCount; i++ ) {
		len = getline(&line, &cap, f);
		while (line[len-1] == '\n' || line[len-1] == '\r') len--;
		line_t = malloc(len+1);
		for (size_t j = 0; j < len+1; j++) line_t[j] = line[j];
		line_t[len] = '\0';
		E.syn.types[i] = line_t;
	}
	getline(&line, &cap, f);
	sscanf(line, "%d", &E.syn.flags);
	if (E.syn.flags & HGHLT_SL_CM) {
		len = getline(&line, &cap, f);
		while (line[len-1] == '\n' || line[len-1] == '\r') len--;
		line_t = malloc(len+1);
		for (size_t j = 0; j < len+1; j++) line_t[j] = line[j];
		line_t[len] = '\0';
		E.syn.slComment = line_t;
	}
	if (E.syn.flags & HGHLT_ML_CM) {
		len = getline(&line, &cap, f);
		while (line[len-1] == '\n' || line[len-1] == '\r') len--;
		line_t = malloc(len+1);
		for (size_t j = 0; j < len+1; j++) line_t[j] = line[j];
		line_t[len] = '\0';
		E.syn.mlCommentStart = line_t;
		len = getline(&line, &cap, f);
		while (line[len-1] == '\n' || line[len-1] == '\r') len--;
		line_t = malloc(len+1);
		for (size_t j = 0; j < len+1; j++) line_t[j] = line[j];
		line_t[len] = '\0';
		E.syn.mlCommentEnd = line_t;
	}
	free(line);
	fclose(f);
}

void freeSyntax(editorSyntax* syn) {
	free(syn->filetype);
	for (int i = 0; i < syn->keywordCount; i++) {
		free(syn->keywords[i]);
	}
	free(syn->keywords);
	for (int i = 0; i < syn->typeCount; i++) {
		free(syn->types[i]);
	}
	free(syn->types);
	free(syn->slComment);
	free(syn->mlCommentStart);
	free(syn->mlCommentEnd);
}

void freeEditor() {
	if (E.win.active) clearWindow();
	for (int i = 0; i < E.numrows; i++) {
		editorFreeRow(&E.row[i]);
	}
	free(E.row);	
	free(E.filename);
	free(E.cpbuffer);	
	for (int i = 0; i < E.win.numrows; i++) editorFreeRow(&E.win.row[i]);
	free(E.win.row);
	freeTree(&E.tree);
	freeSyntax(&E.syn);
	write(STDOUT_FILENO, "\x1b[?1049l", 8);
}

void initEditor() {
	E.cx = 0;
	E.cy = 0;
	E.rx = 0;
	E.rowoff = 0;
	E.numrows = 0;
	E.coloff = 0;
	for (int i = 0; i < 4; i++) {
		E.selected[i] = -1;
	}
	E.dirty = 0;
	E.mode = NORMAL;
	E.cpbuffer = NULL;
	E.filename = NULL;
	E.statusmsg[0] = '\0';
	E.statusmsg_time = 0;
	if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
	E.screenrows -= 2;
	E.win.active = E.win.location = 0;
	E.win.minCols = E.win.screencols = E.win.screenrows = 0;
	E.win.divider = 0;
	E.win.xOffset = E.win.yOffset = 0;
	E.win.handler = 0;
	E.win.row = NULL;
	E.win.numrows = 0;
	E.syn = (editorSyntax){NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0};
	initTree(&E.tree);
	E.urType = NULL_UR;
	E.urMode = 1;
	E.capacity = STARTING_CAPACITY;
	E.row = malloc(sizeof(erow)*E.capacity);
	const char *term = getenv("TERM");
	if (!term) E.colorful =  0;
	else E.colorful = strstr(term, "256color") != NULL;
	E.sticky = 0;
	write(STDOUT_FILENO, "\x1b[?1049h", 8);
}

#ifndef _WIN32
static void win_sighandler(int sig) {
	if (SIGWINCH == sig) {
		//write(STDOUT_FILENO, "\x1b[2J", 4);
		if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
		E.screenrows -= 2;
		if (E.win.active) {
			E.win.screencols = E.screencols/E.win.divider;
			if (E.win.screencols < E.win.minCols) {
				clearWindow();
			}
			else {
				E.screencols -= E.win.screencols;
				E.win.screenrows = E.screenrows;
			}
		}
		redrawWholeScreen = 1;
		editorRefreshScreen();
	}
}
#endif

int main(int argc, char *argv[]) {
	enableRawMode();
	setupCrashHandler();
	initEditor();
	atexit(freeEditor);
	#ifndef _WIN32
	signal(SIGWINCH, win_sighandler);
	#endif
	write(STDOUT_FILENO, "\x1b[2J", 4); //Clear up screen on start
	write(STDOUT_FILENO, "\x1b[H", 3);
	if (argc >= 2) {
		loadSyntax(argv[1]);
		editorOpen(argv[1]);
	}

	editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");

	while (1) {
		editorRefreshScreen();
		editorProcessKeypress();
	}
	return 0;
}
