// ============Commands=============
// Define your command headers here
#include "jim_shell.h"
#include "jim_cat.h"
#include "jim_open.h"
// =================================

typedef int (*CommandFunc)(const int argc, const char* args[]); //By default, 0 will be interpretted as success and any negative values are errors which will be printed

typedef struct {
	char* name;
	CommandFunc handler;
} commandEntry;

// ==========Command Table==========
// Add your command entry & update
// the table size

#define COM_TAB_SIZE 2

commandEntry command_table[] = {
	{"term",jim_shell},
	{"cat",jim_cat},
	{"open",jim_open}
};

// =================================
