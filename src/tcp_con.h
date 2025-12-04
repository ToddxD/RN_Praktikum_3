#ifndef TCP_CON_H
#define TCP_CON_H

#include <unistd.h>

#define BUF_SIZE 1500
#define MAX_READ 5

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/**
 * Creates and connects to a TCP socket.
 *
 * @param ip_str The IP address to connect to.
 * @param port The port to connect to.
 *
 * @return socket file descriptor on success, -1 on error
 */
int CLIENT_connect_to(const char* ip_str, int port);

/**
 * Creates, binds and listens on a TCP socket.
 *
 * @param ip_str The IP address to bind to.
 * @param port The port to bind to.
 *
 * @return socket file descriptor on success, -1 on error
 */
int SERVER_listen_on(const char* ip_str, int port);

/**
 * Sends the given buffer over the TCP socket.
 *
 * @param socket_fd The TCP socket file descriptor to send data to.
 * @param buf The buffer to send.
 * @param size The size of the buffer to send.
 *
 * @return -1 on error
 */
int send_tcp(int socket_fd, char* buf, size_t size);

/**
 * Reads a maxium of BUF_SIZE * MAX_READ bytes from the TCP socket.
 *
 * @param socket_fd The TCP socket file descriptor to read from.
 * @param read_buf return parameter, gets allocated inside the function.
 *
 * @return -1 on error
 */
int read_tcp(int socket_fd, char* read_buf);

/**
 * Closes the given TCP socket.
 *
 * @param socket_fd The TCP socket file descriptor to close.
 *
 * @return -1 on error
 */
int close_tcp(int socket_fd);

#endif  // TCP_CON_H
