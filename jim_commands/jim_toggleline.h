#ifndef JIM_TOGGLELINE
#define JIM_TOGGLELINE
#include <string.h>
#include "compat.h"
#include "data.h"
#include "jimio.h"

int jim_toggleline(const int argc, const char* args[]) {
	if (argc == 0) {
		E.linenum = !E.linenum;
	}
	else {
		if (!strcmp(args[0],"-r") || !strcmp(args[0],"-relative")) {
			E.relative = !E.relative;
			if (!E.linenum) E.linenum = 1;
		}
	}
	markRedrawAll();
	return 0;
}

#endif
