#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

#include "protocol_header.h"

#define MAX_QUEUE_SIZE 100

// TCP Connection Zähler
#define SINGLE 1  // Einmalige Nachricht
#define KEEP 2    // Mehrere Nachrichten werden folgen (Serie)
#define UP 3      // Erste Nachricht einer Serie
#define DOWN 4    // Es werden keine Nachrichten mehr folgen
typedef int msg_counter_t;

typedef struct QueueMessage_UI {
    char text[MSG_SIZE - sizeof(Header)];
    char name[32];
    struct QueueMessage_UI* next;
} QueueMessage_UI;

typedef struct QueueMessage_SEND {
    uint64_t dest_addr;
    char msg[MSG_SIZE];
    size_t msg_len;
    msg_counter_t msg_counter;
    struct QueueMessage_SEND* next;
} QueueMessage_SEND;

typedef struct Queue_UI {
    struct QueueMessage_UI* head;
    pthread_mutex_t queue_mutex;
} Queue_UI;

typedef struct Queue_SEND {
    struct QueueMessage_SEND* head;
    pthread_mutex_t queue_mutex;
} Queue_SEND;

int pop_ui(char* text_buf, char* name_buf);

int push_ui(const char* text_buf, const char* name_buf);

int pop_send(msg_counter_t* msg_counter, uint64_t* dest_addr, char* msg);

int push_send(msg_counter_t msg_counter, const uint64_t dest_addr, const char* msg, size_t msg_len);

#endif  // QUEUE_H