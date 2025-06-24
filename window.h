#ifndef WINDOW_H
#define WINDOW_H
#include "ab.h"

void windowSetup(char location, int minCols, int divider, void (*winHandler)(char c), char* header);
void clearWindow();
void drawWindow(struct abuf* ab, int y);
int windowAddRow(char* text, int row, size_t len);
void windowDelRow(int row);
void windowSetRow(char* text, int row, size_t len);

#endif
