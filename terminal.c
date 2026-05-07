#include "data.h"
#include "terminal.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define ASAN_ENABLED
#endif
#elif defined(__SANITIZE_ADDRESS__)
#define ASAN_ENABLED
#endif
#ifdef ASAN_ENABLED
#include <sanitizer/asan_interface.h>
#include <fcntl.h>
#endif

void die(const char *s) {
	disableRawMode();
	write(STDOUT_FILENO, "\x1b[2J", 4); //Clear up screen on error
	write(STDOUT_FILENO, "\x1b[H", 3);
	perror(s);
	fflush(stderr);
	exit(1);
}

void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
		die("tcsetattr");
	}
}

void enableRawMode() {
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");  //Get terminal attributes
	atexit(disableRawMode);

	struct termios raw = E.orig_termios;
	raw.c_iflag &= ~(IXON | ICRNL | ISTRIP | INPCK | BRKINT); //Legacy stuff (plus disabling a few controls)
	raw.c_cflag |= (CS8); //Set char size to 8
	raw.c_oflag &= ~(OPOST); //Removes implicit '\r' from '\n' 
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN); //Turns off echo and some controls
	raw.c_cc[VMIN] = 0; //Sets min number of bytes before read can return
	raw.c_cc[VTIME] = 1; //Sets timeout time for read (1/10 of a second)
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int editorReadKey() {
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN && errno != EINTR) die("read"); //To allow for Cygwin we check for EAGAIN
	}

	if (c == '\x1b') {
		char seq[3];
		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

		if (seq[0] == '[') {
			if (seq[1] >= '0' && seq[1] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
				if (seq[2] == '~') {
					switch (seq[1]) {
						case '1': return HOME_KEY;
						case '3': return DEL_KEY;
						case '4': return END_KEY;
						case '5': return PAGE_UP;
						case '6': return PAGE_DOWN;
						case '7': return HOME_KEY;
						case '8': return END_KEY;
					}
				}
			}
			else {
				switch (seq[1]) {
					case 'A': return ARROW_UP;
					case 'B': return ARROW_DOWN;
					case 'C': return ARROW_RIGHT;
					case 'D': return ARROW_LEFT;
					case 'H': return HOME_KEY;
					case 'F': return END_KEY;
		
				}
			}
		}
		else if (seq[0] == 'O') {
			switch (seq[1]) {
				case 'H': return HOME_KEY;
				case 'F': return END_KEY;
			}
		}

		return '\x1b';
	}
	else {
		return c;
	}
}

int getCursorPosition(int *rows, int* cols) {
	char buf[32];
	unsigned int i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1; //Get escape sequence for window size

	while (i < sizeof(buf) -1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
		if (buf[i] == 'R') break;
		i++;
	}
	buf[i] = '\0';

	if (buf[0] != '\x1b' || buf[1] != '[') return -1;
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

	return 0;
}

int getWindowSize(int* rows, int* cols) {
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1; //Move cursor to the right-most position (C) and move to the bottom-most position (B)
		return getCursorPosition(rows, cols);
	}
	else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

#ifdef ASAN_ENABLED

static int asan_log_fd = -1;

void setupAsanLog() {
	asan_log_fd = open("/tmp/jim_asan.log", O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (asan_log_fd != -1) __sanitizer_set_report_fd((void*)(long)asan_log_fd);
}

void asanDeathCallback() {
	disableRawMode();
	write(STDOUT_FILENO, "\x1b[?1049l", 8);
//	write(STDERR_FILENO, "\r\n", 2);

	if (asan_log_fd != -1) {
		close(asan_log_fd);
		int fd = open("/tmp/jim_asan.log", O_RDONLY);
		if (fd != -1) {
			char buf[4096];
			int n;
			while ((n = read(fd, buf, sizeof(buf))) > 0) write(STDERR_FILENO, buf, n);
			close(fd);
		}
	}
}

#else

void crashHandler(int sig, siginfo_t* info, void* ctx) {
	(void)ctx;
	(void)info;
	disableRawMode();
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
	write(STDOUT_FILENO, "\x1b[?1049l", 8);
	signal(sig, SIG_DFL);
	raise(sig);
}

#endif

void setupCrashHandler() {
#ifdef ASAN_ENABLED
	setupAsanLog();
	__sanitizer_set_death_callback(asanDeathCallback);
#else
	struct sigaction sa;
	sa.sa_sigaction = crashHandler;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);

	sigaction(SIGBUS, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
#endif
}
