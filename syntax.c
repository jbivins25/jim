#include "syntax.h"
#include "palette.h"
#include "jimio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "terminal.h"
#include "compat.h"

#ifdef _WIN32
#define SYN_PATH "%s\\jim\\jim_%s.syn"
#else
#define SYN_PATH "%s/.jim/jim_%s.syn"
#endif

void loadSyntax(const char* filename, editorSyntax* syn) {
	if (filename == NULL) return;
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
	if (!strcmp(line, "JIMSYN")) return;
	getline(&line, &cap, f);
	sscanf(line, "%hd %hd", &syn->keywordCount, &syn->typeCount);
	syn->keywords = malloc(sizeof(char*)*syn->keywordCount);
	syn->keywordLen = malloc(sizeof(char)*syn->keywordCount);
	char* line_t;
	for ( int i = 0; i < syn->keywordCount; i++ ) {
		len = getline(&line, &cap, f);
		while ((len > 0) && (line[len-1] == '\n' || line[len-1] == '\r')) len--;
		line_t = malloc(len+1);
		for (size_t j = 0; j < len+1; j++) line_t[j] = line[j];
		line_t[len] = '\0';
		syn->keywords[i] = line_t;
		syn->keywordLen[i] = len;
	}
	syn->types = malloc(sizeof(char*)*syn->typeCount);
	syn->typeLen = malloc(sizeof(char)*syn->typeCount);
	for ( int i = 0; i < syn->typeCount; i++ ) {
		len = getline(&line, &cap, f);
		while ((len > 0) && (line[len-1] == '\n' || line[len-1] == '\r')) len--;
		line_t = malloc(len+1);
		for (size_t j = 0; j < len+1; j++) line_t[j] = line[j];
		line_t[len] = '\0';
		syn->types[i] = line_t;
		syn->typeLen[i] = len;
	}
	getline(&line, &cap, f);
	sscanf(line, "%d", &syn->flags);
	if (syn->flags & HL_SL_CM) {
		len = getline(&line, &cap, f);
		while ((len > 0) && (line[len-1] == '\n' || line[len-1] == '\r')) len--;
		line_t = malloc(len+1);
		for (size_t j = 0; j < len+1; j++) line_t[j] = line[j];
		line_t[len] = '\0';
		syn->slComment = line_t;
	}
	if (syn->flags & HL_ML_CM) {
		len = getline(&line, &cap, f);
		while ((len > 0) && (line[len-1] == '\n' || line[len-1] == '\r')) len--;
		line_t = malloc(len+1);
		for (size_t j = 0; j < len+1; j++) line_t[j] = line[j];
		line_t[len] = '\0';
		syn->mlCommentStart = line_t;
		len = getline(&line, &cap, f);
		while ((len > 0) && (line[len-1] == '\n' || line[len-1] == '\r')) len--;
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

static int isSeparator(int c) {
	return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[]{};:", c) != NULL;
}

void editorUpdateSyntax(erow *row, int rowind, editorSyntax* syn, char mode) {
	int changed = 0;
	int numrows = (mode == WINDOW) ? E.win.numrows : E.numrows;
	int offset = (mode == WINDOW) ? E.win.yOffset : E.rowoff;
	int screenrows = (mode == WINDOW) ? E.win.screenrows : E.screenrows;
	do {
		if (changed) { row++; rowind++; }
		changed = 0;
		if (syn->filetype == NULL) return;

		size_t slc_len = syn->slComment ? strlen(syn->slComment) : 0;
		size_t mlcs_len = syn->mlCommentStart ? strlen(syn->mlCommentStart) : 0;
		size_t mlce_len = syn->mlCommentEnd ? strlen(syn->mlCommentEnd) : 0;	
	
		int in_comment = (rowind > 0 && (row-1)->hl_open_comment);
		int in_string = (rowind > 0 && (row-1)->hl_open_string);
	
		for (int i = 0; i < row->size; i++) {
			unsigned char c = (unsigned char)row->chars[i];
	
			if (slc_len && !in_comment && !in_string) {
				if (!strncmp(&row->chars[i],syn->slComment,slc_len)) {
					break;
				}
			}
	
			if (mlcs_len && mlce_len && !in_string) {
				if (in_comment) {

					if (!strncmp(&row->chars[i], syn->mlCommentEnd, mlce_len)) {
						i += mlce_len-1;
						in_comment = 0;
					}
					continue;
				}
				else {
					if (!strncmp(&row->chars[i], syn->mlCommentStart, mlcs_len)) {
						i += mlcs_len-1;
						in_comment = 1;
						continue;
					}
				}
			}	

			if (syn->flags & HL_STRING) {
				if (in_string) {
					if (c == '\\' && i + 1 < row->size) {
						i++;
						continue;
					}
					if (i + 1 == row->size && (!(syn->flags & HL_ML_STRINGS) || c != '\\')) in_string = 0;
					if (c == in_string) {
						in_string = 0;
					}
					continue;
				}
				else {
					if (c == '"' || c == '\'') {
						in_string = c;
						continue;
					}
				}
			}

		}
			row->hl_open_comment = in_comment;
			row->hl_open_string = in_string;
		if (rowind - offset < screenrows && rowind - offset >= 0) markRedrawRow((rowind - offset), (mode == WINDOW) ? REDRAW_WIN : REDRAW_DEF);
	} while (changed && (rowind + 1 < numrows));
}

int editorSyntaxToColor(int hl) {
	if (E.colorful) {
		switch (hl) {
			case COMMENT: return CL_COMMENT;
			case MATCH: return CL_MATCH;
			case TYPE: return CL_TYPE;
			case KEYWORD: return CL_KEYWORD;
			case STRING: return CL_STRING;
			case NUMBER: return CL_NUMBER;
			default: return 97;
		}
	}	
	switch (hl) {
		case COMMENT: return 36;
		case MATCH: return 35;
		case TYPE: return 34;
		case KEYWORD: return 33;
		case STRING: return 32;
		case NUMBER: return 31;
		default: return 97;
	}
}

static int isHex(int c) {
	return strchr("0123456789abcdefABCDEF", c) != NULL;
}

static int isBin(int c) {
	return c == '0' || c == '1';
}

void editorGetSyntax(erow* row, int rowind, unsigned char* buf, int bufsize, editorSyntax* syn) {
	if (bufsize < row->size) die("Invalid buffer size for highlighting");
	int hexNum = 0;
	int binNum = 0;
	memset(buf, NORM, row->size);
	if (syn->filetype == NULL) return;

	size_t slc_len = syn->slComment ? strlen(syn->slComment) : 0;
	size_t mlcs_len = syn->mlCommentStart ? strlen(syn->mlCommentStart) : 0;
	size_t mlce_len = syn->mlCommentEnd ? strlen(syn->mlCommentEnd) : 0;	
	
	int in_comment = (rowind > 0 && (row-1)->hl_open_comment);
	int in_string = (rowind > 0 && (row-1)->hl_open_string);
	int prev_sep = 1;
	
	for (int i = 0; i < row->size; i++) {
		unsigned char c = (unsigned char)row->chars[i];
		unsigned char prev_hl = (i > 0) ? buf[i - 1] : NORM;

		if (slc_len && !in_comment && !in_string) {
			if (!strncmp(&row->chars[i],syn->slComment,slc_len)) {
				memset(&buf[i], COMMENT, row->size - i);
				break;
			}
		}

		if (mlcs_len && mlce_len && !in_string) {
			if (in_comment) {
				buf[i] = COMMENT;
				if (!strncmp(&row->chars[i], syn->mlCommentEnd, mlce_len)) {
					memset(&buf[i], COMMENT, mlce_len);
					i += mlce_len-1;
					in_comment = 0;
					prev_sep = 1;
				}
				continue;
			}
			else {
				if (!strncmp(&row->chars[i], syn->mlCommentStart, mlcs_len)) {
					memset(&buf[i], COMMENT, mlcs_len);
					i += mlcs_len-1;
					in_comment = 1;
					continue;
				}
			}
		}	

		if (syn->flags & HL_STRING) {
			if (in_string) {
				buf[i] = STRING;
				if (c == '\\' && i + 1 < row->size) {
					buf[i+1] = STRING;
					i++;
					continue;
				}
				if (c == in_string) {
					in_string = 0;
				}
				prev_sep = 1;
				continue;
			}
			else {
				if (c == '"' || c == '\'') {
					in_string = c;
					buf[i] = STRING;
					continue;
				}
			}
		}

		if (syn->flags & HL_NUM) {
			if (hexNum) {
				if (!isHex(c)) { hexNum = 0; prev_sep = 0; continue; } 
				buf[i] = NUMBER;
				prev_sep = 0;
				continue;
			}
			if (binNum) {
				if (!isBin(c)) { binNum = 0; prev_sep = 0; continue; }
				buf[i] = NUMBER;
				prev_sep = 0;
				continue;
			}
			if ((isdigit(c) && (prev_sep || prev_hl == NUMBER)) || (c == '.' && prev_hl == NUMBER) || ((c == 'f' ||c == 'b' || c == 'x') && prev_hl == NUMBER) || (isHex(c) && hexNum)) {
				if (c == 'x') hexNum = 1;
				else if (c == 'b') binNum = 1; 
				buf[i] = NUMBER;
				prev_sep = 0;
				continue;
			}
		}
		
		if (prev_sep) {
			int found = 0;
			for (int j = 0; j < syn->keywordCount; j++) {
				int klen = syn->keywordLen[j];
				if (!strncmp(&row->chars[i],syn->keywords[j],klen) && (i+klen <= row->size && isSeparator(row->chars[i+klen]))) {
					memset(&buf[i], KEYWORD, klen);
					found = 1;
					i += klen-1;
					prev_sep = 0;
					break;
				}
			}
			if (found) continue;
			for (int j = 0; j < syn->typeCount; j++) {
				int tlen = syn->typeLen[j];
				if(!strncmp(&row->chars[i],syn->types[j],tlen) && (i+tlen <= row->size && isSeparator(row->chars[i+tlen]))) {
					memset(&buf[i], TYPE, tlen);
					found = 1;
					i += tlen-1;
					prev_sep = 0;
					break;
				}
			}
			if (found) continue;
		}

		if (hexNum && !isHex(c)) hexNum = 0;
		if (binNum && !(c == '0' || c == '1')) binNum = 0;
		prev_sep = isSeparator(c);		
	}
}
