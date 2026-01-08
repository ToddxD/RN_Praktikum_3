#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "heartBeat.h"
#include "protocol.h"
#include "routingTable.h"
#include "sender.h"
#include "server.h"
#include "tcp_con.h"
#include "ui.h"

void* test_thread(void* arg) {
    sleep(2);

    int con = CLIENT_connect_to("127.0.1.1", 6969);

    char read_buf[] = {
        2, 1, 0, 0, 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,   0,   0,   0,   0,   0,   'a', 'b', 'c', 0,   0, 0, 0,
        0, 0, 0, 0, 0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0, 0,
        0, 0, 0, 0, 0, 0, 'd', 'e', 'f', 'a', 'a', 'a', 'a', 'a', 'a', 'a',
    };

    while (1) {
        send_tcp(con, read_buf, sizeof(read_buf));
        sleep(5);

        read_buf[31] = 'x';
        read_buf[32] = 'y';
        read_buf[33] = 'z';
    }

    return NULL;
}

static void parse_host_port(const char* arg, char* host, size_t host_len, int* port,
                            const char* what) {
    char buf[512];
    char* colon;

    strncpy(buf, arg, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    colon = strchr(buf, ':');
    if (!colon) {
        fprintf(stderr, "Invalid %s format (expected host:port)\n", what);
        exit(EXIT_FAILURE);
    }

    *colon = '\0';

    strncpy(host, buf, host_len - 1);
    host[host_len - 1] = '\0';

    *port = atoi(colon + 1);

    if (*port <= 0 || *port > 65535) {
        fprintf(stderr, "Invalid %s port: %d\n", what, *port);
        exit(EXIT_FAILURE);
    }
}

static void usage(const char* prog) {
    fprintf(stderr,
            "Usage: %s -n <name> -l <addr:port> -t <addr:port>\n"
            "  -n <name>        Chat name (max 32 characters) [required]\n"
            "  -l <addr:port>   Local address and port        [required]\n"
            "  -t <addr:port>   Target address and port       [required]\n",
            prog);
    exit(EXIT_FAILURE);
}

void parse_args(int argc, char** argv, char* chat_name, size_t name_len, char* local_host,
                size_t local_len, int* local_port, char* target_host, size_t target_len,
                int* target_port) {
    int opt;

    while ((opt = getopt(argc, argv, "n:l:t:")) != -1) {
        switch (opt) {
            case 'n':
                if (strlen(optarg) > 32) {
                    fprintf(stderr, "Chat name too long (max %d characters)\n", 32);
                    exit(EXIT_FAILURE);
                }
                strncpy(chat_name, optarg, name_len - 1);
                chat_name[name_len - 1] = '\0';
                break;

            case 'l':
                parse_host_port(optarg, local_host, local_len, local_port, "local address");
                break;

            case 't':
                parse_host_port(optarg, target_host, target_len, target_port, "target address");
                break;

            default:
                usage(argv[0]);
        }
    }

    /* Pflichtargumente prüfen */
    if (chat_name[0] == '\0' || local_host[0] == '\0' || *local_port <= 0) {
        fprintf(stderr, "Missing required arguments\n");
        usage(argv[0]);
    }
}

int main(int argc, char** argv) {
    char chat_name[33];

    char local_host[256];
    int local_port;

    char target_host[256];
    int target_port;
    memset(chat_name, 0, sizeof(chat_name));
    memset(target_host, 0, sizeof(target_host));
    memset(local_host, 0, sizeof(local_host));

    target_host[0] = '\0';

    parse_args(argc, argv, chat_name, sizeof(chat_name), local_host, sizeof(local_host),
               &local_port, target_host, sizeof(target_host), &target_port);

    strcpy(ownName, chat_name);

    initTable(chat_name, inet_addr(local_host), local_port);

    start_sender();
    start_server(local_host, local_port, chat_name);

    if (target_host[0] != '\0') {
        do_login(chat_name, local_host, local_port, target_host, target_port);
    }

    startHeartbeatThread();
    start_ui();

    return 0;
}