#include <string.h>

#include "ncurses.h"

#define MAX_INPUT 256

void add_message(WINDOW* win, const char* user, const char* msg) {
    wattron(win, COLOR_PAIR(2));
    wprintw(win, "[%s] ", user);
    wattroff(win, COLOR_PAIR(2));
    wprintw(win, "%s\n", msg);
    wrefresh(win);
}

int main(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);

    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);

    int h, w;
    getmaxyx(stdscr, h, w);

    WINDOW* chat_frame = newwin(h - 3, w, 0, 0);
    WINDOW* chat_inner = derwin(chat_frame, h - 5, w - 4, 1, 2);
    WINDOW* input = newwin(3, w, h - 3, 0);

    scrollok(chat_inner, TRUE);

    box(chat_frame, 0, 0);
    box(input, 0, 0);
    wrefresh(chat_frame);
    wrefresh(chat_inner);
    wrefresh(input);

    char buf[MAX_INPUT] = {0};
    int pos = 0;

    while (1) {
        werase(input);
        box(input, 0, 0);
        mvwprintw(input, 1, 2, "> %s", buf);
        wmove(input, 1, 4 + pos);
        wrefresh(input);

        int ch = wgetch(input);

        if (ch == '\n') {
            if (pos > 0) add_message(chat_inner, "Hendrik", buf);
            pos = 0;
            buf[0] = '\0';
        } else if (ch == KEY_BACKSPACE || ch == 127) {
            if (pos > 0) buf[--pos] = '\0';
        } else if (ch >= 32 && ch < 127 && pos < MAX_INPUT - 1) {
            buf[pos++] = ch;
            buf[pos] = '\0';
        }
    }

    endwin();
    return 0;
}
