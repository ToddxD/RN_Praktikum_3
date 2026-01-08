#ifndef UI_H
#define UI_H

#define MAX_TABS 10
#define MAX_MSGS 200
#define MAX_TEXT (1500 - 66)

typedef struct {
    char text[MAX_TEXT];
    int self;
} Message;

typedef struct {
    char user[32];
    Message msgs[MAX_MSGS];
    int msg_count;
    int unread;
} Tab;

int start_ui();

#endif  // UI_H