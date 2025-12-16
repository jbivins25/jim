#ifndef JIM_SHELL
#define JIM_SHELL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "data.h"
#include "window.h"
#include "jimio.h"
#include "terminal.h"
#include "command.h"

void shellProcessKey(int c) {
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

		case '\r':
			editorCommand();
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

int jim_shell(const int argc, const char* args[]) {
	if ( argc < 1 ) return -1;
	int pipefd[2];
	if (pipe(pipefd) == -1) return -2;
	pid_t pid = fork();
	if (pid == 0) {
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
		execvp(args[0], (char* const*)args);
		exit(-3);
	}
	if (pid < 0) return -3;
	close(pipefd[1]);
	int ret_val;
	waitpid(pid, &ret_val, 0);
	if (WEXITSTATUS(ret_val) != 0) return (signed char)WEXITSTATUS(ret_val);
	if (!E.win.active || strcmp("Terminal", E.win.header)) { 
		char* header = malloc(9);
		strcpy(header, "Terminal");
		windowSetup(1, 10, 2, shellProcessKey, header);
	}
	FILE* fp = fdopen(pipefd[0], "r");
	char* line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	while ((linelen = getline(&line, &linecap, fp)) != -1) {
		while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) linelen--;
		windowAddRow(line, E.win.numrows, linelen);
		if (E.win.numrows-E.win.yOffset >= E.win.screenrows) {E.win.yOffset++; memset(redrawLine, 2, E.win.screenrows);}
		else redrawLine[E.win.numrows-E.win.yOffset] = (redrawLine[E.win.numrows-E.win.yOffset] == 1 || redrawLine[E.win.numrows-E.win.yOffset] == 3) ? 3 : 2;
	}
	free(line);
	fclose(fp);
	return 0;
}

#endif
