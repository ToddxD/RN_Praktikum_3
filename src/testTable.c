
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

#define OFFSETCHATNAME 0
#define OFFSETADRESS 32
#define OFFSETPORT 36
#define OFFSETNEXTCHATNAME 38
#define OFFSETNEXTADRESS 70
#define OFFSETNEXTPORT 74
#define OFFSETHOPCOUNT 76
#define OFFSETMESSAGECOUNT 80

typedef struct __attribute__((packed)) {
    char chatName[32];
    uint32_t adress;
    uint16_t port;
    char nextChatName[32]; 
    uint32_t nextAdress;
    uint16_t nextPort;
    uint32_t hopCount;
} routingTableEntry;

routingTableEntry routingTable[100] = {0}; 
int freeEntries = 99;


void initTable(char* ownName, int ownAdress, int ownPort){
    //printf("%d hallo\n", sizeof(routingTable));
    //printf("%d hallo\n", sizeof(routingTableEntry));
    //routingTable = calloc(2, sizeof(routingTableEntry));
    //printf("%d hallo\n", sizeof(routingTable));
    memcpy(routingTable[0].chatName, ownName, strlen(ownName)+1);
    printf("%s\n", routingTable[0].chatName);
    printf("hallo2\n");
    routingTable[0].adress = ownAdress;
    routingTable[0].port = ownPort;
    routingTable[0].hopCount = 0;
}

void tableUpdate(uint8_t* message, int length){
    uint8_t workingArray[length] = {0};
    for (int i = 0; i < length/OFFSETMESSAGECOUNT; i++)
    {
        memcpy(workingArray, message + i*OFFSETMESSAGECOUNT, OFFSETMESSAGECOUNT);
        char* charName = memcpy(charName, workingArray + OFFSETCHATNAME, 32);
        int index = checkNameInTable(charName);
        if(!index){
            routingTableEntry newEntry;
            memcpy(newEntry.chatName, workingArray + OFFSETCHATNAME, 32);
            newEntry.adress = (uint32_t)(workingArray[OFFSETADRESS] << 24) | (uint32_t)(workingArray[OFFSETADRESS+1] << 16) | (uint32_t)(workingArray[OFFSETADRESS+2] << 8) | (uint32_t)(workingArray[OFFSETADRESS+3]);
            newEntry.port = (uint16_t)(workingArray[OFFSETPORT] << 8) | (uint16_t)(workingArray[OFFSETPORT+1]);
            memcpy(newEntry.nextChatName, workingArray + OFFSETNEXTCHATNAME, 32);
            newEntry.nextAdress = (uint32_t)(workingArray[OFFSETNEXTADRESS] << 24) | (uint32_t)(workingArray[OFFSETNEXTADRESS+1] << 16) | (uint32_t)(workingArray[OFFSETNEXTADRESS+2] << 8) | (uint32_t)(workingArray[OFFSETNEXTADRESS+3]);
            newEntry.nextPort = (uint16_t)(workingArray[OFFSETNEXTPORT] << 8) | (uint16_t)(workingArray[OFFSETNEXTPORT+1]);
            newEntry.hopCount = (uint32_t)(((workingArray[OFFSETHOPCOUNT] << 24) | (uint32_t)(workingArray[OFFSETHOPCOUNT+1] << 16) | (uint32_t)(workingArray[OFFSETHOPCOUNT+2] << 8) | (uint32_t)(workingArray[OFFSETHOPCOUNT+3]))+1);
            if(freeEntries > 0){
                for(int j = 0; j < sizeof(routingTable)/80; j++){
                    if(routingTable[j].hopCount == 0 && strcmp(routingTable[j].chatName, "") == 0){
                        routingTable[j] = newEntry;
                        freeEntries--;
                        break;
                    }
                }
            } else {
                printf("Routing Table full, cannot add new entry for %s\n", newEntry.chatName);
                continue;
            }
            for(int j = 0; j < sizeof(routingTable)/80; j++){
                if(routingTable[j].hopCount == 0 && strcmp(routingTable[j].chatName, "") == 0){
                    routingTable[j] = newEntry;
                    break;
                }
            }
        } else if(routingTable[index].hopCount > (uint32_t)(((workingArray[OFFSETHOPCOUNT] << 24) | (uint32_t)(workingArray[OFFSETHOPCOUNT+1] << 16) | (uint32_t)(workingArray[OFFSETHOPCOUNT+2] << 8) | (uint32_t)(workingArray[OFFSETHOPCOUNT+3]))+1)){
            memcpy(routingTable[index].nextChatName, workingArray + OFFSETNEXTCHATNAME, 32);
            routingTable[index].nextAdress = (uint32_t)(workingArray[OFFSETNEXTADRESS] << 24) | (uint32_t)(workingArray[OFFSETNEXTADRESS+1] << 16) | (uint32_t)(workingArray[OFFSETNEXTADRESS+2] << 8) | (uint32_t)(workingArray[OFFSETNEXTADRESS+3]);
            routingTable[index].nextPort = (uint16_t)(workingArray[OFFSETNEXTPORT] << 8) | (uint16_t)(workingArray[OFFSETNEXTPORT+1]);
            routingTable[index].hopCount = (uint32_t)(((workingArray[OFFSETHOPCOUNT] << 24) | (uint32_t)(workingArray[OFFSETHOPCOUNT+1] << 16) | (uint32_t)(workingArray[OFFSETHOPCOUNT+2] << 8) | (uint32_t)(workingArray[OFFSETHOPCOUNT+3]))+1);
        }
    }
}

uint64_t getRouting(char* chatName){
    for(int i = 0; i < sizeof(routingTable)/80; i++){
        if(strcmp(routingTable[i].chatName, chatName) == 0){
            uint64_t adressAndPort = ((uint64_t)routingTable[i].nextAdress << 32) | (uint64_t)routingTable[i].nextPort;
            return adressAndPort;
        }
    }
    return -1;
}

int deleteFromTable(char* chatName){
    for(int i = 0; i < sizeof(routingTable)/80; i++){
        if(strcmp(routingTable[i].chatName, chatName) == 0){
            memset(&routingTable[i], 0, sizeof(routingTableEntry));
            freeEntries++;
            return 0;
        }
    }
    return -1;
}

void tableToCharArray(uint8_t* ergebnis){
    printf("hallo3\n");
    for(int i = 0; i < sizeof(routingTable)/80; i++){
        uint8_t *name = (uint8_t*)routingTable[i].chatName;
         for(int j = 0; j< 32; j++){
            printf("%d", name[j]);
            ergebnis[j+i*80] = name[j];
            //ergebnis[j+i*80] = (uint8_t)routingTable[i].chatName[j];
        }
    ergebnis[i*80+OFFSETADRESS] = (uint8_t)(routingTable[i].adress >> 24);
    //printf("%d", (routingTable[i].adress >> 24));
    ergebnis[i*80+OFFSETADRESS+1] = (uint8_t)(routingTable[i].adress >> 16);
    //printf("%d", (routingTable[i].adress >> 16));
    ergebnis[i*80+OFFSETADRESS+2] = (uint8_t)(routingTable[i].adress >> 8);
    //printf("%d", (routingTable[i].adress >> 8));
    ergebnis[i*80+OFFSETADRESS+3] = (uint8_t)(routingTable[i].adress);
    //printf("%d\n", ergebnis[i*80+OFFSETADRESS+3]);
    //printf("%d", (uint8_t)(routingTable[i].adress));
    ergebnis[i*80+OFFSETPORT] = (uint8_t)(routingTable[i].port >> 8);
    ergebnis[i*80+OFFSETPORT+1] = (uint8_t)(routingTable[i].port);
    for(int k = 0; k< 32; k++){
            ergebnis[OFFSETNEXTCHATNAME+k+i*80] = (uint8_t)routingTable[i].nextChatName[k];
        }
    ergebnis[i*80+OFFSETNEXTADRESS] = (uint8_t)(routingTable[i].nextAdress >> 24);
    ergebnis[i*80+OFFSETNEXTADRESS+1] = (uint8_t)(routingTable[i].nextAdress >> 16);
    ergebnis[i*80+OFFSETNEXTADRESS+2] = (uint8_t)(routingTable[i].nextAdress >> 8);
    ergebnis[i*80+OFFSETNEXTADRESS+3] = (uint8_t)(routingTable[i].nextAdress);
    ergebnis[i*80+OFFSETNEXTPORT] = (uint8_t)(routingTable[i].nextPort >> 8);
    ergebnis[i*80+OFFSETNEXTPORT+1] = (uint8_t)(routingTable[i].nextPort);
    ergebnis[i*80+OFFSETHOPCOUNT] = (uint8_t)(routingTable[i].hopCount >> 24);
    ergebnis[i*80+OFFSETHOPCOUNT+1] = (uint8_t)(routingTable[i].hopCount >> 16);
    ergebnis[i*80+OFFSETHOPCOUNT+2] = (uint8_t)(routingTable[i].hopCount >> 8);
    ergebnis[i*80+OFFSETHOPCOUNT+3] = (uint8_t)(routingTable[i].hopCount);
    }
    printf("hallo4\n");
}

int checkNameInTable(char* chatName){
    for(int i = 0; i < sizeof(routingTable)/80; i++){
        if(strcmp(routingTable[i].chatName, chatName) == 0){
            return i;
        }
    }
    return 0;
}

int main(int argc, char **argv){
    char* testName = "HalloGutenTag";
    initTable(testName, 1234567, 8000);
    uint8_t ergebnis[sizeof(routingTable)] = {0};
    tableToCharArray(ergebnis);
    printf("hallo5\n");
    printf("%d\n", sizeof(ergebnis));
    for(int i = 0; i < 800; i++){
    printf("%d", ergebnis[i]);
    }

    uint8_t testMessage[80] = {0x41, 0x6C, 0x69, 0x63, 0x65,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Address: 192.168.1.10
    0xC0, 0xA8, 0x01, 0x0A,

    // Port: 5000
    0x13, 0x88,

    // Next Chat Name: "Bob" (32 B)
    0x42, 0x6F, 0x62,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Next Address: 10.0.0.5
    0x0A, 0x00, 0x00, 0x05,

    // Next Port: 6000
    0x17, 0x70,

    // Hop Count: 3
    0x00, 0x00, 0x00, 0x03};

    tableUpdate(testMessage, 80);
    tableToCharArray(ergebnis);
    printf("hallo6\n");
    printf("%d\n", sizeof(ergebnis));
    for(int i = 0; i < 800; i++){
    printf("%d", ergebnis[i]);
    }

    //free(routingTable);
    //free(ergebnis);
    return 0;
}


