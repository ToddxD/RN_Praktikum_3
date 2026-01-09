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

void remove_con_fd(uint64_t fd);

void remove_con(uint64_t addr_port);

#endif  // SENDER_H