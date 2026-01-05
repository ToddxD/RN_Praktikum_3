
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

routingTableEntry routingTable[5] = {0};
int freeEntries = 99;

void initTable(char* ownName, int ownAdress, int ownPort) {
    memset(routingTable, 0, sizeof(routingTable));
    memcpy(routingTable[0].chatName, ownName, strlen(ownName) + 1);
    routingTable[0].adress = ownAdress;
    routingTable[0].port = ownPort;
    routingTable[0].hopCount = 0;
}

int checkNameInTable(char* chatName) {
    for (int i = 0; i < sizeof(routingTable) / 80; i++) {
        if (strcmp(routingTable[i].chatName, chatName) == 0) {
            return i;
        }
    }
    return 0;
}

void tableUpdate(uint8_t* message, int length) {
    uint8_t workingArray[OFFSETMESSAGECOUNT];
    memset(workingArray, 0, OFFSETMESSAGECOUNT);
    for (int i = 0; i < length / OFFSETMESSAGECOUNT; i++) {
        memcpy(workingArray, message + i * OFFSETMESSAGECOUNT, OFFSETMESSAGECOUNT);
        char charName[32];
        memcpy(charName, workingArray + OFFSETCHATNAME, 32);
        int index = checkNameInTable(charName);
        if (!index) {
            routingTableEntry newEntry;
            memcpy(newEntry.chatName, workingArray + OFFSETCHATNAME, 32);
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
                        break;
                    }
                }
            } else {
                printf("Routing Table full, cannot add new entry for %s\n", newEntry.chatName);
                continue;
            }
        } else if (routingTable[index].hopCount >
                   (uint32_t)(((workingArray[OFFSETHOPCOUNT] << 24) |
                               (uint32_t)(workingArray[OFFSETHOPCOUNT + 1] << 16) |
                               (uint32_t)(workingArray[OFFSETHOPCOUNT + 2] << 8) |
                               (uint32_t)(workingArray[OFFSETHOPCOUNT + 3])) +
                              1)) {
            memcpy(routingTable[index].nextChatName, workingArray + OFFSETNEXTCHATNAME, 32);
            routingTable[index].nextAdress = (uint32_t)(workingArray[OFFSETNEXTADRESS] << 24) |
                                             (uint32_t)(workingArray[OFFSETNEXTADRESS + 1] << 16) |
                                             (uint32_t)(workingArray[OFFSETNEXTADRESS + 2] << 8) |
                                             (uint32_t)(workingArray[OFFSETNEXTADRESS + 3]);
            routingTable[index].nextPort = (uint16_t)(workingArray[OFFSETNEXTPORT] << 8) |
                                           (uint16_t)(workingArray[OFFSETNEXTPORT + 1]);
            routingTable[index].hopCount =
                (uint32_t)(((workingArray[OFFSETHOPCOUNT] << 24) |
                            (uint32_t)(workingArray[OFFSETHOPCOUNT + 1] << 16) |
                            (uint32_t)(workingArray[OFFSETHOPCOUNT + 2] << 8) |
                            (uint32_t)(workingArray[OFFSETHOPCOUNT + 3])) +
                           1);
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

int deleteFromTable(char* chatName) {
    for (int i = 0; i < sizeof(routingTable) / 80; i++) {
        if (strcmp(routingTable[i].chatName, chatName) == 0) {
            memset(&routingTable[i], 0, sizeof(routingTableEntry));
            freeEntries++;
            return 0;
        }
    }
    return -1;
}

int getRoutingTableSize() { return (sizeof(routingTable)); }

void tableToCharArray(uint8_t* ergebnis) {
    for (int i = 0; i < sizeof(routingTable) / 80; i++) {
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
            ergebnis[OFFSETNEXTCHATNAME + k + i * 80] = (uint8_t)routingTable[i].nextChatName[k];
        }
        ergebnis[i * 80 + OFFSETNEXTADRESS] = (uint8_t)(routingTable[i].nextAdress >> 24);
        ergebnis[i * 80 + OFFSETNEXTADRESS + 1] = (uint8_t)(routingTable[i].nextAdress >> 16);
        ergebnis[i * 80 + OFFSETNEXTADRESS + 2] = (uint8_t)(routingTable[i].nextAdress >> 8);
        ergebnis[i * 80 + OFFSETNEXTADRESS + 3] = (uint8_t)(routingTable[i].nextAdress);
        ergebnis[i * 80 + OFFSETNEXTPORT] = (uint8_t)(routingTable[i].nextPort >> 8);
        ergebnis[i * 80 + OFFSETNEXTPORT + 1] = (uint8_t)(routingTable[i].nextPort);
        ergebnis[i * 80 + OFFSETHOPCOUNT] = (uint8_t)(routingTable[i].hopCount >> 24);
        ergebnis[i * 80 + OFFSETHOPCOUNT + 1] = (uint8_t)(routingTable[i].hopCount >> 16);
        ergebnis[i * 80 + OFFSETHOPCOUNT + 2] = (uint8_t)(routingTable[i].hopCount >> 8);
        ergebnis[i * 80 + OFFSETHOPCOUNT + 3] = (uint8_t)(routingTable[i].hopCount);
    }
}

void printRoutingTable() {
    printf("Routing Table:\n");
    for (int i = 0; i < sizeof(routingTable) / 80; i++) {
        printf("Entry %d:\n", i);
        printf(" Chat Name: %s\n", routingTable[i].chatName);
        printf(" Address: %u.%u.%u.%u\n", (uint8_t)(routingTable[i].adress >> 24),
               (uint8_t)(routingTable[i].adress >> 16), (uint8_t)(routingTable[i].adress >> 8),
               (uint8_t)(routingTable[i].adress));
        printf(" Port: %u\n", routingTable[i].port);
        printf(" Next Chat Name: %s\n", routingTable[i].nextChatName);
        printf(" Next Address: %u.%u.%u.%u\n", (uint8_t)(routingTable[i].nextAdress >> 24),
               (uint8_t)(routingTable[i].nextAdress >> 16),
               (uint8_t)(routingTable[i].nextAdress >> 8), (uint8_t)(routingTable[i].nextAdress));
        printf(" Next Port: %u\n", routingTable[i].nextPort);
        printf(" Hop Count: %u\n", routingTable[i].hopCount);
    }
}
