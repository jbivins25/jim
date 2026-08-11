#ifndef DATA_H
#define DATA_H
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#ifndef _WIN32
#include <termios.h>
#include <pthread.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <time.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define JIM_VERSION "1.9.3"
#define JIM_TAB_STOP 8
#define JIM_QUIT_TIMES 2 //Functionally you have to hit Ctrl-q three times to quit while the file is dirty
#define SCREEN_ROW_MAX 256
#define UNDO_TIMEOUT 500
#define STARTING_CAPACITY 16
#define MAX_THREADS 4

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
	void (*keypressCallback)();
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
	char showFPS;
};

//Editor Thread Definitions
#ifndef _WIN32
typedef pthread_t editor_thread_t;
typedef pthread_mutex_t editor_mutex_t;
#define CREATE_LOCK(m) pthread_mutex_init(&(m), NULL) //Abstraction has default attributes
#define DELETE_LOCK(m) pthread_mutex_destroy(&(m))
#define THREAD_LOCK(m) pthread_mutex_lock(&(m))
#define THREAD_UNLOCK(m) pthread_mutex_unlock(&(m))
#define THREAD_TRY_UNLOCK(m) pthread_mutex_trylock(&(m))
#else
typedef HANDLE editor_thread_t;
typedef CRITICAL_SECTION editor_mutex_t;
#define CREATE_LOCK(m) InitializeCriticalSection(&(m))
#define DELETE_LOCK(m) DeleteCriticalSection(&(m))
#define THREAD_LOCK(m) EnterCriticalSection(&(m))
#define THREAD_UNLOCK(m) LeaveCriticalSection(&(m))
#define THREAD_TRY_UNLOCK(m) !TryEnterCriticalSection(&(m)) //Returns 1 if it fails
#endif
typedef void (*editorThreadFunc)(void *);

enum threadState {
	THREAD_UNUSED,
	THREAD_ACTIVE,
	THREAD_FINISHED
};

typedef struct {
	editor_thread_t handle;
	int state;

	editorThreadFunc func;
	void* arg;
} editorThread;

typedef struct {
	editorThread slot[MAX_THREADS];
	int count;

	editor_mutex_t threadLock;
	editor_mutex_t redrawLock;
	editor_mutex_t windowThreadLock;
	editor_mutex_t setMessageLock;
} editorThreads;

int editorThreadCreate(editorThreadFunc func, void* arg);
int editorDetachThread(editorThread* t);
int editorJoinThread();

extern struct editorConfig E;
extern editorThreads T;
extern int CLEAN_WIN;
extern char redrawLine[SCREEN_ROW_MAX];
extern int redrawWholeScreen;
#ifndef _WIN32
extern int eventPipe[2];
#else
extern HANDLE eReadPipe, eWritePipe;
extern HANDLE hStdin, pipeEvent;
extern HANDLE hEvents[2];
#endif

#endif
