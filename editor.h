#ifndef EDITOR_H
#define EDITOR_H
#include "data.h"

void exitSelect();
void editorSelectKeypress(int c);
void editorMoveLine();
void editorPaste();
void editorCopy();
void editorDelSelect();
void editorInsertChar(int c);
void editorInsertCharRange(char* text, size_t size);
void editorInsertNewline();
void editorDelChar();
void editorDelRowsRange(int startrow, int endrow, int startrow_x, int endrow_x);

#endif
