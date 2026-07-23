#ifndef COMPAT_H
#define COMPAT_H

#ifdef _WIN32

#include <stdio.h>
#include <io.h>
#include <windows.h>
#define write _write
#define open _open
#define close _close
#define ftruncate _chsize
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
static inline ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
	if (lineptr == NULL || n == NULL || stream == NULL) return -1;

	if (*lineptr == NULL || *n == 0) {
		*n = 128;
		*lineptr = (char *)malloc(*n);
		if (*lineptr == NULL) return -1;
	}

	long int count = 0;
	int ch;

	while ((ch = fgetc(stream)) != EOF) {
		if ((size_t)count + 1 >= *n) {
			size_t new_size = *n * 2;
			char *new_ptr = (char *)realloc(*lineptr, new_size);
			if (new_ptr == NULL) return -1;
			*lineptr = new_ptr;
			*n = new_size;
		}
		(*lineptr)[count++] = ch;
		if (ch == '\n') break;
	}

	if (count == 0 && ch == EOF) return -1;
	(*lineptr)[count] = '\0';
	return count;
}

#else

#include <unistd.h>

#endif

#endif
