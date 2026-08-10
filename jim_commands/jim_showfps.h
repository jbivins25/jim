#ifndef JIM_SHOWFPS
#define JIM_SHOWFPS
#include "../data.h"

int jim_showfps(const int argc, const char* args[]) {
        (void)argc;
        (void)args;
	E.showFPS = !E.showFPS;
        return 0;
}

#endif
