#ifndef JIM_SELECT
#define JIM_SELECT
#include "../data.h"
#include "../editor.h"
#include <string.h>
#include <stdlib.h>

int jim_select(const int argc, const char* args[]) {
	if ( argc < 2 ) return -1;
	if ( strlen(args[0]) > 63 ) return -2; //Arg 1 is greater than the buffer
	if ( strlen(args[1]) > 63 ) return -3; //Arg 2 is greater than the buffer

	E.mode = SELECT;
	char buf[64];
	strcpy(buf, args[0]);
	const char delim[] = ":";
	char *token = strtok(buf, delim);
	E.selected[0] = atoi(token)-1;
	if (E.selected[0] < 0 || E.selected[0] > E.numrows-1) {
		exitSelect();
		return -4; //Values out of range for start
	}
	token = strtok(NULL, delim);
	if (token == NULL) E.selected[2] = 0;
	else {
		E.selected[2] = atoi(token)-1;
		if (E.selected[2] > E.row[E.selected[0]].size-1) E.selected[2] = E.row[E.selected[0]].size-1;
		else if (E.selected[2] < 0) E.selected[2] = 0;
	}
	strcpy(buf, args[1]);
	token = strtok(buf, delim);
	E.selected[1] = atoi(token)-1;
	if (E.selected[1] > E.numrows-1 || E.selected[1] < 0 || E.selected[1] < E.selected[0]) {
		exitSelect();
		return -5; //Values out of range for end
	}
	token = strtok(NULL, delim);
	if (token == NULL) E.selected[3] = 0;
	else {
		E.selected[3] = atoi(token)-1;
		if (E.selected[3] > E.row[E.selected[1]].size-1) E.selected[3] = E.row[E.selected[1]].size-1;
		else if (E.selected[3] < 0) E.selected[3] = 0;
	}
	E.cy = E.selected[1];
	E.cx = E.selected[3];
	redrawWholeScreen = 1;
	return 0;
}



#endif
