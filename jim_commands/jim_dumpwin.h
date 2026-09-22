#ifndef JIM_DUMPWIN
#define JIM_DUMPWIN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "data.h"
#include "jimio.h"

int jim_dumpwin(const int argc, const char* args[]) {
	(void)argc;
	(void)args;
	if (E.win.active == 0) return -1;
	FILE* fptr = fopen("dumpwin.txt","w");
	fprintf(fptr,"Numrows: %d\n", E.win.numrows);
	fprintf(fptr,"Contents:\n");
	for (int i = 0; i < E.win.numrows; i++) {
		fprintf(fptr,"%d: ", i);
		for (int j = 0; j < E.win.row[i].rsize; j++) {
			fprintf(fptr,"%c",E.win.row[i].render[j]);
		}
		fprintf(fptr,"\n");
	}
	return 0;
}

#endif
