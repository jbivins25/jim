#ifndef COMMAND_H
#define COMMAND_H

void editorCommandCallback(char* query, int key);
void editorCommand();
void parseQuery(char* query, char*** args, size_t limit);
int findCommand(char* query);

#endif
