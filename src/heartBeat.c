
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "heartBeat.h"
#include "routingTable.h"
#include "sender.h"
#include "queue.h"
#include "protocol.h"
#include "protocol_header.h"

typedef struct {
    user oneHopaway[50];
    pthread_mutex_t lock;
} Users;

Users users = {
    .lock = PTHREAD_MUTEX_INITIALIZER
};
void sendHeartbeatSignal() {
    pthread_mutex_lock(&users.lock);
    getHopsOneAway(users.oneHopaway);
    for(int i = 0; i< sizeof(users.oneHopaway)/sizeof(user); i++) {
        if(users.oneHopaway[i].chatName[0] == '\0') {
            break;
        }
        Header header;
        memset(&header, 0, sizeof(Header));
        protocol_create_header(&header, ownName , users.oneHopaway[i].chatName, TYPE_HEART);
        push_send(SINGLE, getRouting(users.oneHopaway[i].chatName), (char*)&header, sizeof(Header));
    }
    pthread_mutex_unlock(&users.lock);
}

void* heartBeatFunction(void* arg) {
    while (1) {
        pthread_mutex_lock(&users.lock);
        getHopsOneAway(users.oneHopaway);
        if(users.oneHopaway[0].chatName[0] == '\0') {
        usleep(1000*1000);
        pthread_mutex_unlock(&users.lock);
        continue;
        }
        pthread_mutex_unlock(&users.lock);
         sendHeartbeatSignal();
         usleep(200*1000);
         pthread_mutex_lock(&users.lock);
        for(int i = 0; i < sizeof(users.oneHopaway)/sizeof(user); i++) {
            if(users.oneHopaway[i].chatName[0] == '\0') {
                break;
            }
            if(users.oneHopaway[i].notResponded == 1) {
                printf("User %s is not responding. Removing from routing table.\n", users.oneHopaway[i].chatName);
                push_ui("<Empfänger %s nicht erreichbar, aus Routing Tabelle entfernt!>", users.oneHopaway[i].chatName);
                deleteFromTable(users.oneHopaway[i].chatName);
                getHopsOneAway(users.oneHopaway);
                int index = 0;
                while (users.oneHopaway[index].chatName[0] != '\0' &&
                    index < (getRoutingTableSize() / OFFSETMESSAGECOUNT)) {
                     Header header;
                     protocol_create_header(&header, ownName, users.oneHopaway[index].chatName, TYPE_ROUTE);
                     char fullMessage[sizeof(Header)];
                     memcpy(fullMessage, &header, sizeof(Header));
                    push_send(SINGLE, (getRouting(users.oneHopaway[index].chatName)), fullMessage, sizeof(fullMessage));
                    index++;
                }
                
            } else {
                users.oneHopaway[i].notResponded = 1;

            }

        }
        pthread_mutex_unlock(&users.lock);     
        usleep(800*1000); 
    
    }
        return NULL;
}

void startHeartbeatThread() {
    pthread_mutex_lock(&users.lock);
    memset(users.oneHopaway, 0, sizeof(users.oneHopaway));
    pthread_mutex_unlock(&users.lock);
    pthread_t heartbeatThread;
    if (pthread_create(&heartbeatThread, NULL, heartBeatFunction, NULL) != 0) {
        perror("Failed to create heartbeat thread");
        exit(EXIT_FAILURE);
    }
    pthread_detach(heartbeatThread); // Detach the thread to allow automatic resource cleanup
}

void receiveHeartbeatResponse(const char* senderName) {
    pthread_mutex_lock(&users.lock);
    getHopsOneAway(users.oneHopaway);
    for(int i = 0; i < sizeof(users.oneHopaway)/sizeof(user); i++) {
        if(strcmp(users.oneHopaway[i].chatName, senderName) == 0) {
            users.oneHopaway[i].notResponded = 0;
            break;
        }
    }
    pthread_mutex_unlock(&users.lock);
}

