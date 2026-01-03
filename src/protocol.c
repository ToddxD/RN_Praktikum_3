#include "protocol.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
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

#include "tcp_con.h"
#include "queue.h"

#define NAME_LEN 32

#define TYPE_LOGIN 1
#define TYPE_CHAT 2
#define TYPE_LOGOUT 3
#define TYPE_ROUTE 4
#define TYPE_HEART 5
#define TYPE_HEARTRESPONSE 6
#define TYPE_ERROR 7

typedef struct __attribute__((packed)) {
    u_int8_t type;
    u_int8_t version;
    char sendername[NAME_LEN];
    char targetname[NAME_LEN];
} Header;

void getName(char* dest, char* src) {
    int count = NAME_LEN;
    while (*src == 0) {
        src++;
        count--;
    }
    memcpy(dest, src, count);
    dest[NAME_LEN] = 0;
}

void forward(const char* target, const char* msg) { 
	printf("Forwarding message to %s\n", target); 
 	// TODO \004 wahrscheinlich noch an msg anhängen
	// target in routing tabelle suchen
	// gesamte Nachricht (msg) an target aus routing tabelle senden
	// keine Heart und heartresponse weiterleiten (sollte hier gar nicht ankommen)
}

void msg_login(const char* sender, const char* content) { 
	printf("Handling login message\n"); 
	// sender zur routing tabelle hinzufügen
	// aktualisierte ROUTE message versenden
}

void msg_chat(const char* sender, const char* content) { 
	//printf("Chat from %s: %s\n", sender, content); 
	// auf UI anzeigen
    push(&ui_queue, content, sender);
}

void msg_logout(const char* sender) {
	printf("Handling logout message\n"); 
	// sender aus routing tabelle entfernen (disconnect)
}

void msg_route(const char* sender, const char* content) {
	printf("Handling route message\n");
	// content zu routing tabelle parsen
	// hop counts erhöhen
	// eigene routing tabelle mit neuen Daten aktualisieren
	// aktualisierte ROUTE message versenden
}

void msg_heart(const char* sender) {
	printf("Handling heart message\n"); 
	// HEARTRESPONSE an sender senden
}

void msg_heartresponse(const char* sender) {
	printf("Handling heart response message\n"); 
	// TODO vllt in einem anderen thread?
}

void msg_error() { printf("Handling error message\n"); }

void protocol_handle_msg(const int connection) {
    char* read_buf;
	int count = read_tcp(connection, &read_buf);
    if (count < 0) {
        if (errno != EAGAIN) {
            close(connection);
    		free(read_buf);
			return;
        }
    } 
	
	if (count < sizeof(Header)) {
        printf("[Server] Message too short\n");
		free(read_buf);
		return;
    }

    Header header;
    memcpy(&header, read_buf, sizeof(Header));

	char sender[NAME_LEN + 1] = {};
    getName(sender, header.sendername);
    char target[NAME_LEN + 1] = {};
    getName(target, header.targetname);
	char* content = read_buf + sizeof(Header);

    if (strcmp(target, "def") != 0) {  // TODO Name dynamisch vergeben
        forward(target, read_buf);
    } else {
        switch (header.type) {
            case TYPE_LOGIN:
                msg_login(sender, content);
                break;
            case TYPE_CHAT:
                msg_chat(sender, content);
                break;
            case TYPE_LOGOUT:
                msg_logout(sender);
                break;
            case TYPE_ROUTE:
                msg_route(sender, content);
                break;
            case TYPE_HEART:
                msg_heart(sender);
                break;
            case TYPE_HEARTRESPONSE:
                msg_heartresponse(sender);
                break;
            case TYPE_ERROR:
                msg_error();
                break;
            default:
                printf("[Server] Unknown message type: %d\n", header.type);
        }
    }

    free(read_buf);
}
