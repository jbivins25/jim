#ifndef SYNTAX_H
#define SYNTAX_H
#include "data.h"

void loadSyntax(const char* filename, editorSyntax* syn);
void freeSyntax(editorSyntax* syn);

#endif
