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

#include "heartBeat.h"
#include "protocol_header.h"
#include "queue.h"
#include "routingTable.h"
#include "tcp_con.h"

char ownName[32];

void do_chat(const char* target, const char* msg) {
    if (strcmp(target, "System") == 0) {
        if (strcmp(msg, "routing") == 0) {
            printRoutingTable();
        }
        return;
    }

    Header newHeader;
    int offset = 0;
    protocol_create_header(&newHeader, ownName, target, TYPE_CHAT);

    char message[MSG_SIZE] = {0};
    memcpy(message + offset, &newHeader, sizeof(Header));
    offset += sizeof(Header);
    strcpy(message + offset, msg);  // TODO msg fragmentieren, wenn zu lang
    offset += strlen(msg);
    message[offset] = EOT;

    uint64_t targetAdresseUndPort = getRouting(target);
    push_send(SINGLE, targetAdresseUndPort, message,
              msglen(message));  // TODO SINGLE zu UP/DOWN, wenn fragmentiert
}

void do_login(const char* chat_name, const char* local_host, const int local_port,
              const char* target_host, const int target_port) {
    // printf("Sende login\n");
    Header newHeader;
    int offset = 0;
    protocol_create_header(&newHeader, ownName, "", TYPE_LOGIN);
    char message[MSG_SIZE] = {0};
    memset(message, 0, sizeof(message));
    memcpy(message + offset, &newHeader, sizeof(Header));
    offset += sizeof(Header);
    getName(message + offset, chat_name);
    offset += NAME_LEN;

    struct in_addr local_addr;
    inet_aton(local_host, &local_addr);
    // printf("Target IP: %d\n", ((uint32_t)addr.s_addr));
    memcpy(message + offset, &local_addr.s_addr, 4);
    offset += 4;
    // memcpy(message + offset, &target_port, 2);
    message[offset] = (uint8_t)((local_port >> 8) & 0xFF);
    message[offset + 1] = (uint8_t)(local_port & 0xFF);
    offset += 2;
    message[offset] = EOT;

    struct in_addr target_addr;
    inet_aton(target_host, &target_addr);
    push_send(SINGLE, ((uint64_t)target_addr.s_addr << 32) | (uint64_t)target_port, message,
              msglen(message));
}

void do_logout() {
    Header newHeader;
    protocol_create_header(&newHeader, ownName, "\0", TYPE_LOGOUT);
    char message[MSG_SIZE] = {0};

    memset(message, 0, sizeof(message));
    memcpy(message, &newHeader, sizeof(Header));
    message[sizeof(Header)] = EOT;

    send_to_all_neighboors(message, "\0", sizeof(message));
}

// Liste -- kann ggf. verallgemeinert werden -----------------------------------
struct list {
    char chatName[32];
    struct list* next;
}* list_head;

// @return 1, wenn schon vorhanden, sonst 0
int add_list(const char* chatName) {
    if (list_head == NULL) {
        list_head = malloc(sizeof(struct list));
        strcpy(list_head->chatName, chatName);
        list_head->next = NULL;
    } else {
        struct list* current = list_head;
        if (strcmp(current->chatName, chatName) == 0) {
            return 1;
        }

        while (current->next != NULL) {
            current = current->next;
            if (strcmp(current->chatName, chatName) == 0) {
                return 1;
            }
        }
        current->next = malloc(sizeof(struct list));
        strcpy(current->next->chatName, chatName);
        current->next->next = NULL;
    }
    return 0;
}

void remove_list(const char* chatName) {
    struct list* current = list_head;
    struct list* prev = NULL;

    while (current != NULL) {
        if (strcmp(current->chatName, chatName) == 0) {
            if (prev == NULL) {
                list_head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}
// -------------------------------------------------------------------

void forward(const char* target, const char* msg) {
    // printf("Forwarding message to %s\n", target);
    uint64_t targetAdresseUndPort = getRouting(target);
    if (targetAdresseUndPort == 0) {
        printf("No route to target %s\n", target);
        return;
    }

    if (!add_list(target)) {
        push_send(UP, targetAdresseUndPort, msg, msglen(msg));
    } else if (memchr(msg, '\004', msglen(msg)) == NULL) {
        push_send(KEEP, targetAdresseUndPort, msg, msglen(msg));
    } else {
        push_send(DOWN, targetAdresseUndPort, msg, msglen(msg));
        remove_list(target);
    }

    // TODO \004 wahrscheinlich noch an msg anhängen
    // target in routing tabelle suchen
    // gesamte Nachricht (msg) an target aus routing tabelle senden
    // keine Heart und heartresponse weiterleiten (sollte hier gar nicht ankommen)
}

void msg_login(const char* sender, const char* content) {
    // printf("Handling login message\n");
    uint8_t contentNew[OFFSETMESSAGECOUNT];
    memset(contentNew, 0, sizeof(contentNew));
    // Name
    getName((char*)contentNew, (char*)content);
    // Addresse und Port
    memcpy(contentNew + OFFSETADRESS, content + OFFSETADRESS, OFFSETNEXTCHATNAME - OFFSETADRESS);

    // Next Name
    getName((char*)contentNew + OFFSETNEXTCHATNAME, (char*)content);
    // Next Name und Port
    memcpy(contentNew + OFFSETNEXTCHATNAME + OFFSETADRESS, content + OFFSETADRESS,
           OFFSETNEXTCHATNAME - OFFSETADRESS);

    contentNew[OFFSETHOPCOUNT] = 0;  // hop count auf 0 setzen
    tableUpdate(contentNew, sizeof(contentNew));
    uint8_t ergebnis[getRoutingTableSize()];
    memset(ergebnis, 0, sizeof(ergebnis));
    tableToCharArray(ergebnis);
    // sender zur routing tabelle hinzufügen
    // aktualisierte ROUTE message versenden

    Header header;
    protocol_create_header(&header, ownName, sender, TYPE_HEART);
    push_send(SINGLE, getRouting(sender), (char*)&header, sizeof(Header));

    protocol_create_header(&header, ownName, "\0", TYPE_ROUTE);
    char message[sizeof(Header) + sizeof(ergebnis)];
    memcpy(message, &header, sizeof(Header));
    memcpy(message + sizeof(Header), ergebnis, sizeof(ergebnis));

    send_to_all_neighboors(message, "\0", sizeof(message));
}

void send_to_all_neighboors(char full_message[], char* skip_sender, int size) {
    user hopsOneAway[getRoutingTableSize() / OFFSETMESSAGECOUNT];
    memset(hopsOneAway, 0, sizeof(hopsOneAway));
    getHopsOneAway(hopsOneAway);
    int index = 0;

    while (hopsOneAway[index].chatName[0] != '\0' &&
           index < (getRoutingTableSize() / OFFSETMESSAGECOUNT)) {
        setName(full_message + 34, hopsOneAway[index].chatName);  // set target name in header

        if (strcmp(hopsOneAway[index].chatName, skip_sender) == 0) {
            // printf("Not sending ROUTE to sender %s\n", sender);
            index++;
            continue;
        }
        push_send(SINGLE, (getRouting(hopsOneAway[index].chatName)), full_message, size);
        index++;
    }
}

void msg_chat(const char* sender, /*const*/ char* content) {
    // printf("Chat from %s: %s\n", sender, content);
    //  auf UI anzeigen
    content[msglen(content) - 1] = 0;  // EOT entfernen für UI
    push_ui(content, sender);
}

void msg_logout(const char* sender) {
    // sender aus routing tabelle entfernen (disconnect)
    push_ui("<Empfänger hat sich abgemeldet!>", sender);
}

void msg_route(const char* sender, const char* content, int size) {
    tableUpdate((uint8_t*)content, size);

    // printf("Handling route message\n");
    uint8_t message[getRoutingTableSize()];
    memset(message, 0, sizeof(message));
    tableToCharArray(message);

    Header header;
    protocol_create_header(&header, sender, "\0", TYPE_ROUTE);
    char fullMessage[sizeof(Header) + sizeof(message)];
    memcpy(fullMessage, &header, sizeof(Header));
    memcpy(fullMessage + sizeof(Header), message, sizeof(message));

    send_to_all_neighboors(fullMessage, sender, sizeof(fullMessage));
    // hop counts erhöhen
    // eigene routing tabelle mit neuen Daten aktualisieren
    // aktualisierte ROUTE message versenden

    // printRoutingTable();
}

void msg_heart(const char* sender) {
    printf("Handling heart message\n");
    Header newHeader;
    protocol_create_header(&newHeader, ownName, sender, TYPE_HEARTRESPONSE);
    push_send(SINGLE, getRouting(sender), (char*)&newHeader, sizeof(Header));
}

void msg_heartresponse(const char* sender) { receiveHeartbeatResponse(sender); }

void msg_error(const char* sender) { push_ui("<Fehler beim Senden!>", sender); }

void protocol_handle_msg(const int connection) {
    char* read_buf;
    int count = read_tcp(connection, &read_buf);
    if (count < 0) {
        if (errno != EAGAIN) {
            close_tcp(connection);
            free(read_buf);
            return;
        }
    }

    if (count == 0) {
        close_tcp(connection);
        free(read_buf);
        return;
    }

    if (count < sizeof(Header)) {
        push_ui("[Server] Message too short\n", "System");
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

    // Da einige in ihren Route Nachrichten kein Target schreiben und Route Nachrichten eh nie
    // weitergeletet werden, hier abfangen:
    if (header.type != TYPE_LOGIN && header.type != TYPE_ROUTE && strcmp(target, ownName) != 0) {
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
                msg_route(sender, content, count - sizeof(Header) - 1);
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
                printf("");  // Das muss so...
                char unknownMsg[40] = {0};
                sprintf(unknownMsg, "[Server] Unknown message type: %d\n", header.type);
                push_ui(unknownMsg, "System");
        }
    }

    free(read_buf);
}
