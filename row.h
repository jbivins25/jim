#ifndef ROW_H
#define ROW_H

#include "data.h"
#include <stdlib.h>

int editorRowCxToRx(erow* row, int cx);
int editorRowRxToCx(erow* row, int rx);
void editorHghltRow(erow *row, int start, int end);
void editorUpdateRow(erow* row);
void editorInsertRow(int at, char* s, size_t len);
void editorFreeRow(erow* row);
void editorDelRow(int at);
void editorRowInsertChar(erow* row, int at, int c);
void editorRowAppendString(erow* row, char* s, size_t len);
void editorRowDelChar(erow* row, int at);

#endif
