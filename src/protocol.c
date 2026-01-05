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

#include "protocol_header.h"
#include "queue.h"
#include "routingTable.h"
#include "tcp_con.h"

char ownName[32];

void do_chat(const char* target, const char* msg) {
    Header newHeader;
    int offset = 0;
    protocol_create_header(&newHeader, ownName, target, TYPE_CHAT);
    
    char message[sizeof(Header) + MSG_SIZE] = {0};
    memcpy(message + offset, &newHeader, sizeof(Header));
    offset += sizeof(Header);
    strcpy(message + offset, msg); // TODO msg fragmentieren, wenn zu lang
    offset += msglen(message + offset);
    message[offset] = '\004';

    uint64_t targetAdresseUndPort = getRouting(target);
    push_send(targetAdresseUndPort, message, msglen(message));
}

void do_login(const char* chat_name, const int local_port, const char* target_host, const int target_port) {
    printf("Sende login\n");
    Header newHeader;
    int offset = 0;
    protocol_create_header(&newHeader, ownName, "???", TYPE_LOGIN);
    char message[sizeof(Header) + MSG_SIZE] = {0};
    memcpy(message + offset, &newHeader, sizeof(Header));
    offset += sizeof(Header);
    getName(message + offset, chat_name); // TODO wie soll der Name eigentlich drin stehen?
    offset += NAME_LEN;
    struct in_addr addr;
    inet_aton(target_host, &addr);
    memcpy(message + offset, &addr.s_addr, 4);
    offset += 4;
    memcpy(message + offset, &target_port, 2);
    offset += 2;
    message[offset] = '\004';

    push_send(((uint64_t)addr.s_addr << 32) | (uint64_t)target_port, message, msglen(message));
}

void forward(const char* target, Header header, const char* msg) {
    printf("Forwarding message to %s\n", target);
    uint64_t targetAdresseUndPort = getRouting(target);
    if (targetAdresseUndPort == 0) {
        printf("No route to target %s\n", target);
        return;
    }
    char message[sizeof(Header) + msglen(msg)];
    memcpy(message, &header, sizeof(Header));
    memcpy(message + sizeof(Header), msg, msglen(msg));
    push_send(targetAdresseUndPort, message, sizeof(message));

    // TODO \004 wahrscheinlich noch an msg anhängen
    // target in routing tabelle suchen
    // gesamte Nachricht (msg) an target aus routing tabelle senden
    // keine Heart und heartresponse weiterleiten (sollte hier gar nicht ankommen)
}

void msg_login(const char* sender, const char* content) {
    printf("Handling login message\n");
    uint8_t contentNew[OFFSETMESSAGECOUNT];
    memset(contentNew, 0, sizeof(contentNew));
    memcpy(contentNew, content, OFFSETMESSAGECOUNT);
    memcpy(contentNew + OFFSETNEXTCHATNAME, content + OFFSETNEXTCHATNAME, content);
    contentNew[OFFSETHOPCOUNT] = 0;  // hop count auf 0 setzen
    tableUpdate(contentNew, sizeof(content));
    uint8_t ergebnis[getRoutingTableSize()];
    memset(ergebnis, 0, sizeof(ergebnis));
    tableToCharArray(ergebnis);
    // sender zur routing tabelle hinzufügen
    // aktualisierte ROUTE message versenden

    push_ui("<Empfänger hat sich angemeldet!>", sender);

    user hopsOneAway[getRoutingTableSize() / OFFSETMESSAGECOUNT];
    memset(hopsOneAway, 0, sizeof(hopsOneAway));
    getHopsOneAway(hopsOneAway);
    int index = 0;

    while (hopsOneAway[index].chatName[0] != '\0' &&
           index < (getRoutingTableSize() / OFFSETMESSAGECOUNT)) {
        Header header;
        protocol_create_header(&header, ownName, hopsOneAway[index].chatName, TYPE_ROUTE);
        char message[sizeof(Header) + sizeof(ergebnis)];
        memcpy(message, &header, sizeof(Header));
        memcpy(message + sizeof(Header), ergebnis, sizeof(ergebnis));
        push_send(((uint64_t)hopsOneAway[index].adress) << 32 | (uint64_t)hopsOneAway[index].port,
                  message, sizeof(message));
    }
}

void msg_chat(const char* sender, /*const*/ char* content) {
    // printf("Chat from %s: %s\n", sender, content);
    //  auf UI anzeigen
    content[msglen(content)-1] = 0;  // EOT entfernen für UI
    push_ui(content, sender);
}

void msg_logout(const char* sender) {
    printf("Handling logout message\n");
    // sender aus routing tabelle entfernen (disconnect)
    push_ui("<Empfänger hat sich abgemeldet!>", sender);
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

void msg_error(const char* sender) {
    printf("Handling error message\n");
    push_ui("<Fehler beim Senden!>", sender);
}

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

    printf("Nachricht bekommen, Type: %d, From: %s, To: %s\n", header.type, sender, target);
    if (header.type != TYPE_LOGIN && strcmp(target, ownName) != 0) {
        forward(target, header, content);
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
                msg_error(sender);
                break;
            default:
                printf("[Server] Unknown message type: %d\n", header.type);
        }
    }

    free(read_buf);
}
