#ifndef SERVER_H
#define SERVER_H

void start_server(char* _local_address, int _port, char* chat_name);

void stop_server();

extern int epoll;

#endif  // SERVER_H