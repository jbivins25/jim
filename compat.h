#ifndef COMPAT_H
#define COMPAT_H

#ifdef _WIN32

#include <io.h>
#include <windows.h>
#define write _write
#define open _open
#define close _close
#define ftruncate _chsize
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
ssize_t getline(char **lineptr, size_t *n, FILE *stream);

#else

#include <unistd.h>

#endif

#endif
