#include "tcp_con.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int CLIENT_connect_to(const char* ip_str, int port) {
    struct sockaddr_in socket_addr;
    memset(&socket_addr, 0, sizeof(socket_addr));
    socket_addr.sin_family = AF_INET;
    socket_addr.sin_port = htons(port);
    socket_addr.sin_addr.s_addr = inet_addr(ip_str);

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(sock, (struct sockaddr*)&socket_addr, sizeof(socket_addr)) < 0) {
        fprintf(stderr, "[TCP] error connecting to %s:%d: %s\n", ip_str, port,strerror(errno));
        return -1;
    }

    return sock;
}

int SERVER_listen_on(const char* ip_str, int port) {
    struct sockaddr_in socket_addr;
    memset(&socket_addr, 0, sizeof(socket_addr));
    socket_addr.sin_family = AF_INET;
    socket_addr.sin_port = htons(port);
    socket_addr.sin_addr.s_addr = inet_addr(ip_str);

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    int one = 1;
    const int* val = &one;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, val, sizeof(one)) < 0) {
        fprintf(stderr, "[TCP] error setsockopt SO_REUSEADDR for%s:%d: %s\n", ip_str, port, strerror(errno));
        return -1;
    }

    socklen_t addr_len = sizeof(socket_addr);
    if (bind(sock, (struct sockaddr*)&socket_addr, addr_len) < 0) {
        fprintf(stderr, "[TCP] error binding %s:%d: %s\n", ip_str, port, strerror(errno));
        return -1;
    }

    if (listen(sock, 100) < 0) {
        fprintf(stderr, "[TCP] error listening %s:%d: %s\n", ip_str, port, strerror(errno));
        return -1;
    }

    return sock;
}

int send_tcp(int socket_fd, char* str, size_t size) {
    size_t total_sent = 0;
    size += sizeof(EOT);
    char* buf = malloc(size);
    strcpy(buf, str);
    strcat(buf, EOT);

    while (total_sent < size) {
        size_t n = write(socket_fd, buf + total_sent, MIN(BUF_SIZE, size - total_sent));
        if (n < 0) {
            fprintf(stderr, "[TCP] error sending file data: %s\n", strerror(errno));
            free(buf);
            return -1;
        }
        total_sent += n;
    }

    return 0;
}

int read_tcp(int socket_fd, char** read_buf) {
    *read_buf = malloc(BUF_SIZE * sizeof(char));

    for (int i = 0; i < MAX_READ; i++) {
        if (i > 0) {
            *read_buf = realloc(*read_buf, BUF_SIZE * (i + 1) * sizeof(char));
        }

        if (read(socket_fd, *read_buf + (i * BUF_SIZE), BUF_SIZE) < 0) {
            fprintf(stderr, "[TCP] error reading from socket: %s\n", strerror(errno));
            free(read_buf);
            read_buf = NULL;
            return -1;
        }

        char* eot = memchr(*read_buf, '\004', BUF_SIZE);  // EOT finden und abbrechen
        if (eot) {
            break;
        }
    }
    return 0;
}

int close_tcp(int socket_fd) {
    if (close(socket_fd) < 0) {
        fprintf(stderr, "[TCP] error closing socket: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}