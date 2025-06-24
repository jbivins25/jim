#ifndef DATA_H
#define DATA_H
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <termios.h>
#include <time.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define JIM_VERSION "1.3.1"
#define JIM_TAB_STOP 8
#define JIM_QUIT_TIMES 2 //Functionally you have to hit Ctrl-q three times to quit while the file is dirty

enum editorKey {
	BACKSPACE = 127,
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN
};

enum styleType {
	NORM = 0,
	HGHLT
};

enum modeType {
	NORMAL = 0,
	SELECT,
	WINDOW
};

typedef struct erow {
	int size;
	int rsize;
	char* chars;
	char* render;
} erow;

typedef void (*winHandler) (char c);

typedef struct {
	char active, location;
	int minCols, screencols, screenrows;
	int xOffset, yOffset;
	winHandler handler;
	erow* row;
	int divider;
	int numrows;
	char* header;
} windowConfig;

struct editorConfig {
	int cx, cy;
	int rx; //Render x for handling tabs
	int rowoff;
	int coloff;
	int screenrows;
	int screencols;
	int numrows;
	int selected[4];
	char* cpbuffer;
	erow *row;
	int dirty;
	int mode;
	char *filename;
	char statusmsg[80];
	time_t statusmsg_time;
	struct termios orig_termios;
	windowConfig win;
};

extern struct editorConfig E;

#endif
