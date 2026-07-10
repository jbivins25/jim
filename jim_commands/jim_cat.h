#ifndef JIM_CAT
#define JIM_CAT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../compat.h"
#include "../data.h"
#include "../window.h"
#include "../jimio.h"

void catProcessKey(int c) {
	static int quit_times = JIM_QUIT_TIMES;
	switch(c) {
		case CTRL_KEY('w'):
			E.mode = NORMAL;
			break;

		case CTRL_KEY('q'):
			if (E.dirty && quit_times > 0) {
				editorSetStatusMessage("Warning: Unsaved changes. Press Ctrl-Q %d more times to quit.", quit_times);
				quit_times--;
				return;
			}
			write(STDOUT_FILENO, "\x1b[2J", 4); //Clear up screen on exit
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;

		case '\x1b':
			clearWindow();
			E.mode = NORMAL;
			break;

		case ARROW_LEFT:
		case ARROW_RIGHT:
		case ARROW_UP:
		case ARROW_DOWN:
			windowPageScroll(c);
			break;
		default:
			break;
	}
	quit_times = JIM_QUIT_TIMES;
}

int jim_cat(const int argc, const char* args[]) {
	if ( argc < 1 ) return -1;
	FILE* fp = fopen(args[0], "r");
	if (!fp) return -1;
	windowSetup(1, 10, 2, catProcessKey, strdup(args[0]));
	char* line = NULL;
	size_t linecap = 0;
	ssize_t linelen;	
	while ((linelen = getline(&line, &linecap, fp)) != -1) {
		while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) linelen--;
		windowAddRow(line, E.win.numrows, linelen);
	}
	free(line);
	fclose(fp);
	return 0;
}

#endif
