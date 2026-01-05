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
#include <pthread.h>

#include "server.h"
#include "tcp_con.h"
#include "ui.h"
#include "protocol.h"
#include "routingTable.h"
#include "sender.h"

void* test_thread(void* arg) {
    sleep(2);

    int con = CLIENT_connect_to("127.0.1.1", 6969);

    char read_buf[] = {
        2, 
        1, 
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        'a',
        'b',
        'c',
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        'd',
        'e',
        'f',
        'a',
        'a',
        'a',
        'a',
        'a',
        'a',
        'a',
    };

    while(1) {
        send_tcp(con, read_buf, sizeof(read_buf));
        sleep(5);

        read_buf[31] = 'x';
        read_buf[32] = 'y';
        read_buf[33] = 'z';
    }

    return NULL;
}

static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s -n <name> -p <local_port> -t <host:port>\n"
        "  -n <name>        Chat name (max 32 characters) [required]\n"
        "  -p <port>        Local listening port           [required]\n"
        "  -t <host:port>   Target address and port        [required]\n",
        prog);
    exit(EXIT_FAILURE);
}

void parse_args(int argc, char **argv,
                char *chat_name, size_t name_len,
                int *local_port,
                char *target_host, size_t host_len,
                int *target_port)
{
    int opt;

    while ((opt = getopt(argc, argv, "n:p:t:")) != -1) {
        switch (opt) {

        case 'n':
            if (strlen(optarg) > 32) {
                fprintf(stderr,
                        "Chat name too long (max %d characters)\n",
                        32);
                exit(EXIT_FAILURE);
            }
            strncpy(chat_name, optarg, name_len - 1);
            chat_name[name_len - 1] = '\0';
            break;

        case 'p':
            *local_port = atoi(optarg);
            break;

        case 't': {
            char *colon = strchr(optarg, ':');
            if (!colon) {
                fprintf(stderr,
                        "Invalid target format (expected host:port)\n");
                usage(argv[0]);
            }

            *colon = '\0';
            strncpy(target_host, optarg, host_len - 1);
            target_host[host_len - 1] = '\0';
            *target_port = atoi(colon + 1);
            break;
        }

        default:
            usage(argv[0]);
        }
    }

    /* Pflichtargumente prüfen */
    if (chat_name[0] == '\0' ||*local_port <= 0) {
        fprintf(stderr, "Missing required arguments\n");
        usage(argv[0]);
    }
}

int main(int argc, char** argv) {
    char chat_name[33];
    int  local_port;
    char target_host[256];
    memset(chat_name, 0, sizeof(chat_name));
    memset(target_host, 0, sizeof(target_host));    
    target_host[0] = '\0';
    int  target_port;

    parse_args(argc, argv,
               chat_name, sizeof(chat_name),
               &local_port,
               target_host, sizeof(target_host),
               &target_port);

    strcpy(ownName, chat_name);

    start_sender(); 
    start_server(local_port, chat_name);

    if (target_host[0] != '\0') {
        do_login(chat_name, local_port, target_host, target_port);
    }
    
    //pthread_t thread1;
    //pthread_create(&thread1, NULL, test_thread, NULL);
    
    //start_ui();


    while(1){ }

    return 0;
}