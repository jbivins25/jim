#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

void get_terminal_size(int *rows, int *cols) {
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    *rows = ws.ws_row;
    *cols = ws.ws_col;
}

void move_cursor(int row, int col) {
    printf("\x1b[%d;%dH", row, col);
}

void hide_cursor() {
    printf("\x1b[?25l");
}

void show_cursor() {
    printf("\x1b[?25h");
}

void clear_screen() {
    printf("\x1b[2J");
}

void draw_window(int start_row, int start_col, int height, int width) {
    // Draw top border
    move_cursor(start_row, start_col);
    printf("+");
    for (int i = 0; i < width - 2; i++) printf("-");
    printf("+");

    // Draw sides with text
    for (int i = 1; i < height - 1; i++) {
        move_cursor(start_row + i, start_col);
        printf("|");
        printf(" Line %2d ", i);
        for (int j = 0; j < width - 11; j++) printf(".");
        printf("|");
    }

    // Draw bottom border
    move_cursor(start_row + height - 1, start_col);
    printf("+");
    for (int i = 0; i < width - 2; i++) printf("-");
    printf("+");
}

int main() {
    int rows, cols;
    get_terminal_size(&rows, &cols);

    // Define window size
    int win_width = cols / 2;
    int win_height = rows - 2;
    int win_start_row = 2;
    int win_start_col = cols - win_width;

    clear_screen();
    hide_cursor();

    draw_window(win_start_row, win_start_col, win_height, win_width);

    move_cursor(rows, 1); // Move cursor to bottom so prompt isn't overwritten
    show_cursor();

    return 0;
}

