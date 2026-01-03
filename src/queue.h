#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

#define MAX_TEXT 256
#define MAX_QUEUE_SIZE 100

typedef struct Queue {
    struct QueueMessage* head;
    pthread_mutex_t queue_mutex;
} Queue;

typedef struct QueueMessage {
    char text[MAX_TEXT];
    char name[32];
    struct QueueMessage* next;
} QueueMessage;

extern Queue ui_queue;
extern Queue send_queue;

int pop(Queue* queue, char* text_buf, char* name_buf);

int push(Queue* queue, const char* text, const char* name);

int is_queue_empty(Queue* queue);

#endif  // QUEUE_H