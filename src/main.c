#include <stdio.h>

#include "tcp_con.h"

int main(int argc, char** argv) {
    printf("Hello, World!\n");
    CLIENT_connect_to("127.0.0.1", 8080);
    return 0;
}