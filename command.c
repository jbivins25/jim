#include "data.h"
#include "config.h"
#include "command.h"
#include "row.h"
#include "jimio.h"
#include <string.h>
#include <stdlib.h>

void editorCommandCallback(char* query, int key) {
	size_t limit = 10;
	char** args = malloc(sizeof(char*)*limit);
	int ret_val;

	if (key == '\x1b') {
		return;
	}
	else if (key == '\r') {
		parseQuery(query, &args, limit);
		int com_ind = findCommand(query);
		if (com_ind < 0) {
			return;
		}
		int argc = 0;
		while (args[argc] != NULL) argc++;
		ret_val = command_table[com_ind].handler(argc, (const char**) args);
		if (ret_val < 0) editorSetStatusMessage("Error: %d",ret_val);
	}
	free(args);
}

void parseQuery(char* query, char*** args, size_t limit) {
	char* temp = query;
	size_t arg_counter = 0;
	temp++;
	while (*temp != '\0') {
		if (*temp == ' ') {
			*temp = 0;
			(*args)[arg_counter++] = temp + 1;
			if (arg_counter == limit) {
				limit += 10;
				*args = realloc(*args, limit*(sizeof(char*)));
			}
		}
		temp++;
	}
}

int findCommand(char* query) {
	for (size_t i = 0; i < COM_TAB_SIZE; i++) {
		if (strcmp(query, command_table[i].name) == 0) return i;
	}
	return -1;
}

void editorCommand() {
	char* query = editorPrompt("Command: %s", editorCommandCallback);
	if (query) free(query);
}
