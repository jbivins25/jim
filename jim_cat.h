#ifndef JIM_CAT
#define JIM_CAT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data.h"
#include "window.h"

int jim_cat(const int argc, const char* args[]) {
	if ( argc < 1 ) return -1;
	FILE* fp = fopen(args[0], "r");
	if (!fp) return -1;
	windowSetup(1, 10, 2, NULL, strdup(args[0]));
	char* line = NULL;
	size_t linecap = 0;
	ssize_t linelen;	
	while ((linelen = getline(&line, &linecap, fp)) != -1) {
		while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) linelen--;
		windowAddRow(line, E.win.numrows, linelen);
	}
	free(line);
	fclose(fp);
	return 0;
}

#endif
