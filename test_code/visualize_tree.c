#include <stdio.h>
#include <stdlib.h>

typedef enum {
	WRITE,
	DELETE
} Command;

typedef struct {
	Command type;
	int x;
	int y;
	char* text;
	Node* prev;
	Node** next;
	Node* linNext;
	int nextSize;
} Node;

typedef struct {
	Node* root;
	Node* current;	
} rutree;

Node* makeNode(Command type, int x, int y, char* text, Node* prev) {
	Node* temp = malloc(sizeof(Node);
	temp->type = type;
	temp->x = x;
	temp->y = y;
	temp->text = text;
	temp->prev = prev;
	if (prev != NULL) {
		if (prev->next == NULL) {
			prev->next = malloc(sizeof(Node*));
			prev->next[0] = NULL;
		}
		if (prev->next[prev->nextSize-1] != NULL) {
			prev->nextSize += 1;
			prev->next = realloc(prev->next, sizeof(Node*)*prev->nextSize);	
		}
		prev->next[prev->nextSize-1] = temp;
		prev->linNext = temp;
	}
	temp->nextSize = 1;
	temp->next = NULL;
	temp->linNext = NULL;
	return temp;
}

void freeTree(rutree* Tree) {

}

char* writeText = "a";
char* deleteText = "b";

int main() {
	rutree tree = {NULL, NULL};
	int choice = 0;
	while(choice != 2) {
		scanf("%d", &choice);
		if (choice == 0) {
			if (tree.root == NULL) {
				tree.root = makeNode(WRITE,0,0,writeText,NULL);
			}
			else {

			}
		}
		else {

		}
	}
	return 0;
}
