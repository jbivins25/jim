// ============Commands=============
// Define your command headers here
#include "jim_commands/jim_shell.h"
#include "jim_commands/jim_cat.h"
#include "jim_commands/jim_open.h"
#include "jim_commands/jim_viewsyn.h"
#include "jim_commands/jim_select.h"
// =================================

typedef int (*CommandFunc)(const int argc, const char* args[]); //By default, 0 will be interpretted as success and any negative values are errors which will be printed

typedef struct {
	char* name;
	CommandFunc handler;
} commandEntry;

// ==========Command Table==========
// Add your command entry & update
// the table size

#ifndef _WIN32
#define COM_TAB_SIZE 5
#else
#define COM_TAB_SIZE 0
#endif

commandEntry command_table[] = {
	{"term",jim_shell},
	{"cat",jim_cat},
	{"open",jim_open},
	{"viewsyn",jim_viewsyn},
	{"select",jim_select}
};

// =================================

