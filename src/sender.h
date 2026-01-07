#ifndef SENDER_H
#define SENDER_H

#include <stdint.h>

typedef struct Connection {
    uint64_t addr_port;
    int socket_fd;
    int counter;
    struct Connection* next_item;
} Connection;

void start_sender();

void stop_sender();

#endif // SENDER_H