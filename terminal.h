#ifndef TERMINAL_H
#define TERMINAL_H
#include <signal.h>

#define KEY_NONE -1

void die(const char* s);
void disableRawMode();
void enableRawMode();
int editorReadKey();
int getCursorPosition(int* rows, int* cols);
int getWindowSize(int* rows, int* cols);
void setupCrashHandler();

#endif
