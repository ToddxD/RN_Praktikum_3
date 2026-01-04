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
#include "tcp_con.h"
#include "queue.h"

static bool running = false;

static Connection* con_head = NULL;

int get_con(uint64_t addr_port) {
    if (con_head == NULL) {  
        con_head = malloc(sizeof(Connection));
        con_head->addr_port = addr_port;
        con_head->socket_fd = CLIENT_connect_to(
            inet_ntoa((struct in_addr){.s_addr = (uint32_t)(addr_port >> 32)}),
            (uint16_t)(addr_port & 0xFFFFFFFF)
        );
        con_head->next_item = NULL;
        return con_head->socket_fd;
    } else {
        Connection* current = con_head;
        if (current->addr_port == addr_port) {
            return current->socket_fd;
        }

        while (current->next_item != NULL) {
            current = current->next_item;
            if (current->addr_port == addr_port) {
                return current->socket_fd;
            }
        } 

        Connection* new_con = malloc(sizeof(Connection));
        new_con->addr_port = addr_port;
        new_con->socket_fd = CLIENT_connect_to(
            inet_ntoa((struct in_addr){.s_addr = (uint32_t)(addr_port >> 32)}),
            (uint16_t)(addr_port & 0xFFFFFFFF)
        );
        new_con->next_item = NULL;
        current->next_item = new_con;
        return new_con->socket_fd;
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
        char msg[sizeof(Header) + MSG_SIZE];
        uint64_t dest_addr;

        if (pop_send(&dest_addr, msg) == 0) {
            printf("Sending message to %lu\n", dest_addr);
            int fd = get_con(dest_addr);
            if(send_tcp(fd, msg, sizeof(Header) + MSG_SIZE)< 0) {
                printf("Error sending message to %lu\n", dest_addr);
            }
            
            // TODO close con
        }
    }

    return NULL;
}

void start_sender() {
    pthread_t thread1;

    pthread_create(&thread1, NULL, sender_loop, NULL);
}

void stop_sender() { running = false; }