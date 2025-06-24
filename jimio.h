#ifndef JIMIO_H
#define JIMIO_H

#include "data.h"
#include "ab.h"

//Output
void editorScroll();
void editorDrawRows(struct abuf *ab);
void editorDrawStatusBar(struct abuf* ab);
void editorDrawMessageBar(struct abuf* ab);
void editorRefreshScreen();
void editorSetStatusMessage(const char* fmt, ...);
//Input
char* editorPrompt(char* prompt, void (*callback)(char *, int));
void editorMoveCursor(int key);
void editorProcessKeypress();

#endif
