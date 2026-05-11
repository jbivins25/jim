#ifndef JIM_DUMPTREE
#define JIM_DUMPTREE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../data.h"
#include "../jimio.h"

int jim_dumptree(const int argc, const char* args[]) {
	(void)argc;
	(void)args;
	if (E.win.active == 1) clearWindow();
	windowSetup(1, 10, 2, NULL, strdup("Tree Dump"));
	urBlock* temp = E.tree.root;
	char buf[80] = {0};
	int length = 0;
	while (temp != NULL) {
		if (temp->type == NULL_UR) temp = (temp->children == NULL) ? NULL : temp->children[0];
		// urBlock: Type, Start, End, Length, Chars
		if (temp->type == WRITE) {memcpy(buf, "Type: WRITE", 11); length = 11;}
		else {memcpy(buf, "Type: DELETE", 12); length = 12;}
		windowAddRow(buf, E.win.numrows, length);
		length = snprintf(buf, sizeof(buf), "Start: %d, %d", temp->start[0], temp->start[1]);
		windowAddRow(buf, E.win.numrows, length);
		length = snprintf(buf, sizeof(buf), "End: %d, %d", temp->end[0], temp->end[1]);
		windowAddRow(buf, E.win.numrows, length);
		length = snprintf(buf, sizeof(buf), "Length: %d", temp->length);
		windowAddRow(buf, E.win.numrows, length);
		length = snprintf(buf, sizeof(buf), "Chars:");
		windowAddRow(buf, E.win.numrows, length);
		if (temp->length < 80) {
			length = temp->length;
			for ( int i = 0; i < temp->length; i++ ) {
				buf[i] = (temp->chars[i] == '\r') ? '$' : temp->chars[i];
			}
			windowAddRow(buf, E.win.numrows, length);
		}
		temp = (temp->children == NULL) ? NULL : temp->children[0];
	}
	return 0;
}

#endif
