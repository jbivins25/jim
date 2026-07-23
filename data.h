#ifndef DATA_H
#define DATA_H
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#ifndef _WIN32
#include <termios.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <time.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define JIM_VERSION "1.5.4"
#define JIM_TAB_STOP 8
#define JIM_QUIT_TIMES 2 //Functionally you have to hit Ctrl-q three times to quit while the file is dirty
#define SCREEN_ROW_MAX 256
#define UNDO_TIMEOUT 500
#define STARTING_CAPACITY 16

//====================================
// Syntax Flags
#define HGHLT_NUM (1 << 0)
#define HGHLT_STRING (1 << 1)
#define HGHLT_SL_CM (1 << 2)
#define HGHLT_ML_CM (1 << 3)
#define HGHLT_ML_STRINGS (1 << 4)
//====================================

//====================================
// Redraw flags
#define REDRAW_DEF (1 << 0)
#define REDRAW_WIN (1 << 1)
//====================================

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
	HGHLT,
	COMMENT,
	STRING,
	NUMBER,
	KEYWORD,
	TYPE,
	MATCH
};

enum modeType {
	NORMAL = 0,
	SELECT,
	WINDOW
};

enum urType {
	WRITE = 0,
	DELETE_UR,
	NULL_UR
};

typedef struct erow {
	int ind;
	int size;
	int rsize;
	char* chars;
	char* render;
	unsigned char* hl;
	int hl_open_comment;
	int hl_open_string;
} erow;

typedef void (*winHandler) (int c);

typedef struct {
	char* filetype;
	char** keywords;
	char* keywordLen;
	char** types;
	char* slComment;
	char* mlCommentStart;
	char* mlCommentEnd;
	int keywordCount;
	int typeCount;
	int flags;
} editorSyntax;

typedef struct {
	char active, location;
	int minCols, screencols, screenrows;
	int xOffset, yOffset;
	winHandler handler;
	erow* row;
	editorSyntax syn;
	int divider;
	int numrows;
	char* header;
} windowConfig;

typedef struct urBlock {
	char type;
	int start[2];
	int end[2];
	int length;
	int childlen;
	char* chars;
	struct urBlock* parent;
	struct urBlock** children;
	struct timespec timestamp;
} urBlock;

typedef struct {
	urBlock* root;
	urBlock* curr;
} urTree;

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
	erow* row;
	int dirty;
	int mode;
	char *filename;
	char statusmsg[80];
	time_t statusmsg_time;
	#ifndef _WIN32
	struct termios orig_termios;
	#else
	DWORD orig_termios;
	#endif
	windowConfig win;
	editorSyntax syn;
	urTree tree;
	int colorful;
	int sticky;
	int capacity;
	char urType;
	char urMode;
};

extern struct editorConfig E;
extern char redrawLine[SCREEN_ROW_MAX];
extern int redrawWholeScreen;

#endif
