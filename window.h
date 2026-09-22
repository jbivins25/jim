#ifndef WINDOW_H
#define WINDOW_H
#include "ab.h"

#define WINDOW_LEFT	(0 << 0)
#define WINDOW_RIGHT	(1 << 0)
#define THREAD_OWNED	(1 << 1)

void windowSetup(unsigned char flags, int minCols, int divider, void (*winHandler)(int c), char* header);
void clearWindow();
void drawWindow(struct abuf* ab, int y);
int windowAddRow(char* text, int row, size_t len);
void windowDelRow(int row);
void windowSetRow(char* text, int row, size_t len);
void windowPageScroll(int c);
void windowClearRows();

#endif
