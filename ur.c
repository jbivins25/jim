#include "data.h"
#include "ur.h"
#include "window.h"
#include "jimio.h"
#include "editor.h"
#include "row.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

void initTree(urTree* tree) {
	tree->root = malloc(sizeof(urBlock));
	tree->root->parent = NULL;
	tree->root->type = NULL_UR;
	tree->root->childlen = 0;
	tree->root->children = NULL;
	tree->root->chars = NULL;
	tree->curr = tree->root;
}

void addNode(char type, int startx, int starty, char c) {
	if (E.tree.curr->childlen == 0) {
		E.tree.curr->children = malloc(sizeof(urBlock*));
	}
	else {
		E.tree.curr->children = realloc(E.tree.curr->children,sizeof(urBlock*)*(E.tree.curr->childlen+1));
	}
	E.tree.curr->children[E.tree.curr->childlen] = malloc(sizeof(urBlock));
	E.tree.curr->children[E.tree.curr->childlen]->parent = E.tree.curr;
	E.tree.curr = E.tree.curr->children[E.tree.curr->childlen++];
	E.tree.curr->type = type;
	E.tree.curr->start[0] = startx;
	E.tree.curr->start[1] = starty;
	if (type == WRITE) {
		E.tree.curr->end[0] = startx + 1;
		E.tree.curr->end[1] = starty;
	}
	else {
		if (startx > 0) {
			E.tree.curr->end[0] = startx-1;
			E.tree.curr->end[1] = starty;
		}
		else {
			E.tree.curr->end[0] = E.row[starty-1].size;
			E.tree.curr->end[1] = starty-1;
		}
	}
	E.tree.curr->chars = malloc(sizeof(char));
	E.tree.curr->chars[0] = c;
	E.tree.curr->length = 1;
	E.tree.curr->children = NULL;
	E.tree.curr->childlen = 0;
	clock_gettime(CLOCK_MONOTONIC, &E.tree.curr->timestamp);
}

void appendUrChar(char c) {
	E.tree.curr->chars = realloc(E.tree.curr->chars,sizeof(char)*(E.tree.curr->length+1));
	E.tree.curr->chars[E.tree.curr->length++] = c;	
	E.tree.curr->end[0] = E.cx;
	E.tree.curr->end[1] = E.cy;
	clock_gettime(CLOCK_MONOTONIC, &E.tree.curr->timestamp);
}

void undo() {
	if (E.tree.curr == E.tree.root) return;
	E.urMode = 0;
	urBlock* curr = E.tree.curr;
	if (curr->type == WRITE) {
		E.cx = curr->end[0];
		E.cy = curr->end[1];
		for ( int i = 0; i < curr->length; i++ ) {
			editorDelChar();
		}
	}
	else {
		E.cx = curr->end[0];
		E.cy = curr->end[1];
		if (E.cy == E.numrows) editorInsertRow(E.numrows,"",0);
		for ( int i = curr->length-1; i >= 0; i-- ) {
			if (curr->chars[i] != '\r') {
				editorRowInsertChar(&E.row[E.cy], E.cx, curr->chars[i]); 
				E.cx++;
			}
			else editorInsertNewline();
		}
	}
	for ( int i =  0; i < E.screenrows; i++ ) {
		redrawLine[i] |= REDRAW_DEF;
	}
	E.tree.curr = E.tree.curr->parent;
	E.urMode = 1;
}

void redo() {
	if (E.tree.curr->childlen == 0) return;
	E.urMode = 0;
	int ind = 0;
	for (int i = 1; i < E.tree.curr->childlen; i++) {
		long sec = E.tree.curr->children[ind]->timestamp.tv_sec - E.tree.curr->children[i]->timestamp.tv_sec;
		long nsec = E.tree.curr->children[ind]->timestamp.tv_nsec - E.tree.curr->children[i]->timestamp.tv_nsec;
		sec = sec * 1000 + nsec / 1000000;
		if (sec < 0) ind = i;
	}
	E.tree.curr = E.tree.curr->children[ind];	
	urBlock* curr = E.tree.curr;
	if (curr->type == DELETE) {
		E.cx = curr->start[0];
		E.cy = curr->start[1];
		for ( int i = 0; i < curr->length; i++ ) {
			editorDelChar();
		}
	}
	else {
		E.cx = curr->start[0];
		E.cy = curr->start[1];
		if (E.cy == E.numrows) editorInsertRow(E.numrows,"",0);
		for ( int i = 0; i < curr->length; i++ ) {
			if (curr->chars[i] != '\r') {
				editorRowInsertChar(&E.row[E.cy], E.cx, curr->chars[i]); 
				E.cx++;
			}
			else editorInsertNewline();
		}
	}
	for (int i = 0; i < E.screenrows; i++) {
		redrawLine[i] |= REDRAW_DEF;
	}
	E.urMode = 1;
}

void freeNode(urBlock* node) {
	if (node == NULL) return;
	free(node->chars);
	for (int i = 0; i < node->childlen; i++) freeNode(node->children[i]);
	free(node->children);
	free(node);
}

void freeTree(urTree* tree) {
	if (tree->root == NULL) return;
	freeNode(tree->root);
}

int findDepth(urBlock* node) {
	if (node == NULL) return 0;
	int depth = 0;
	for ( int i = 0; i < node->childlen; i++) {
		int temp = findDepth(node->children[i]);
		depth = (temp > depth) ? temp : depth;
	}
	return depth+1;
}

void findNodesInRow(int* nodeInRow, urBlock* node, int currDepth) {
	nodeInRow[currDepth] += node->childlen;
	for ( int i = 0; i < node->childlen; i++) {
		findNodesInRow(nodeInRow, node->children[i], currDepth+1);
	}
}

void fillCanvas(int rows, int cols, char canvas[rows][cols], urBlock* node, int currDepth, int* hlght_row, int* hlght_col) {
	int ind = 0;
	while (canvas[currDepth][ind] != 0) ind++;
	canvas[currDepth][ind] = '*';
	if (E.tree.curr == node) {*hlght_row = currDepth; *hlght_col = ind;}
	for ( int i = 0; i < node->childlen; i++ ) {
		int parentInd = ind;
		if (canvas[currDepth+1][ind] == 0) canvas[currDepth+1][ind] = '|';
		else {
			ind++;
			while (canvas[currDepth+1][ind] != 0) ind++;
			canvas[currDepth+1][ind] = '\\';
			for ( int j = 1; j < ind-parentInd; j++ ) canvas[currDepth][parentInd+j] = '-';
		}
		fillCanvas(rows, cols, canvas, node->children[i], currDepth+2, hlght_row, hlght_col);
		ind = parentInd;
	}
}

void drawTree() {
	if (E.tree.root == NULL) return;
	int depth = findDepth(E.tree.root);
	int nodesInRow[depth];
	memset(nodesInRow, 0, depth*sizeof(int));
	nodesInRow[0] = 1;
	findNodesInRow(nodesInRow, E.tree.root, 0);
	int max_len = 1;
	for ( int i = 0; i < depth; i++ ) {
		max_len = (nodesInRow[i] > max_len) ? nodesInRow[i] : max_len;
	}
	max_len *= 2;
	char canvas[2*depth-1][max_len+1];
	memset(canvas, 0, (max_len+1)*(2*depth-1));
	int hlght_row;
	int hlght_col;
	fillCanvas(2*depth-1, max_len+1, canvas, E.tree.root, 0, &hlght_row, &hlght_col);
	for ( int i = 1; i < 2*depth-1; i++ ) {
		if (canvas[i][0] == 0) {
			int ind = 0;
			while ( ind < max_len+1 && canvas[i][ind] == 0 ) canvas[i][ind++] = ' ';
		}
	}
	for ( int i = 0; i < depth; i++ ) {
		windowAddRow(canvas[i], E.win.numrows, strlen(canvas[i]));
	}
	E.win.row[hlght_row].hl[hlght_col] = KEYWORD;
}

void treeProcessKey(int c) {
	static int quit_times = JIM_QUIT_TIMES;
	switch(c) {
		case CTRL_KEY('w'):
			clearWindow();
			E.mode = NORMAL;
			break;

		case CTRL_KEY('q'):
			if (E.dirty && quit_times > 0) {
				editorSetStatusMessage("Warning: Unsaved changes. Press Ctrl-Q %d more times to quit.", quit_times);
				quit_times--;
				return;
			}
			write(STDOUT_FILENO, "\x1b[2J", 4); //Clear up screen on exit
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;

		case CTRL_KEY('t'):
		case '\x1b':
			clearWindow();
			E.mode = NORMAL;
			break;

		case ARROW_LEFT:
		case ARROW_RIGHT:
		case ARROW_UP:
		case ARROW_DOWN:
			windowPageScroll(c);
			break;
		default:
			break;
	}
	quit_times = JIM_QUIT_TIMES;
}
