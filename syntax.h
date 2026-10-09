#ifndef SYNTAX_H
#define SYNTAX_H
#include "data.h"

void loadSyntax(const char* filename, editorSyntax* syn);
void freeSyntax(editorSyntax* syn);
void editorUpdateSyntax(erow *row, int rowind, editorSyntax* syn, char mode);
int editorSyntaxToColor(int hl);
void editorGetSyntax(erow* row, int rowind, unsigned char* buf, int bufsize, editorSyntax* syn);

#endif
