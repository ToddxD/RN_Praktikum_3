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

int main(int argc, char** argv) {
    printf("Hello, World!\n");

    start_server();
    
    pthread_t thread1;
    pthread_create(&thread1, NULL, test_thread, NULL);
    
    start_ui();


    while(1){ }

    return 0;
}