#include "sender.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "protocol.h"
#include "queue.h"
#include "tcp_con.h"

static bool running = false;

static Connection* con_head = NULL;

Connection* get_con(uint64_t addr_port) {
    if (con_head == NULL) {
        con_head = malloc(sizeof(Connection));
        con_head->addr_port = addr_port;
        con_head->socket_fd =
            CLIENT_connect_to(inet_ntoa((struct in_addr){.s_addr = (uint32_t)(addr_port >> 32)}),
                              (uint16_t)(addr_port & 0xFFFFFFFF));
        con_head->counter = 0;
        con_head->next_item = NULL;
        return con_head;
    } else {
        Connection* current = con_head;
        if (current->addr_port == addr_port) {
            return current;
        }

        while (current->next_item != NULL) {
            current = current->next_item;
            if (current->addr_port == addr_port) {
                return current;
            }
        }

        Connection* new_con = malloc(sizeof(Connection));
        new_con->addr_port = addr_port;
        new_con->socket_fd =
            CLIENT_connect_to(inet_ntoa((struct in_addr){.s_addr = (uint32_t)(addr_port >> 32)}),
                              (uint16_t)(addr_port & 0xFFFFFFFF));
        new_con->counter = 0;
        new_con->next_item = NULL;
        current->next_item = new_con;
        return new_con;
    }
}

void remove_con(uint64_t addr_port) {
    Connection* current = con_head;
    Connection* prev = NULL;

    while (current != NULL) {
        if (current->addr_port == addr_port) {
            if (prev == NULL) {
                con_head = current->next_item;
            } else {
                prev->next_item = current->next_item;
            }
            close_tcp(current->socket_fd);
            free(current);
            return;
        }
        prev = current;
        current = current->next_item;
    }
}

void* sender_loop(void* arg) {
    running = true;

    while (running) {
        char msg[MSG_SIZE];
        uint64_t dest_addr;
        msg_counter_t msg_counter;

        int len = pop_send(&msg_counter, &dest_addr, msg);
        if (len > 0) {
            Connection* con = get_con(dest_addr);

            switch (msg_counter) {
                case UP:
                    con->counter++;
                    break;
                case DOWN:
                    con->counter--;
                    break;
                case KEEP:
                case SINGLE:
                    break;
                default:
                    // eigentlich ein Fehler
                    break;
            }

            if (send_tcp(con->socket_fd, msg, len) < 0) {
                printf("Error sending message to %lu\n", dest_addr);
            }

            if (con->counter <= 0) {
                remove_con(dest_addr);
            }
        }
    }

    return NULL;
}

void start_sender() {
    pthread_t thread1;

    pthread_create(&thread1, NULL, sender_loop, NULL);
}

void stop_sender() { running = false; }