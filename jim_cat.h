#ifndef JIM_CAT
#define JIM_CAT
#include <stdio.h>
#include <stdlib.h>
#include "data.h"
#include "window.h"

int jim_cat(const int argc, const char* args[]) {
	if ( argc < 1 ) return -1;
	FILE* fp = fopen(args[0], "r");
	if (!fp) return -1;
	char* line = NULL;
	size_t linesize = 0;
	ssize_t linelen;
	
	while ((linelen = getline(&line, &linesize, fp)) != -1) {
		windowAddRow(line, E.win.numrows, linesize);
		free(line);
	}
	fclose(fp);
	windowSetup(1, 10, 2, NULL, NULL);
	return -2;
}

#endif
