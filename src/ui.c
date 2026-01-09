#define _POSIX_C_SOURCE 200112L
#include "ui.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include "protocol.h"
#include "protocol_header.h"
#include "queue.h"

static Tab tabs[MAX_TABS];
static int tab_count = 0;
static int active_tab = -1;

static struct termios orig_term;
static int rows, cols;

static volatile sig_atomic_t running = 1;
static volatile sig_atomic_t resized = 0;

void handle_sigint(int sig) {
    (void)sig;
    running = 0;
}

/* ---------- Terminal ---------- */

void enable_raw(void) {
    struct termios t;
    tcgetattr(STDIN_FILENO, &orig_term);
    t = orig_term;
    t.c_lflag &= ~(ICANON | ECHO | ISIG);
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void disable_raw(void) { tcsetattr(STDIN_FILENO, TCSANOW, &orig_term); }

void get_size(void) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    rows = w.ws_row;
    cols = w.ws_col;
}

/* ---------- ANSI Helpers ---------- */

void cls(void) { printf("\033[2J\033[H"); }

void move(int r, int c) { printf("\033[%d;%dH", r, c); }

void color(const char* c) { printf("%s", c); }

/* ---------- Tabs ---------- */

int find_tab(const char* user) {
    for (int i = 0; i < tab_count; i++)
        if (strcmp(tabs[i].user, user) == 0) return i;
    return -1;
}

void add_message(const char* user, const char* text, int self) {
    int i = find_tab(user);
    if (i < 0 && tab_count < MAX_TABS) {
        i = tab_count++;
        strncpy(tabs[i].user, user, sizeof(tabs[i].user) - 1);
        tabs[i].msg_count = 0;
        tabs[i].unread = 0;
        if (active_tab < 0) active_tab = i;
    }

    Tab* t = &tabs[i];
    Message* m = &t->msgs[t->msg_count++];
    strncpy(m->text, text, MAX_TEXT - 1);
    m->self = self;

    if (!self && i != active_tab) t->unread = 1;
}

/* ---------- Rendering ---------- */

void draw_tabs(void) {
    move(1, 1);
    for (int i = 0; i < tab_count; i++) {
        if (i == active_tab)
            color("\033[7m");
        else if (tabs[i].unread)
            color("\033[33m");
        else
            color("\033[0m");

        printf(" %s ", tabs[i].user);
        color("\033[0m");
        printf(" ");
    }
}

void draw_messages(void) {
    Tab* t = &tabs[active_tab];
    int line = 3;

    for (int i = 0; i < t->msg_count && line < rows - 2; i++) {
        Message* m = &t->msgs[i];
        int len = strlen(m->text);

        if (m->self) {
            color("\033[32m");
            move(line++, cols - len - 1);
        } else {
            color("\033[34m");
            move(line++, 1);
        }
        printf("%s", m->text);
        color("\033[0m");
    }
}

static void add_message_internal(int tab, const char* text, int self) {
    Tab* t = &tabs[tab];

    if (t->msg_count >= MAX_MSGS) return;

    Message* m = &t->msgs[t->msg_count++];
    strncpy(m->text, text, MAX_TEXT - 1);
    m->text[MAX_TEXT - 1] = 0;
    m->self = self;
}

void add_own_message(const char* text) {
    if (active_tab < 0) return;

    add_message_internal(active_tab, text, 1);
}

void add_foreign_message(const char* user, const char* text) {
    int i = find_tab(user);

    if (i < 0 && tab_count < MAX_TABS) {
        i = tab_count++;
        strncpy(tabs[i].user, user, sizeof(tabs[i].user) - 1);
        tabs[i].user[sizeof(tabs[i].user) - 1] = 0;
        tabs[i].msg_count = 0;
        tabs[i].unread = 0;

        if (active_tab < 0) active_tab = i;
    }

    add_message_internal(i, text, 0);

    if (i != active_tab) tabs[i].unread = 1;
}

void draw_input(const char* buf) {
    move(rows, 1);
    printf("\033[0K> %s", buf);
}

void redraw(const char* input) {
    cls();
    draw_tabs();
    if (active_tab >= 0) draw_messages();
    draw_input(input);
    fflush(stdout);
}

/* ---------- Input ---------- */

int read_key(void) {
    fd_set fds;
    struct timeval tv = {0, 100000};

    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    int r = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);

    if (r > 0) {
        char c;
        if (read(STDIN_FILENO, &c, 1) == 1) return c;
    }

    return -1;
}

/* ---------- Main ---------- */

void cleanup(void) {
    disable_raw();
    printf("\033[0m");    // Farben zurücksetzen
    printf("\033[?25h");  // Cursor anzeigen
    printf("\033[H\n");
    fflush(stdout);

    do_logout();
    sleep(1); // damit alle Nachrichten noch rausgehen
}

int start_ui() {
    char input[MAX_TEXT] = {0};
    int ipos = 0;

    enable_raw();
    atexit(cleanup);

    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    get_size();

    add_foreign_message("System", "Debug-Chat:");

    while (running) {
        redraw(input);
        int k = read_key();
        if (k == 3) {  // Ctrl+C = ASCII ETX
            running = 0;
            break;
        }

        // nach neuen Nachrichten schauen:
        char msg_text[MAX_TEXT];
        char msg_name[32];
        while (pop_ui(msg_text, msg_name) == 0 && running) {
            add_foreign_message(msg_name, msg_text);
        }

        if (k == 27) { /* ESC */
            char seq[2];
            if (read(0, seq, 2) == 2 && seq[0] == '[') {
                if (seq[1] == 'C' && active_tab < tab_count - 1) active_tab++;
                if (seq[1] == 'D' && active_tab > 0) active_tab--;
                tabs[active_tab].unread = 0;
            }
        } else if (k == '\n' && ipos > 0 && active_tab >= 0) {
            add_own_message(input);
            do_chat(tabs[active_tab].user, input);
            ipos = 0;
            input[0] = 0;
        } else if (k == 127 && ipos > 0) {
            input[--ipos] = 0;
        } else if (k > 31 && k < 127 && ipos < MAX_TEXT - 1) {
            input[ipos++] = k;
            input[ipos] = 0;
        }
    }

    return 0;
}
