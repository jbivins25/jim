#ifndef JIM_SHELL
#define JIM_SHELL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../compat.h"
#ifndef _WIN32
#include <sys/wait.h>
#endif
#include "../data.h"
#include "../window.h"
#include "../jimio.h"
#include "../terminal.h"
#include "../command.h"

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
		case '!':
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

#ifndef _WIN32
int jim_shell(const int argc, const char* args[]) {
	if ( argc < 1 ) return -101;
	int pipefd[2];
	if (pipe(pipefd) == -1) return -102;
	pid_t pid = fork();
	if (pid == 0) {
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
		execvp(args[0], (char* const*)args);
		exit(-103);
	}
	if (pid < 0) return -104;
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
		if(windowAddRow(line, E.win.numrows, linelen) < 0) clearWindow();
	}
	if (E.win.numrows > E.win.screenrows - 1) {
		E.win.yOffset = E.win.numrows - E.win.screenrows + 1;
	}
	else {
		E.win.yOffset = 0;
	}
	E.win.xOffset = 0;
	for (int i = 0; i < E.win.screenrows; i++) {
		redrawLine[i] |= REDRAW_WIN;
	}
	free(line);
	fclose(fp);
	return 0;
}

#else
int win_execvp(const char* program, const char* argv[], const int argc, HANDLE hStdOut, PROCESS_INFORMATION* pi) {
	STARTUPINFOA si;
	ZeroMemory(&si, sizeof(si));
	ZeroMemory(pi, sizeof(*pi));

	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	si.hStdOutput = hStdOut;
	si.hStdError = hStdOut;

	char exe[MAX_PATH] = {0};
	
	DWORD len = SearchPathA(NULL, program, ".exe", MAX_PATH, exe, NULL);

	if (len >= MAX_PATH) return -1;

	char cmdline[4096] = {0};

	if (len == 0) {
		strcat(cmdline, "cmd.exe /C ");
	}
	for (int i = 0; i < argc; i++) {
		if (i) strcat(cmdline, " ");
		strcat(cmdline, "\"");
		strcat(cmdline, argv[i]);
		strcat(cmdline, "\"");
	}

	if (!CreateProcessA(exe, cmdline, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, pi)) return -2;

	return 0;
}

int jim_shell(const int argc, const char* args[]) {
	if (argc < 1) return -111;

	SECURITY_ATTRIBUTES sa = {0};
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	
	HANDLE hRead = NULL, hWrite = NULL;

	if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return -112;

	SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);
	
	PROCESS_INFORMATION pi;

	if (win_execvp(args[0], args, argc, hWrite, &pi) != 0) {
		CloseHandle(hRead);
		CloseHandle(hWrite);
		return -113;
	}
	
	CloseHandle(hWrite);

	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD exitCode;
	GetExitCodeProcess(pi.hProcess, &exitCode);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	if (exitCode != 0) {
		CloseHandle(hRead);
		switch (exitCode) {
			case(0x1):
				return -101;
			case(0x2):
			case(0x3):
				return -102;
			case(0x5):
				return -105;
			case(0xC0000005):
				return -107;
			case(0x8):
			case(0xC00000FD):
				return -108;
			default:
				return -109;
		}
	}

	if (!E.win.active || strcmp("Terminal", E.win.header)) { 
		char* header = malloc(9);
		strcpy(header, "Terminal");
		windowSetup(1, 10, 2, shellProcessKey, header);
	}

	char buf[4096];
	ZeroMemory(buf, sizeof(buf));
	char line[8192];
	size_t lineLen = 0;
	DWORD bytesRead;

	while (ReadFile(hRead, buf, sizeof(buf), &bytesRead, NULL) && bytesRead > 0) {
		for (DWORD i = 0; i < bytesRead; i++) {
			char c = buf[i];
			if (c == '\r') continue;
			if (c == '\n') {
				if (windowAddRow(line, E.win.numrows, (int)lineLen) < 0) clearWindow();
				lineLen = 0;
			}
			else {
				if (lineLen < sizeof(line) - 1) line[lineLen++] = c;
			}
		}
	}

	if (lineLen) windowAddRow(line, E.win.numrows, (int)lineLen);

	CloseHandle(hRead);
	if (E.win.numrows > E.win.screenrows - 1) {
		E.win.yOffset = E.win.numrows - E.win.screenrows + 1;
	}
	else {
		E.win.yOffset = 0;
	}
	E.win.xOffset = 0;
	THREAD_LOCK(T.redrawLock);
	for (int i = 0; i < E.win.screenrows; i++) {
		redrawLine[i] |= REDRAW_WIN;
	}
	THREAD_UNLOCK(T.redrawLock);
	return 0;
}

#endif

#endif
