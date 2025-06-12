#define _GNU_SOURCE
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <string.h>
#include <assert.h>

static void sig_handler(int sig) {
	if (SIGWINCH == sig) {
		struct winsize winsz;

		ioctl(0, TIOCGWINSZ, &winsz);
		printf("SIGWINCH raised window size: %d rows / %d cols\n", winsz.ws_row, winsz.ws_col);
	}
}

int main(void) {
	signal(SIGWINCH, sig_handler);

	for ( int i = 0; i < 100; i++ ) { sleep(1); pause; }
	return 0;
}
