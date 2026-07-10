#include "data.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "compat.h"
#include "fileio.h"
#include "jimio.h"
#include "row.h"

char* editorRowsToString(int* buflen) {
	int totlen = 0;
	int j;
	for (j = 0; j < E.numrows; j++) totlen += E.row[j].size + 1; //Length + newline
	*buflen = totlen;

	char* buf = malloc(totlen);
	char *p = buf;
	for (j = 0; j < E.numrows; j++) {
		memcpy(p, E.row[j].chars, E.row[j].size);
		p += E.row[j].size;
		*p = '\n';
		p++;
	}

	return buf;
}

void editorOpen(char* filename) {
	free(E.filename);
	E.filename = strdup(filename);
	FILE *fp = fopen(filename, "r");
	if (!fp) return;

	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	while ((linelen = getline(&line, &linecap, fp)) != -1) {
		while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) linelen--;
		editorInsertRow(E.numrows,line,linelen);
	}
	E.dirty = 0;
	free(line);
	fclose(fp);
}

void editorSave() {
	if (E.filename == NULL) {
		E.filename = editorPrompt("Save as: %s", NULL);
		if (E.filename == NULL) {
			editorSetStatusMessage("Save aborted");
			return;
		}
	}

	int len;
	char* buf = editorRowsToString(&len);

	int fd = open(E.filename, O_RDWR | O_CREAT, 0644); //Create or open file for read/write and use permissions 0664
	if (fd != -1) {
		if (ftruncate(fd, len) != -1) { //Sets file size to specific length
			if (write(fd,buf,len) == len) { //Will hopefully not overwrite all the data if write fails by using ftruncate
				close(fd);
				free(buf);
				E.dirty = 0;
				editorSetStatusMessage("Saved! Wrote %d bytes to disk", len);
				return;
			}
		}
		close(fd);
	}
	free(buf);
	editorSetStatusMessage("Can't save! Error: %s", strerror(errno));
}
