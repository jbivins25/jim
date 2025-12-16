#ifndef UR_H
#define UR_H
#include "data.h"

void initTree(urTree* tree);
void addNode(char type, int startx, int starty, char c);
void appendUrChar(char c);
void undo();
void redo();
void freeNode(urBlock* node);
void freeTree(urTree* tree);
void drawTree();
void treeProcessKey(int c);

#endif
