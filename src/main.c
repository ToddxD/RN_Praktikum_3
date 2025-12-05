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

#include "tcp_con.h"

int main(int argc, char** argv) {
    printf("Hello, World!\n");

    int sock = SERVER_listen_on("127.0.0.10", 6970);

    int server = CLIENT_connect_to("127.0.0.10", 6970);

    struct sockaddr_in con_addr;
    socklen_t laenge = sizeof(con_addr);
    // Handshake
    int connection = accept(sock, (struct sockaddr*)&con_addr, &laenge);

    send_tcp(server, "Hello", 5);
    char* buffer = NULL;
    read_tcp(connection, &buffer);
    printf("Received: %s\n", buffer);
    free(buffer);

    close_tcp(server);
    close_tcp(connection);
    close_tcp(sock);

    return 0;
}