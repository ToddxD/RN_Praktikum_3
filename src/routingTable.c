
#include "routingTable.h"

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
#include "ui.h"
#include "protocol_header.h"

routingTableEntry routingTable[100] = {0};
reachables reachbleTable[100] = {0};
int freeEntries = 99;

void initTable(char* ownName, int ownAdress, int ownPort) {
    memset(routingTable, 0, sizeof(routingTable));
    memcpy(routingTable[0].chatName, ownName, strlen(ownName) + 1);

    uint32_t adress = ((uint32_t)((ownAdress & 0xFF))) << 24 |
                ((uint32_t)((ownAdress & 0xFF00))) << 8 |
                ((uint32_t)((ownAdress & 0xFF0000))) >> 8 |
                ((uint32_t)((ownAdress & 0xFF000000))) >> 24;

    routingTable[0].adress = adress;
    routingTable[0].port = ownPort;
    memcpy(routingTable[0].nextChatName, ownName, strlen(ownName) + 1);
    routingTable[0].nextAdress = adress;
    routingTable[0].nextPort = ownPort;
    routingTable[0].hopCount = 0;
}

int checkNameInTable(char* chatName) {
    for (int i = 0; i < sizeof(routingTable) / 80; i++) {
        if (strcmp(routingTable[i].chatName, chatName) == 0) {
            return i+1;
        }
    }
    return 0;
}

int checkNameNotInMessage(uint8_t* message, char* chatName) {
    for (int i = 0; i < (sizeof(message) / OFFSETMESSAGECOUNT); i++) {
        char currentName[32];
        memcpy(currentName, message + i * OFFSETMESSAGECOUNT + OFFSETCHATNAME, 32);
        if (strcmp(currentName, chatName) == 0) {
            return 0;
        }
    }
    return 1;
}

void tableUpdate(uint8_t* message, int length) {
    uint8_t workingArray[OFFSETMESSAGECOUNT];
    memset(workingArray, 0, OFFSETMESSAGECOUNT);
    for (int i = 0; i < length / OFFSETMESSAGECOUNT; i++) {
        memcpy(workingArray, message + i * OFFSETMESSAGECOUNT, OFFSETMESSAGECOUNT);
        char charName[32];
        getName(charName, workingArray + OFFSETCHATNAME);
        int index = checkNameInTable(charName);
        if (!index) {
            routingTableEntry newEntry;
            memcpy(newEntry.chatName, charName, 32);
            uint32_t adress = ((uint32_t)(workingArray[OFFSETADRESS] << 24)) |
                              ((uint32_t)(workingArray[OFFSETADRESS + 1] << 16)) |
                              ((uint32_t)(workingArray[OFFSETADRESS + 2] << 8)) |
                              ((uint32_t)(workingArray[OFFSETADRESS + 3]));
            newEntry.adress = adress;
            newEntry.port = (uint16_t)(workingArray[OFFSETPORT] << 8) |
                            (uint16_t)(workingArray[OFFSETPORT + 1]);
            memcpy(newEntry.nextChatName, workingArray + OFFSETNEXTCHATNAME, 32);
            newEntry.nextAdress = ((uint32_t)(workingArray[OFFSETNEXTADRESS] << 24)) |
                                  ((uint32_t)(workingArray[OFFSETNEXTADRESS + 1] << 16)) |
                                  ((uint32_t)(workingArray[OFFSETNEXTADRESS + 2] << 8)) |
                                  ((uint32_t)(workingArray[OFFSETNEXTADRESS + 3]));
            newEntry.nextPort = (uint16_t)(workingArray[OFFSETNEXTPORT] << 8) |
                                (uint16_t)(workingArray[OFFSETNEXTPORT + 1]);
            newEntry.hopCount = (uint32_t)(((workingArray[OFFSETHOPCOUNT] << 24) |
                                            (uint32_t)(workingArray[OFFSETHOPCOUNT + 1] << 16) |
                                            (uint32_t)(workingArray[OFFSETHOPCOUNT + 2] << 8) |
                                            (uint32_t)(workingArray[OFFSETHOPCOUNT + 3])) +
                                           1);
            if (freeEntries > 0) {
                for (int j = 0; j < sizeof(routingTable) / 80; j++) {
                    if (routingTable[j].hopCount == 0 &&
                        strcmp(routingTable[j].chatName, "") == 0) {
                        routingTable[j] = newEntry;
                        freeEntries--;
                        reachbleTable[j].reachable = 1;
                        memcpy(reachbleTable[j].chatName, workingArray + OFFSETCHATNAME, 32);
                        push_ui("<Empfänger hat sich angemeldet!>", newEntry.chatName);
                        break;
                    }
                }
            } else {
                printf("Routing Table full, cannot add new entry for %s\n", newEntry.chatName);
                continue;
            }
        } else if (routingTable[index-1].hopCount >
                   (uint32_t)((workingArray[OFFSETHOPCOUNT] << 24) |
                               (uint32_t)(workingArray[OFFSETHOPCOUNT + 1] << 16) |
                               (uint32_t)(workingArray[OFFSETHOPCOUNT + 2] << 8) |
                               (uint32_t)(workingArray[OFFSETHOPCOUNT + 3]))) {
            memcpy(routingTable[index-1].nextChatName, workingArray + OFFSETNEXTCHATNAME, 32);
            routingTable[index-1].nextAdress = (uint32_t)(workingArray[OFFSETNEXTADRESS] << 24) |
                                             (uint32_t)(workingArray[OFFSETNEXTADRESS + 1] << 16) |
                                             (uint32_t)(workingArray[OFFSETNEXTADRESS + 2] << 8) |
                                             (uint32_t)(workingArray[OFFSETNEXTADRESS + 3]);
            routingTable[index-1].nextPort = (uint16_t)(workingArray[OFFSETNEXTPORT] << 8) |
                                           (uint16_t)(workingArray[OFFSETNEXTPORT + 1]);
            routingTable[index-1].hopCount =
                (uint32_t)(((workingArray[OFFSETHOPCOUNT] << 24) |
                            (uint32_t)(workingArray[OFFSETHOPCOUNT + 1] << 16) |
                            (uint32_t)(workingArray[OFFSETHOPCOUNT + 2] << 8) |
                            (uint32_t)(workingArray[OFFSETHOPCOUNT + 3])) +
                           1);
            reachbleTable[index-1].reachable = 1;
        } else {
            reachbleTable[index-1].reachable = 1;
            continue;
        }
    }
    printRoutingTable();
    cleanUp();
}

void cleanUp() {
    for (int i = 0; i < sizeof(reachbleTable) / sizeof(reachables); i++) {
        if (reachbleTable[i].chatName[0] == '\0') {
            break;
        }
        if (reachbleTable[i].reachable == 0) {
            deleteFromTable(reachbleTable[i].chatName);
            push_ui("<Empfänger %s nicht erreichbar, aus Routing Tabelle entfernt!>", reachbleTable[i].chatName);
        } else {
            reachbleTable[i].reachable = 0;
        }
    }
}

void getHopsOneAway(user* hopsOneAway) {
    int count = 0;
    for (int i = 0; i < sizeof(routingTable) / 80; i++) {
        if (routingTable[i].hopCount == 1) {
            memcpy(hopsOneAway[count].chatName, routingTable[i].chatName, 32);
            hopsOneAway[count].adress = routingTable[i].adress;
            hopsOneAway[count].port = routingTable[i].port;
            hopsOneAway[count].notResponded = 1;
            count++;
        }
    }
    hopsOneAway[count].chatName[0] = '\0';
}

char* getChatName(uint64_t adressUndPort) {
    uint32_t adress = (uint32_t)(adressUndPort >> 32);
    uint16_t port = (uint16_t)(adressUndPort & 0xFFFFFFFF);
    for (int i = 0; i < sizeof(routingTable) / 80; i++) {
        if (routingTable[i].adress == adress && routingTable[i].port == port) {
            return routingTable[i].chatName;
        }
    }
    return NULL;
}

uint64_t getRouting(char* chatName) {
    for (int i = 0; i < sizeof(routingTable) / 80; i++) {
        if (strcmp(routingTable[i].chatName, chatName) == 0) {
            uint64_t adressAndPort =

                ((uint64_t)((routingTable[i].nextAdress & 0xFF))) << 56 |
                ((uint64_t)((routingTable[i].nextAdress & 0xFF00))) << 40 |
                ((uint64_t)((routingTable[i].nextAdress & 0xFF0000))) << 24 |
                ((uint64_t)((routingTable[i].nextAdress & 0xFF000000))) << 8 |
                (uint64_t)routingTable[i].nextPort;
            return adressAndPort;
        }
    }
    return -1;
}
int getSizeofRoutingTable(){
    return sizeof(routingTable);
}
int deleteFromTable(char* chatName) {
    for (int i = 0; i < sizeof(routingTable) / 80; i++) {
        if (strcmp(routingTable[i].nextChatName, chatName) == 0) {
            memset(&routingTable[i], 0, sizeof(routingTableEntry));
            freeEntries++;
            return 0;
        }
    }
    return -1;
}

int getRoutingTableSize() { 
    int result = 0;
    for (int i = 0; i < sizeof(routingTable) / 80; i++) {
        if( routingTable[i].hopCount == 0 && strcmp(routingTable[i].chatName, "") == 0){
            continue;
        }
        result += OFFSETMESSAGECOUNT;
    }
    return result;
}

void tableToCharArray(uint8_t* ergebnis) {
    for (int i = 0; i < sizeof(routingTable) / 80; i++) {
        if( routingTable[i].hopCount == 0 && strcmp(routingTable[i].chatName, "") == 0){
            continue;
        }
        uint8_t* name = (uint8_t*)routingTable[i].chatName;
        for (int j = 0; j < 32; j++) {
            ergebnis[j + i * 80] = name[j];
        }
        ergebnis[i * 80 + OFFSETADRESS] = (uint8_t)(routingTable[i].adress >> 24);
        ergebnis[i * 80 + OFFSETADRESS + 1] = (uint8_t)(routingTable[i].adress >> 16);
        ergebnis[i * 80 + OFFSETADRESS + 2] = (uint8_t)(routingTable[i].adress >> 8);
        ergebnis[i * 80 + OFFSETADRESS + 3] = (uint8_t)(routingTable[i].adress);
        ergebnis[i * 80 + OFFSETPORT] = (uint8_t)(routingTable[i].port >> 8);
        ergebnis[i * 80 + OFFSETPORT + 1] = (uint8_t)(routingTable[i].port);
        for (int k = 0; k < 32; k++) {
            ergebnis[OFFSETNEXTCHATNAME + k + i * 80] = routingTable[0].nextChatName[k];
        }
        ergebnis[i * 80 + OFFSETNEXTADRESS] = (uint8_t)(routingTable[0].nextAdress >> 24);
        ergebnis[i * 80 + OFFSETNEXTADRESS + 1] = (uint8_t)(routingTable[0].nextAdress >> 16);
        ergebnis[i * 80 + OFFSETNEXTADRESS + 2] = (uint8_t)(routingTable[0].nextAdress >> 8);
        ergebnis[i * 80 + OFFSETNEXTADRESS + 3] = (uint8_t)(routingTable[0].nextAdress);
        ergebnis[i * 80 + OFFSETNEXTPORT] = (uint8_t)(routingTable[0].nextPort >> 8);
        ergebnis[i * 80 + OFFSETNEXTPORT + 1] = (uint8_t)(routingTable[0].nextPort);
        ergebnis[i * 80 + OFFSETHOPCOUNT] = (uint8_t)(routingTable[i].hopCount >> 24);
        ergebnis[i * 80 + OFFSETHOPCOUNT + 1] = (uint8_t)(routingTable[i].hopCount >> 16);
        ergebnis[i * 80 + OFFSETHOPCOUNT + 2] = (uint8_t)(routingTable[i].hopCount >> 8);
        ergebnis[i * 80 + OFFSETHOPCOUNT + 3] = (uint8_t)(routingTable[i].hopCount);
    }
}

void printRoutingTable() {
    char str[1000] = {0};
    sprintf(str, "Routing Table:\n");
    for (int i = 0; i < sizeof(routingTable) / 80; i++) {
        if(routingTable[i].hopCount == 0 && strcmp(routingTable[i].chatName, "") == 0){
            continue;
        }
        sprintf(str + strlen(str), "Entry %d:\n", i);
        sprintf(str + strlen(str), " Chat Name: %s\n", routingTable[i].chatName);
        sprintf(str + strlen(str), " Address: %u.%u.%u.%u\n", (uint8_t)(routingTable[i].adress >> 24),
               (uint8_t)(routingTable[i].adress >> 16), (uint8_t)(routingTable[i].adress >> 8),
               (uint8_t)(routingTable[i].adress));
        sprintf(str + strlen(str), " Port: %u\n", routingTable[i].port);
        sprintf(str + strlen(str), " Next Chat Name: %s\n", routingTable[i].nextChatName);
        sprintf(str + strlen(str), " Next Address: %u.%u.%u.%u\n", (uint8_t)(routingTable[i].nextAdress >> 24),
               (uint8_t)(routingTable[i].nextAdress >> 16),
               (uint8_t)(routingTable[i].nextAdress >> 8), (uint8_t)(routingTable[i].nextAdress));
        sprintf(str + strlen(str), " Next Port: %u\n", routingTable[i].nextPort);
        sprintf(str + strlen(str), " Hop Count: %u\n", routingTable[i].hopCount);
    }

    push_ui(str, "System");
}
