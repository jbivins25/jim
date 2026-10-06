#ifndef UR_H
#define UR_H
#include "data.h"

void initTree(urTree* tree);
void addNode(char type, int startx, int starty, char c);
void appendUrChar(char c, int endx, int endy);
void appendUrCharRange(char* text, size_t len, int endx, int endy);
void undo();
void redo();
void freeNode(urBlock* node);
void freeTree(urTree* tree);
void drawTree();
void treeProcessKey(int c);

#endif
