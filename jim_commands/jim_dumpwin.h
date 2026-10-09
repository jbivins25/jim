#ifndef JIM_DUMPWIN
#define JIM_DUMPWIN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "data.h"
#include "syntax.h"
#include "jimio.h"

int jim_dumpwin(const int argc, const char* args[]) {
	(void)argc;
	(void)args;
	if (E.win.active == 0) return -1;
	FILE* fptr = fopen("dumpwin.txt","w");
	fprintf(fptr,"Numrows: %d\n", E.win.numrows);
	fprintf(fptr,"Contents:\n");
	int rsize = 0;
	int temp;
	char* render = NULL;
	for (int i = 0; i < E.win.numrows; i++) {
		temp = editorGetRenderSize(&E.win.row[i], 
		if (rsize < temp) render = realloc(render, temp+1);
		rsize = temp;
		editorGetRender(&E.win.row[i], render, rsize+1);
		fprintf(fptr,"%d: ", i);
		for (int j = 0; j < rsize; j++) {
			fprintf(fptr,"%c",render[j]);
		}
		fprintf(fptr,"\n");
	}
	free(render);
	return 0;
}

#endif
