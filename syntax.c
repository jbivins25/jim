#include "syntax.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "terminal.h"
#include "compat.h"

#ifdef _WIN32
#define SYN_PATH "%s\\jim\\jim_%s.syn"
#else
#define SYN_PATH "%s/.jim/jim_%s.syn"
#endif

void loadSyntax(const char* filename, editorSyntax* syn) {
	char* ext = strrchr(filename, '.');
	if (!ext) return;
	size_t len = strlen(++ext);
	if (len < 1) return;
	#ifndef _WIN32
	const char *home = getenv("HOME");
	#else
	const char *home = getenv("APPDATA");
	#endif
	if (!home) die("Couldn't find home");
	char file[512];
	snprintf(file, len+16+strlen(home), SYN_PATH, home, ext);
	FILE* f = fopen(file,"r");
	if (f == NULL) return;
	syn->filetype = malloc(len+1);
	strcpy(syn->filetype,ext);
	char* line = NULL;
	size_t cap = 0;
	getline(&line, &cap, f);
	sscanf(line, "%d %d", &syn->keywordCount, &syn->typeCount);
	syn->keywords = malloc(sizeof(char*)*syn->keywordCount);
	syn->keywordLen = malloc(sizeof(char)*syn->keywordCount);
	char* line_t;
	for ( int i = 0; i < syn->keywordCount; i++ ) {
		len = getline(&line, &cap, f);
		while (line[len-1] == '\n' || line[len-1] == '\r') len--;
		line_t = malloc(len+1);
		for (size_t j = 0; j < len+1; j++) line_t[j] = line[j];
		line_t[len] = '\0';
		syn->keywords[i] = line_t;
		syn->keywordLen[i] = len;
	}
	syn->types = malloc(sizeof(char*)*syn->typeCount);
	for ( int i = 0; i < syn->typeCount; i++ ) {
		len = getline(&line, &cap, f);
		while (line[len-1] == '\n' || line[len-1] == '\r') len--;
		line_t = malloc(len+1);
		for (size_t j = 0; j < len+1; j++) line_t[j] = line[j];
		line_t[len] = '\0';
		syn->types[i] = line_t;
	}
	getline(&line, &cap, f);
	sscanf(line, "%d", &syn->flags);
	if (syn->flags & HGHLT_SL_CM) {
		len = getline(&line, &cap, f);
		while (line[len-1] == '\n' || line[len-1] == '\r') len--;
		line_t = malloc(len+1);
		for (size_t j = 0; j < len+1; j++) line_t[j] = line[j];
		line_t[len] = '\0';
		syn->slComment = line_t;
	}
	if (syn->flags & HGHLT_ML_CM) {
		len = getline(&line, &cap, f);
		while (line[len-1] == '\n' || line[len-1] == '\r') len--;
		line_t = malloc(len+1);
		for (size_t j = 0; j < len+1; j++) line_t[j] = line[j];
		line_t[len] = '\0';
		syn->mlCommentStart = line_t;
		len = getline(&line, &cap, f);
		while (line[len-1] == '\n' || line[len-1] == '\r') len--;
		line_t = malloc(len+1);
		for (size_t j = 0; j < len+1; j++) line_t[j] = line[j];
		line_t[len] = '\0';
		syn->mlCommentEnd = line_t;
	}
	free(line);
	fclose(f);
}

void freeSyntax(editorSyntax* syn) {
	free(syn->filetype);
	for (int i = 0; i < syn->keywordCount; i++) {
		free(syn->keywords[i]);
	}
	free(syn->keywords);
	for (int i = 0; i < syn->typeCount; i++) {
		free(syn->types[i]);
	}
	free(syn->types);
	free(syn->slComment);
	free(syn->mlCommentStart);
	free(syn->mlCommentEnd);
}
