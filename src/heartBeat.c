
#include "heartBeat.h"

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "protocol.h"
#include "protocol_header.h"
#include "queue.h"
#include "routingTable.h"
#include "sender.h"

typedef struct {
    user oneHopaway[50];
    pthread_mutex_t lock;
} Users;

Users users = {0};

void __attribute__((constructor)) init_users() {
    pthread_mutex_init(&users.lock, NULL);
}

void sendHeartbeatSignal() {
    pthread_mutex_lock(&users.lock);
    for (int i = 0; i < sizeof(users.oneHopaway) / sizeof(user); i++) {
        if (users.oneHopaway[i].chatName[0] == '\0') {
            break;
        }
        Header header;
        memset(&header, 0, sizeof(Header));
        protocol_create_header(&header, ownName, users.oneHopaway[i].chatName, TYPE_HEART);
        push_send(SINGLE, getRouting(users.oneHopaway[i].chatName), (char*)&header, sizeof(Header));
    }
    pthread_mutex_unlock(&users.lock);
}

void* heartBeatFunction(void* arg) {
    while (1) {
        pthread_mutex_lock(&users.lock);
        getHopsOneAway(users.oneHopaway);
        if (users.oneHopaway[0].chatName[0] == '\0') {
            pthread_mutex_unlock(&users.lock);
            usleep(1000 * 1000);
            continue;
        }
        pthread_mutex_unlock(&users.lock);
        memset(users.oneHopaway, 0, sizeof(users.oneHopaway));
        getHopsOneAway(users.oneHopaway);
        sendHeartbeatSignal();
        usleep(2000 * 1000);
        pthread_mutex_lock(&users.lock);
        for (int i = 0; i < sizeof(users.oneHopaway) / sizeof(user); i++) {
            if (users.oneHopaway[i].chatName[0] == '\0') {
                break;
            }

            if (users.oneHopaway[i].notResponded == 0) {
                char str[100] = {0};
                sprintf(str, "User %s is not responding. Removing from routing table.\n",
                        users.oneHopaway[i].chatName);
                push_ui(str, users.oneHopaway[i].chatName);
                deleteFromTable(users.oneHopaway[i].chatName);
                memset(&users.oneHopaway[i], 0, sizeof(user));
                // After deletion, send updated ROUTE messages to all one-hop-away users
                getHopsOneAway(users.oneHopaway);
                uint8_t tableArray[getSizeofRoutingTable()];
                memset(tableArray, 0, sizeof(tableArray));
                tableToCharArray(tableArray);
                int index = 0;
                while (users.oneHopaway[index].chatName[0] != '\0' &&
                       index < (getRoutingTableSize() / OFFSETMESSAGECOUNT)) {
                    Header header;
                    protocol_create_header(&header, ownName, users.oneHopaway[index].chatName,
                                           TYPE_ROUTE);
                    char fullMessage[sizeof(Header) + sizeof(tableArray)];
                    memcpy(fullMessage, &header, sizeof(Header));
                    memcpy(fullMessage + sizeof(Header), tableArray, sizeof(tableArray));
                    push_send(SINGLE, (getRouting(users.oneHopaway[index].chatName)), fullMessage,
                              sizeof(fullMessage));
                    index++;
                }

            } else {
                users.oneHopaway[i].notResponded = 0;
            }
        }
        pthread_mutex_unlock(&users.lock);
        usleep(600 * 1000);
    }
    return NULL;
}

void startHeartbeatThread() {
    pthread_t heartbeatThread;
    if (pthread_create(&heartbeatThread, NULL, heartBeatFunction, NULL) != 0) {
        perror("Failed to create heartbeat thread");
        exit(EXIT_FAILURE);
    }
    pthread_detach(heartbeatThread);  // Detach the thread to allow automatic resource cleanup
}

void receiveHeartbeatResponse(const char* senderName) {
    pthread_mutex_lock(&users.lock);
    getHopsOneAway(users.oneHopaway);
    for (int i = 0; i < sizeof(users.oneHopaway) / sizeof(user); i++) {
        if (strcmp(users.oneHopaway[i].chatName, senderName) == 0) {
            users.oneHopaway[i].notResponded = 1;
            break;
        }
    }
    pthread_mutex_unlock(&users.lock);
}
