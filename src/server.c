#include "server.h"

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

#define PORT 6969

#define MAX_EVENTS 20  // maximal 20 Clients gleichzeitig

static bool running = false;

void* thread_loop(void* arg) {
    running = true;

    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    struct hostent* host = gethostbyname(hostname);
    struct in_addr* addr = (struct in_addr*)host->h_addr_list[0];

    int server_socket = SERVER_listen_on(inet_ntoa(*addr), PORT);

    int epoll = epoll_create1(0);
    if (epoll < 0) {
        perror("[Server] error creating epoll");
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_socket;

    if (epoll_ctl(epoll, EPOLL_CTL_ADD, server_socket, &event) < 0) {
        perror("[Server] error adding socket");
    }

    struct epoll_event events[MAX_EVENTS];

    while (running) {
        int event_count = epoll_wait(epoll, events, MAX_EVENTS, -1);

        for (int i = 0; i < event_count; i++) {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) ||
                (!(events[i].events & EPOLLIN))) {
                printf("[Server] epoll error\n");
                // remove_sockaddr(events[i].data.fd);
                close(events[i].data.fd);
            } else if (events[i].data.fd == server_socket) {  // Event kommt vom Socket
                struct sockaddr_in con_addr;
                socklen_t laenge = sizeof(con_addr);
                // Handshake
                int connection = accept(server_socket, (struct sockaddr*)&con_addr, &laenge);
                // add_sockaddr(con_addr, connection);

                if (connection < 0) {
                    perror("[Server] accept error");
                }

                // Die Verbindung (filedescr.) zum Client zusätzlich in die Liste packen
                event.data.fd = connection;
                event.events = EPOLLIN;
                if (epoll_ctl(epoll, EPOLL_CTL_ADD, connection, &event) < 0) {
                    perror("[Server] couldn't add connection");
                }

            } else {  // Wenn das Event von der Verbindung und nicht vom socket kommt, dann kann
                      // gelesen werden
                protocol_handle_msg(events[i].data.fd);
            }
        }
    }

    close_tcp(server_socket);

    return NULL;
}

void start_server() {
    pthread_t thread1;

    pthread_create(&thread1, NULL, thread_loop, NULL);
}

void stop_server() { running = false; }