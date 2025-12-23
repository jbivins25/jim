#ifndef EDITOR_H
#define EDITOR_H

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
void editorUpdateSyntax(erow *row);
int editorSyntaxToColor(int hl);

#endif
