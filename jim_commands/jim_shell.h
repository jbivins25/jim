#ifndef JIM_SHELL
#define JIM_SHELL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compat.h"
#ifndef _WIN32
#include <sys/wait.h>
#endif
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
void Worker(void* param) {
	ThreadArgs* t = (ThreadArgs*)param;
	int argc = t->argc;
	char** args = t->args;
	int id = t->id;
	free(t);

	if ( argc < 1 ) {
		THREAD_LOCK(T.setMessageLock);
		editorSetStatusMessage("Terminal Thread Error: %d, Not enough arguments", -111);
		THREAD_UNLOCK(T.setMessageLock);
		for (int i = 0; i < argc; i++) {
			free(args[i]);
		}
		free(args);
		return;
	}
	int pipefd[2];
	if (pipe(pipefd) == -1) {
		THREAD_LOCK(T.setMessageLock);
		editorSetStatusMessage("Terminal Thread Error: %d, Could not create pipe", -112);
		THREAD_UNLOCK(T.setMessageLock);
		for (int i = 0; i < argc; i++) {
			free(args[i]);
		}
		free(args);
		return;
	}
	pid_t pid = fork();
	if (pid == 0) {
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
		execvp(args[0], (char* const*)args);
		exit(-101);
	}
	if (pid < 0) {
		THREAD_LOCK(T.setMessageLock);
		editorSetStatusMessage("Terminal Thread Error: %d, Fork failed," -113);
		THREAD_UNLOCK(T.setMessageLock);
		for (int i = 0; i < argc; i++) {
			free(args[i]);
		}
		free(args);
		return;
	}
	close(pipefd[1]);

	int ret_val;
	FILE* fp = fdopen(pipefd[0], "r");
	char* line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	linelen = getline(&line, &linecap, fp);
	char redrawChar = 'r';

	do {
		while (linelen > 0) {
			while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) linelen--;
			
			THREAD_LOCK(T.threadLock);
			if (!T.slot[id].writeEnabled) { THREAD_UNLOCK(T.threadLock); linelen = 0; continue; }
			if(windowAddRow(line, E.win.numrows, linelen) < 0) clearWindow();
			THREAD_UNLOCK(T.threadLock);
		
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
			THREAD_LOCK(T.eventPipeLock);
			write(eventPipe[1], &redrawChar, 1);
			THREAD_UNLOCK(T.eventPipeLock);
			linelen = getline(&line, &linecap, fp);
		}
	} while ((linelen = getline(&line, &linecap, fp)) != -1 || waitpid(pid, &ret_val, WNOHANG) == 0);

	ret_val = (signed char)WEXITSTATUS(ret_val);
	if (WEXITSTATUS(ret_val) != 0) {
		if (ret_val != -101) {
			THREAD_LOCK(T.setMessageLock);
			editorSetStatusMessage("Terminal Thread Error: %d, General Thread Error", ret_val);
			THREAD_UNLOCK(T.setMessageLock);
		}
		else {
			THREAD_LOCK(T.setMessageLock);
			editorSetStatusMessage("Terminal Thread Error: %d, execvp failed", ret_val);
			THREAD_UNLOCK(T.setMessageLock);
		}
	}

	free(line);
	fclose(fp);
	for (int i = 0; i < argc; i++) {
		free(args[i]);
	}
	free(args);
	return;
}

#else
int win_execvp(char* program, char* argv[], int argc, HANDLE hStdOut, PROCESS_INFORMATION* pi) {
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

unsigned __stdcall Worker(void* lpParam) {
	ThreadArgs* t = (ThreadArgs*)lpParam;
	int argc = t->argc;
	char** args = t->args;
	int id = t->id;
	free(t);

	if (argc < 1) {
		THREAD_LOCK(T.setMessageLock);
		editorSetStatusMessage("Terminal Thread Error: %d, Not enough arguments", -111);
		THREAD_UNLOCK(T.setMessageLock);
		for (int i = 0; i < argc; i++) {
			free(args[i]);
		}
		free(args);
		return 1;
	}

	SECURITY_ATTRIBUTES sa = {0};
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	
	HANDLE hRead = NULL, hWrite = NULL;

	if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
		THREAD_LOCK(T.setMessageLock);
		editorSetStatusMessage("Terminal Thread Error: %d, Could not create pipe", -112);
		THREAD_UNLOCK(T.setMessageLock);
		return 1;
	}

	SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);
	
	PROCESS_INFORMATION pi;

	if (win_execvp(args[0], args, argc, hWrite, &pi) != 0) {
		CloseHandle(hRead);
		CloseHandle(hWrite);
		THREAD_LOCK(T.setMessageLock);
		editorSetStatusMessage("Terminal Thread Error: %d, execvp failed", -113);
		THREAD_UNLOCK(T.setMessageLock);
		for (int i = 0; i < argc; i++) {
			free(args[i]);
		}
		free(args);
		return 1;
	}
	
	CloseHandle(hWrite);

	char buf[4096];
	ZeroMemory(buf, sizeof(buf));
	char line[8192];
	size_t lineLen = 0;
	DWORD bytesRead;
	char e = 'r';

	while(1) {
		if (!PeekNamedPipe(hRead, NULL, 0, NULL, &bytesRead, NULL)) break;
		if (bytesRead == 0) continue;

		BOOL success = ReadFile(hRead, buf, min(bytesRead,sizeof(buf)), &bytesRead, NULL);
		
		if (!success) {
			if (GetLastError() == ERROR_BROKEN_PIPE) break;
		}

		for (DWORD i = 0; i < bytesRead; i++) {
			char c = buf[i];
			if (c == '\r') continue;
			if (c == '\n') {
				THREAD_LOCK(T.threadLock);
				if (!T.slot[id].writeEnabled) {
					lineLen = 0;
					THREAD_UNLOCK(T.threadLock);
					continue;
				}
				if (windowAddRow(line, E.win.numrows, (int)lineLen) < 0) clearWindow();
				THREAD_UNLOCK(T.threadLock);
				lineLen = 0;
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
				THREAD_LOCK(T.eventPipeLock);
				WriteFile(eWritePipe, &e, 1, NULL, NULL);
				THREAD_UNLOCK(T.eventPipeLock);
				SetEvent(hEvents[1]);
			}
			else {
				if (lineLen < sizeof(line) - 1) line[lineLen++] = c;
				else { 
					THREAD_LOCK(T.threadLock);
					if (!T.slot[id].writeEnabled) {
						lineLen = 0;
						THREAD_UNLOCK(T.threadLock);
						continue;
					}
					if (windowAddRow(line, E.win.numrows, (int)lineLen) < 0) clearWindow();
					THREAD_UNLOCK(T.threadLock);
					lineLen = 0; 
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
			
					THREAD_LOCK(T.eventPipeLock);
					WriteFile(eWritePipe, &e, 1, NULL, NULL);
					THREAD_UNLOCK(T.eventPipeLock);
					SetEvent(hEvents[1]);
				}
			}
		}
		
	}

	if (lineLen) {
		THREAD_LOCK(T.threadLock);
		if (T.slot[id].writeEnabled) {
			windowAddRow(line, E.win.numrows, (int)lineLen);
			THREAD_UNLOCK(T.threadLock);

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
	
			THREAD_LOCK(T.eventPipeLock);
			WriteFile(eWritePipe, &e, 1, NULL, NULL);
			THREAD_UNLOCK(T.eventPipeLock);
			SetEvent(hEvents[1]);
		}
		else THREAD_UNLOCK(T.threadLock);
	}
	CloseHandle(hRead);

	WaitForSingleObject(pi.hProcess,INFINITE);

	DWORD exitCode;
	GetExitCodeProcess(pi.hProcess, &exitCode);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	if (exitCode != 0) {
		switch (exitCode) {
			case(0x1):
				THREAD_LOCK(T.setMessageLock);
				editorSetStatusMessage("Terminal Thread Error: %d, Process encountered failure", -101);
				THREAD_UNLOCK(T.setMessageLock);
				break;
			case(0x2):
			case(0x3):
				THREAD_LOCK(T.setMessageLock);
				editorSetStatusMessage("Terminal Thread Error: %d, File or path not found", -102);
				THREAD_UNLOCK(T.setMessageLock);
				break;
			case(0x5):
				THREAD_LOCK(T.setMessageLock);
				editorSetStatusMessage("Terminal Thread Error: %d, Access denied", -105);
				THREAD_UNLOCK(T.setMessageLock);
				break;
			case(0xC0000005):
				THREAD_LOCK(T.setMessageLock);
				editorSetStatusMessage("Terminal Thread Error: %d, Access violation", -107);
				THREAD_UNLOCK(T.setMessageLock);
				break;
			case(0x8):
			case(0xC00000FD):
				THREAD_LOCK(T.setMessageLock);
				editorSetStatusMessage("Terminal Thread Error: %d, Not enough memory or stack overflow", -108);
				THREAD_UNLOCK(T.setMessageLock);
				break;
			default:
				THREAD_LOCK(T.setMessageLock);
				editorSetStatusMessage("Terminal Thread Error: %d, Undefined exit error code", -109);
				THREAD_UNLOCK(T.setMessageLock);
				break;
		}
		for (int i = 0; i < argc; i++) {
			free(args[i]);
		}
		free(args);
		return 1;
	}

	for (int i = 0; i < argc; i++) {
		free(args[i]);
	}
	free(args);
	return 0;
}
#endif

int jim_shell(const int argc, const char* args[]) {
	if (argc < 1) return -1;
	if (!E.win.active || strcmp("Terminal", E.win.header)) { 
		char* header = malloc(9);
		strcpy(header, "Terminal");
		windowSetup((THREAD_OWNED | WINDOW_RIGHT), 10, 2, shellProcessKey, header);
	}
	
	int commandLen = 6;
	for (int i = 0; i < argc; i++) commandLen += strlen(args[i]);
	char* command = malloc(commandLen);
	int offset = snprintf(command, commandLen, ">>> %s ", args[0]);
	for (int i = 1; i < argc; i++) {
		size_t len = strlen(args[i]);
		memcpy(command+offset, args[i], len);
		offset += len;

		command[offset++] = (i < argc - 1) ? ' ' : '\0';
	}
	windowAddRow(command, E.win.numrows, commandLen-1);
	free(command);

	char** argCopy = malloc((argc+1)*sizeof(char*));

	for (int i = 0; i < argc; i++) {
		argCopy[i] = strdup(args[i]);
	}

	argCopy[argc] = NULL;

	ThreadArgs* tArgs = malloc(sizeof(ThreadArgs));
	tArgs->argc = argc;
	tArgs->args = argCopy;

	int slot = editorThreadCreate(Worker, (void*)tArgs);
	editorThreadLinkWindow(slot);

	return 0;
}

#endif
