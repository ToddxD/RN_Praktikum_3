#include "ui.h"

#include <ncurses.h>
#include <string.h>

#include "queue.h"

/* ===================== Tab-Logik ===================== */

void init_tabs(TabManager* tm) {
    tm->tab_count = 0;
    tm->active = -1;
}

int find_or_create_tab(TabManager* tm, const char* username) {
    for (int i = 0; i < tm->tab_count; i++) {
        if (strcmp(tm->tabs[i].name, username) == 0) return i;
    }

    if (tm->tab_count >= MAX_TABS) return -1;

    ChatTab* t = &tm->tabs[tm->tab_count];
    strncpy(t->name, username, sizeof(t->name) - 1);
    t->name[sizeof(t->name) - 1] = '\0';
    t->msg_count = 0;

    tm->tab_count++;

    if (tm->active == -1) tm->active = 0;

    return tm->tab_count - 1;
}

void add_own_message(TabManager* tm, const char* text) {
    if (tm->active == -1) return;

    ChatTab* tab = &tm->tabs[tm->active];
    if (tab->msg_count >= MAX_MSGS) return;

    Message* m = &tab->msgs[tab->msg_count++];
    strncpy(m->text, text, sizeof(m->text) - 1);
    m->text[sizeof(m->text) - 1] = '\0';
    m->is_own = 1;
}

void add_foreign_message(TabManager* tm, const char* from_user, const char* text) {
    int idx = find_or_create_tab(tm, from_user);
    if (idx < 0) return;

    ChatTab* tab = &tm->tabs[idx];
    if (tab->msg_count >= MAX_MSGS) return;

    Message* m = &tab->msgs[tab->msg_count++];
    strncpy(m->text, text, sizeof(m->text) - 1);
    m->text[sizeof(m->text) - 1] = '\0';
    m->is_own = 0;

    if (idx != tm->active) {
        tab->unread_count++;
    }
}

void switch_tab(TabManager* tm, int dir) {
    if (tm->tab_count == 0) return;

    tm->active = (tm->active + dir + tm->tab_count) % tm->tab_count;

    tm->tabs[tm->active].unread_count = 0;
}

/* ===================== Rendering ===================== */

void render_tabs(WINDOW *win, const TabManager *tm)
{
    werase(win);

    if (tm->tab_count == 0) {
        wattron(win, A_DIM);
        wprintw(win, " No conversations ");
        wattroff(win, A_DIM);
        wrefresh(win);
        return;
    }

    for (int i = 0; i < tm->tab_count; i++) {

        ChatTab *tab = &tm->tabs[i];

        if (i == tm->active)
            wattron(win, A_REVERSE);

        if (tab->unread_count && i != tm->active)
            wattron(win, COLOR_PAIR(3) | A_BOLD);

        if (tab->unread_count == 0) {
            wprintw(win, " %s ", tab->name);
        } else {
            wprintw(win, " %s (%d) ", tab->name, tab->unread_count);
        }


        if (tab->unread_count && i != tm->active)
            wattroff(win, COLOR_PAIR(3) | A_BOLD);

        if (i == tm->active)
            wattroff(win, A_REVERSE);
    }

    wrefresh(win);
}

static int text_width(const char* s) { return strlen(s); }

void render_chat(WINDOW* inner, const TabManager* tm) {
    werase(inner);

    if (tm->active == -1) {
        wattron(inner, A_DIM);
        mvwprintw(inner, 0, 0, "Waiting for messages...");
        wattroff(inner, A_DIM);
        wrefresh(inner);
        return;
    }

    const ChatTab* tab = &tm->tabs[tm->active];

    int maxy, maxx;
    getmaxyx(inner, maxy, maxx);

    int start = tab->msg_count - maxy;
    if (start < 0) start = 0;

    int row = 0;

    for (int i = start; i < tab->msg_count && row < maxy; i++) {
        const Message* m = &tab->msgs[i];

        int len = strlen(m->text);
        int x;

        if (m->is_own) {
            /* rechtsbündig */
            x = maxx - len - 1;
            if (x < 0) x = 0;

            wattron(inner, COLOR_PAIR(1));
            wmove(inner, row, x);
            waddstr(inner, m->text);
            wattroff(inner, COLOR_PAIR(1));
        } else {
            /* linksbündig */
            wattron(inner, COLOR_PAIR(2));
            wmove(inner, row, 0);
            waddstr(inner, m->text);
            wattroff(inner, COLOR_PAIR(2));
        }

        /* >>> WICHTIG: echte Cursorposition abfragen <<< */
        int new_y, new_x;
        getyx(inner, new_y, new_x);

        row = new_y + 1;

        if (row >= maxy) break;
    }

    wrefresh(inner);
}

/* ===================== Main ===================== */

int start_ui() {
    initscr();
    cbreak();
    noecho();
    curs_set(1);

    set_escdelay(25);

    int h, w;
    getmaxyx(stdscr, h, w);

    WINDOW* tabwin = newwin(1, w, 0, 0);
    WINDOW* chatbox = newwin(h - 4, w, 1, 0);
    WINDOW* chat = derwin(chatbox, h - 6, w - 2, 1, 1);
    WINDOW* inputbox = newwin(3, w, h - 3, 0);

    keypad(inputbox, TRUE);

    box(chatbox, 0, 0);
    box(inputbox, 0, 0);

    nodelay(inputbox, TRUE);

    start_color();
    use_default_colors();

    init_pair(1, COLOR_GREEN, -1);  // eigene Nachrichten
    init_pair(2, COLOR_CYAN, -1);   // fremde Nachrichten
    init_pair(3, COLOR_YELLOW, -1);   // Tab mit neuen Nachrichten

    scrollok(chat, TRUE);

    TabManager tabs;
    init_tabs(&tabs);

    char input[MAX_TEXT] = {0};
    int pos = 0;

    render_tabs(tabwin, &tabs);
    render_chat(chat, &tabs);

    while (1) {
        werase(inputbox);
        box(inputbox, 0, 0);
        mvwprintw(inputbox, 1, 2, "> %s", input);
        wmove(inputbox, 1, 4 + pos);
        wrefresh(inputbox);

        // nach neuen Nachrichten schauen:
        char msg_text[MAX_TEXT];
        char msg_name[32];
        bool new_msg = false;
        while (pop(&ui_queue, msg_text, msg_name) == 0) {
            add_foreign_message(&tabs, msg_name, msg_text);
            new_msg = true;
        }
        if (new_msg) {
            render_tabs(tabwin, &tabs);
            render_chat(chat, &tabs);
        }

        int ch = wgetch(inputbox);

        if (ch == '\n') {
            if (pos > 0) {
                add_own_message(&tabs, input);
                render_tabs(tabwin, &tabs);
                render_chat(chat, &tabs);
            }
            pos = 0;
            input[0] = '\0';
        } else if (ch == KEY_LEFT) {
            switch_tab(&tabs, -1);
            render_tabs(tabwin, &tabs);
            render_chat(chat, &tabs);
        } else if (ch == KEY_RIGHT) {
            switch_tab(&tabs, 1);
            render_tabs(tabwin, &tabs);
            render_chat(chat, &tabs);
        } else if ((ch == KEY_BACKSPACE || ch == 127) && pos > 0) {
            input[--pos] = '\0';
        } else if (ch >= 32 && ch < 127 && pos < MAX_TEXT - 1) {
            input[pos++] = ch;
            input[pos] = '\0';
        }

        if (ch != ERR) {
            render_tabs(tabwin, &tabs);
            render_chat(chat, &tabs);
        }
    }

    endwin();
    return 0;
}
