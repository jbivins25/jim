#ifndef TERMINAL_H
#define TERMINAL_H
#include <signal.h>

static struct sigaction old_sigbus;
static struct sigaction old_sigsegv;
static struct sigaction old_sigabrt;

void die(const char* s);
void disableRawMode();
void enableRawMode();
int editorReadKey();
int getCursorPosition(int* rows, int* cols);
int getWindowSize(int* rows, int* cols);
void crashHandler(int sig, siginfo_t* info, void* ctx);
void setupCrashHandler();

#endif
