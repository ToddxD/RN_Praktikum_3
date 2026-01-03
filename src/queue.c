#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

Queue ui_queue = {
    .head = NULL,
    .queue_mutex = PTHREAD_MUTEX_INITIALIZER
};

Queue send_queue = {
    .head = NULL,
    .queue_mutex = PTHREAD_MUTEX_INITIALIZER
};


int pop(Queue* queue, char* text_buf, char* name_buf) {
    pthread_mutex_lock(&queue->queue_mutex);

    if (queue->head == NULL) {
        pthread_mutex_unlock(&queue->queue_mutex);
        return -1;  // Queue empty
    }

    QueueMessage* to_pop = queue->head;

    strcpy(text_buf, to_pop->text);
    strcpy(name_buf, to_pop->name);
    queue->head = to_pop->next;
    free(to_pop);

    pthread_mutex_unlock(&queue->queue_mutex);
    return 0;
}

int push(Queue* queue, const char* text, const char* name) {
    pthread_mutex_lock(&queue->queue_mutex);

    if (queue->head == NULL) {
        queue->head = malloc(sizeof(QueueMessage));
        strcpy(queue->head->text, text);
        strcpy(queue->head->name, name);
        queue->head->next = NULL;
    } else {
        int size = 1;
        QueueMessage* current = queue->head;
        while (current->next != NULL) {
            current = current->next;
            size++;
            if (size >= MAX_QUEUE_SIZE) {
                pthread_mutex_unlock(&queue->queue_mutex);
                return -1;  // Queue full
            }
        }
        QueueMessage* new_msg = malloc(sizeof(QueueMessage));
        memcpy(new_msg->text, text, MAX_TEXT);
        memcpy(new_msg->name, name, 32);
        new_msg->next = NULL;
        current->next = new_msg;
    }

    pthread_mutex_unlock(&queue->queue_mutex);
    return 0;
}

int is_queue_empty(Queue* queue) {
    pthread_mutex_lock(&queue->queue_mutex);
    int empty = (queue->head == NULL);
    pthread_mutex_unlock(&queue->queue_mutex);
    return empty;
}