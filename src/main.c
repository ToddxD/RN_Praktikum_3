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

void* test_thread(void* arg) {
    sleep(2);

    int con = CLIENT_connect_to("127.0.0.1", 6969);

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
        'H',
        'a',
        'l',
        'l',
        'o',
        '?',
    };


    send_tcp(con, read_buf, sizeof(read_buf));

    return NULL;
}

int main(int argc, char** argv) {
    printf("Hello, World!\n");

    start_server();

    pthread_t thread1;
    pthread_create(&thread1, NULL, test_thread, NULL);



    while(1){ }

    return 0;
}