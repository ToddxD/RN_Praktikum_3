#ifndef UI_H
#define UI_H

#define MAX_TABS 10
#define MAX_MSGS 500
#define MAX_TEXT 256

typedef struct {
    char text[MAX_TEXT];
    int is_own;  // 1 = eigene Nachricht, 0 = fremd
} Message;

typedef struct {
    char name[32];  // Username = Tabname
    Message msgs[MAX_MSGS];
    int msg_count;
    int unread_count;  // Anzahl neuer ungelesener Nachrichten
} ChatTab;

typedef struct {
    ChatTab tabs[MAX_TABS];
    int tab_count;
    int active;  // -1 == kein aktiver Tab
} TabManager;

int start_ui();

#endif  // UI_H