#ifndef EDITOR_H
#define EDITOR_H
#include "data.h"

void exitSelect();
void editorHghlt(int c);
void editorMoveLine();
void editorPaste();
void editorDelSelect();
void editorInsertChar(int c);
void editorInsertNewline();
void editorInsertNewlineUR();
void editorDelChar();
void editorDelCharUR();
void editorUpdateSyntax(erow *row, editorSyntax* syn, char mode);
int editorSyntaxToColor(int hl);

#endif
