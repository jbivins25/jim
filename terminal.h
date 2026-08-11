#ifndef TERMINAL_H
#define TERMINAL_H
#include <signal.h>

#define EVENT_INPUT 0x1
#define EVENT_QUEUE 0x2

void die(const char* s);
void disableRawMode();
void enableRawMode();
int editorReadKey();
int getCursorPosition(int* rows, int* cols);
int getWindowSize(int* rows, int* cols);
void setupCrashHandler();
int editorReadEvent();
char terminalWaitEvent();
#ifndef _WIN32
#ifdef WIN_SIG
static void win_sighandler(int sig);
#endif
#endif

#endif
