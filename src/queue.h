#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include "protocol_header.h"

#define MAX_QUEUE_SIZE 100

typedef struct QueueMessage_UI {    
    char text[MSG_SIZE];
    char name[32];
    struct QueueMessage_UI* next;
} QueueMessage_UI;

typedef struct QueueMessage_SEND {
    uint64_t dest_addr;
    char msg[sizeof(Header) + MSG_SIZE];
    size_t msg_len;
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

int pop_send(uint64_t* dest_addr, char* msg);

int push_send(const uint64_t dest_addr, const char* msg, size_t msg_len);

#endif  // QUEUE_H