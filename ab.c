#include <stdlib.h>
#include <string.h>
#include "ab.h"

void abAppend(struct abuf *ab, const char *s, int len) {
	if (ab->len + len > ab->capacity) {
		do { ab->capacity += 128; } while (ab->len + len > ab->capacity);
		ab->b = realloc(ab->b, ab->capacity); //Dynamic memory buffer
	}

	memcpy(&ab->b[ab->len], s, len); 
	ab->len += len;
}

void abFree(struct abuf *ab) {	
	free(ab->b);
	ab->len = 0;
	ab->capacity = 0;
}

void abClear(struct abuf* ab) {
	ab->len = 0;
}
