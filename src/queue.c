#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

Queue_UI ui_queue = {.head = NULL, .queue_mutex = PTHREAD_MUTEX_INITIALIZER};

Queue_SEND send_queue = {.head = NULL, .queue_mutex = PTHREAD_MUTEX_INITIALIZER};

int pop_ui(char* text_buf, char* name_buf) {
    pthread_mutex_lock(&ui_queue.queue_mutex);

    if (ui_queue.head == NULL) {
        pthread_mutex_unlock(&ui_queue.queue_mutex);
        return -1;  // Queue empty
    }

    QueueMessage_UI* to_pop = ui_queue.head;
    strcpy(text_buf, to_pop->text);
    strcpy(name_buf, to_pop->name);
    ui_queue.head = to_pop->next;
    free(to_pop);

    pthread_mutex_unlock(&ui_queue.queue_mutex);
    return 0;
}

int push_ui(const char* text_buf, const char* name_buf) {
    pthread_mutex_lock(&ui_queue.queue_mutex);

    if (ui_queue.head == NULL) {
        ui_queue.head = malloc(sizeof(QueueMessage_UI));
        strcpy(ui_queue.head->text, text_buf);
        strcpy(ui_queue.head->name, name_buf);
        ui_queue.head->next = NULL;
    } else {
        int size = 1;
        QueueMessage_UI* current = ui_queue.head;
        while (current->next != NULL) {
            current = current->next;
            size++;
            if (size >= MAX_QUEUE_SIZE) {
                pthread_mutex_unlock(&ui_queue.queue_mutex);
                return -1;  // Queue full
            }
        }
        QueueMessage_UI* new_msg = malloc(sizeof(QueueMessage_UI));
        strcpy(new_msg->text, text_buf);
        strcpy(new_msg->name, name_buf);
        new_msg->next = NULL;
        current->next = new_msg;
    }

    pthread_mutex_unlock(&ui_queue.queue_mutex);
    return 0;
}

int pop_send(uint64_t* dest_addr, char* msg) {
    pthread_mutex_lock(&send_queue.queue_mutex);

    if (send_queue.head == NULL) {
        pthread_mutex_unlock(&send_queue.queue_mutex);
        return -1;  // Queue empty
    }

    QueueMessage_SEND* to_pop = send_queue.head;
    memcpy(msg, to_pop->msg, to_pop->msg_len);
    *dest_addr = to_pop->dest_addr;
    send_queue.head = to_pop->next;
    free(to_pop);

    pthread_mutex_unlock(&send_queue.queue_mutex);
    return 0;
}

int push_send(const uint64_t dest_addr, const char* msg, size_t msg_len) {
    pthread_mutex_lock(&send_queue.queue_mutex);
    if (send_queue.head == NULL) {
        printf("queueDebug1\n");
        send_queue.head = malloc(sizeof(QueueMessage_SEND));
        send_queue.head->dest_addr = dest_addr;
        memcpy(send_queue.head->msg, msg, msg_len);
        send_queue.head->msg_len = msg_len;
        send_queue.head->next = NULL;
    } else {
        printf("queueDebug2\n");
        int size = 1;
        QueueMessage_SEND* current = send_queue.head;
        while (current->next != NULL) {
            current = current->next;
            size++;
            if (size >= MAX_QUEUE_SIZE) {
                pthread_mutex_unlock(&send_queue.queue_mutex);
                return -1;  // Queue full
            }
        }
        QueueMessage_SEND* new_msg = malloc(sizeof(QueueMessage_SEND));
        new_msg->dest_addr = dest_addr;
        memcpy(new_msg->msg, msg, msg_len);
        new_msg->msg_len = msg_len;
        new_msg->next = NULL;
        current->next = new_msg;
    }

    pthread_mutex_unlock(&send_queue.queue_mutex);
    return 0;
}
