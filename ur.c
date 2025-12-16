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
	clock_gettime(CLOCK_MONOTONIC, &E.tree.curr->timestamp);
}

void undo() {
	if (E.tree.curr == E.tree.root) return;
	urBlock* curr = E.tree.curr;
	if (curr->type == WRITE) {
		int x = curr->start[0];
		int y = curr->start[1];
		int length = curr->length;
		while ( length > 0) {
			if (x+length > E.row[y].size) {
				length -= E.row[y].size - x + 1;
				y += 1;
				x = 0;
			}
			else {
				x += length;
				length = 0;
			}
		}

		E.cx = x;
		E.cy = y;
		for (int i = curr->length - 1; i >= 0; i--) {
			editorDelCharUR();
		}
	}
	else {
		int x = curr->start[0];
		int y = curr->start[1];
		int length = curr->length;
		int tempLen = 0;
		int size = 0;
		while ( length > 0) {
			if (x-length < 0) {
				length -= x+1;
				y -= 1;
				tempLen = length+1;
				while (tempLen < curr->length && curr->chars[tempLen] != '\r') {size++; tempLen++;}
				x = size;
				size = 0;
			}
			else {
				x -= length;
				length = 0;
			}
		}
		char text[curr->length+1];
		text[curr->length] = 0;
		for (int i = 0; i < curr->length; i++) text[i] = (curr->chars[i] == '\r') ? '|' : curr->chars[i];
		editorSetStatusMessage("Hex: %02X %02X %02X %02X %02X %02X %02X %02X %02X", text[0], text[1], text[2], text[3], text[4], text[5], text[6], text[7], text[8]);
		E.cx = x;
		E.cy = y;
		for (int i = curr->length - 1; i >= 0; i--) {
			if (E.cy == E.numrows) editorInsertRow(E.numrows,"",0);
			if (curr->chars[i] != '\r') {editorRowInsertChar(&E.row[E.cy], E.cx, curr->chars[i]); E.cx++;}
			else editorInsertNewlineUR();
		}
		for (int i = (y-E.rowoff < 0) ? 0 : y-E.rowoff; i < E.cy+1; i++) {
			redrawLine[i] = (redrawLine[i] == 2 || redrawLine[i] == 3) ? 3 : 1;
		}
	}
	E.tree.curr = E.tree.curr->parent;
}

void redo() {
	if (E.tree.curr->childlen == 0) return;
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
		for (int i = 0; i < curr->length; i++) {
			editorDelCharUR();
		}
	}
	else {
		E.cx = curr->start[0];
		E.cy = curr->start[1];
		for (int i = 0; i < curr->length; i++) {
			if (E.cy == E.numrows) editorInsertRow(E.numrows,"",0);
			if (curr->chars[i] != '\r') {editorRowInsertChar(&E.row[E.cy], E.cx, curr->chars[i]); E.cx++;}
			else editorInsertNewlineUR();
		}
		for (int i = (curr->start[1]-E.rowoff < 0) ? 0 : curr->start[1]-E.rowoff; i < E.cy+1; i++) {
			redrawLine[i] = (redrawLine[i] == 2 || redrawLine[i] == 3) ? 3 : 1;
		}
	}
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

void drawNode(const urBlock* node, const char *prefix, int is_last) {
	if (!node) return;
	char line[256];
	snprintf(line, sizeof(line), "%s%s o(x=%d, y=%d, len=%d)", prefix, is_last ? "`-" : "|-", node->start[0], node->start[1], node->length);
	windowAddRow(line, E.win.numrows, strlen(line));
	char new_prefix[256];
	snprintf(new_prefix, sizeof(new_prefix), "%s%s  ", prefix, is_last ? "   " : "|  ");
	for (int i = 0; i < node->childlen; i++) {
		drawNode(node->children[i], new_prefix, i == node->childlen - 1);
	}
}

void drawTree() {
	if (E.tree.root == NULL) return;
	for (int i = 0; i < E.win.numrows/4; i++) windowAddRow(strdup(" "), i, 1);
	drawNode(E.tree.root, "", 1);
}

void treeProcessKey(int c) {
	static int quit_times = JIM_QUIT_TIMES;
	switch(c) {
		case CTRL_KEY('w'):
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
