#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "heartBeat.h"
#include "routingTable.h"
#include "sender.h"
#include "queue.h"
#include "protocol.h"
#include "protocol_header.h"


void sendHeartbeatSignal() {
    user oneHopaway[getSizeofRoutingTable()/OFFSETMESSAGECOUNT];
    getHopsOneAway(oneHopaway);
    for(int i = 0; i< sizeof(oneHopaway)/sizeof(user); i++) {
        if(oneHopaway[i].chatName[0] == '\0') {
            break;
        }
        Header header;
        memset(&header, 0, sizeof(Header));
        protocol_create_header(&header, ownName , oneHopaway[i].chatName, TYPE_HEART);
        push_send(SINGLE, getRouting(oneHopaway[i].chatName), (char*)&header, sizeof(Header));
    }
}

void* heartBeatFunction(void* arg) {
    int checkForHeart = 0;
    user oneHopaway[getSizeofRoutingTable()/OFFSETMESSAGECOUNT];
    memset(&oneHopaway, 0, sizeof(oneHopaway));
    while (1) {
        getHopsOneAway(oneHopaway);
        if(oneHopaway[0].chatName[0] == '\0') {
        usleep(1000*1000);
        continue;
        }
         sendHeartbeatSignal();
         usleep(200*1000);
        for(int i = 0; i < sizeof(oneHopaway)/sizeof(user); i++) {
            if(oneHopaway[i].chatName[0] == '\0') {
                break;
            }
            if(oneHopaway[i].notResponded == 1) {
                printf("User %s is not responding. Removing from routing table.\n", oneHopaway[i].chatName);
                push_ui("<Empfänger nicht erreichbar, aus Routing Tabelle entfernt!>", oneHopaway[i].chatName);
                deleteFromTable(oneHopaway[i].chatName);
                getHopsOneAway(oneHopaway);
                int index = 0;
                while (oneHopaway[index].chatName[0] != '\0' &&
                    index < (getRoutingTableSize() / OFFSETMESSAGECOUNT)) {
                     Header header;
                     protocol_create_header(&header, ownName, oneHopaway[index].chatName, TYPE_ROUTE);
                     char fullMessage[sizeof(Header)];
                     memcpy(fullMessage, &header, sizeof(Header));
                    push_send(SINGLE, (getRouting(oneHopaway[index].chatName)), fullMessage, sizeof(fullMessage));
                    index++;
                }
                
        } 
        usleep(800*1000); 
    }
}
    return NULL;
    
}

void startHeartbeatThread() {
    pthread_t heartbeatThread;
    if (pthread_create(&heartbeatThread, NULL, heartBeatFunction, NULL) != 0) {
        perror("Failed to create heartbeat thread");
        exit(EXIT_FAILURE);
    }
    pthread_detach(heartbeatThread); // Detach the thread to allow automatic resource cleanup
}

void receiveHeartbeatResponse(const char* senderName) {
    user oneHopaway[getSizeofRoutingTable()/OFFSETMESSAGECOUNT];
    getHopsOneAway(oneHopaway);
    for(int i = 0; i < sizeof(oneHopaway)/sizeof(user); i++) {
        if(strcmp(oneHopaway[i].chatName, senderName) == 0) {
            oneHopaway[i].notResponded = 0;
            break;
        }
    }
}

