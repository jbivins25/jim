#ifndef JIM_OPEN
#define JIM_OPEN
#include <stdio.h>
#include <stdlib.h>

int jim_open(const int argc, const char* args[]) {
	if ( argc < 1 ) return -1;
	const char* filename = args[0];
	FILE* fp = fopen(filename, "r");
	if (!fp) return -1;
	char* line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	while ((linelen = getline(&line, &linecap, fp)) != -1) {
		while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) linelen--;
	}
	free(line);
	fclose(fp);
	return 0;
}

#endif
