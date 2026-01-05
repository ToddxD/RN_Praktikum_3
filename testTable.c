
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

routingTableEntry routingTable[5] = {0}; 
int freeEntries = 99;


void initTable(char* ownName, int ownAdress, int ownPort){
    memset(routingTable, 0, sizeof(routingTable));
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

int checkNameInTable(char* chatName){
    for(int i = 0; i < sizeof(routingTable)/80; i++){
        if(strcmp(routingTable[i].chatName, chatName) == 0){
            return i;
        }
    }
    return 0;
}

void tableUpdate(uint8_t* message, int length){
    uint8_t workingArray[OFFSETMESSAGECOUNT];
    memset(workingArray, 0, OFFSETMESSAGECOUNT);
    for (int i = 0; i < length/OFFSETMESSAGECOUNT; i++)
    {
        memcpy(workingArray, message + i*OFFSETMESSAGECOUNT, OFFSETMESSAGECOUNT);
        printf("hallo7\n");
        char charName[32];
        memcpy(charName, workingArray + OFFSETCHATNAME, 32);
        printf("%s\n", charName);
        int index = checkNameInTable(charName);
        if(!index){
            routingTableEntry newEntry;
            memcpy(newEntry.chatName, workingArray + OFFSETCHATNAME, 32);
            printf("hallo8\n");
            uint32_t adress = ((uint32_t)(workingArray[OFFSETADRESS] << 24)) | ((uint32_t)(workingArray[OFFSETADRESS+1] << 16)) | ((uint32_t)(workingArray[OFFSETADRESS+2] << 8)) | ((uint32_t)(workingArray[OFFSETADRESS+3]));
            printf("Parsed adress: %u", adress);
            printf("%d.%d.%d.%d\n", (uint8_t)(adress >> 24), (uint8_t)(adress >> 16), (uint8_t)(adress >> 8), (uint8_t)(adress));
            newEntry.adress = adress;
            newEntry.port = (uint16_t)(workingArray[OFFSETPORT] << 8) | (uint16_t)(workingArray[OFFSETPORT+1]);
            memcpy(newEntry.nextChatName, workingArray + OFFSETNEXTCHATNAME, 32);
            newEntry.nextAdress = ((uint32_t)(workingArray[OFFSETNEXTADRESS] << 24)) | ((uint32_t)(workingArray[OFFSETNEXTADRESS+1] << 16)) | ((uint32_t)(workingArray[OFFSETNEXTADRESS+2] << 8)) | ((uint32_t)(workingArray[OFFSETNEXTADRESS+3]));
            newEntry.nextPort = (uint16_t)(workingArray[OFFSETNEXTPORT] << 8) | (uint16_t)(workingArray[OFFSETNEXTPORT+1]);
            newEntry.hopCount = (uint32_t)(((workingArray[OFFSETHOPCOUNT] << 24) | (uint32_t)(workingArray[OFFSETHOPCOUNT+1] << 16) | (uint32_t)(workingArray[OFFSETHOPCOUNT+2] << 8) | (uint32_t)(workingArray[OFFSETHOPCOUNT+3]))+1);
            if(freeEntries > 0){
                printf("Adding new entry for %s to routing table\n", newEntry.chatName);
                for(int j = 0; j < sizeof(routingTable)/80; j++){
                    if(routingTable[j].hopCount == 0 && strcmp(routingTable[j].chatName, "") == 0){
                        routingTable[j] = newEntry;
                        freeEntries--;
                        printf("Free entries left: %d\n", freeEntries); 
                        printf("newEntry.chatName: %s\n", newEntry.chatName);
                        printf("routingTable[j].chatName: %s\n", routingTable[j].chatName);
                        printf("%u\n", routingTable[j].adress);
                        printf("%u\n", routingTable[j].port);
                        printf("%s\n", routingTable[j].nextChatName);
                        printf("%u\n", routingTable[j].nextAdress);
                        printf("%u\n", routingTable[j].nextPort);
                        printf("%u\n", routingTable[j].hopCount);
                        break;
                    }
                }
            } else {
                printf("Routing Table full, cannot add new entry for %s\n", newEntry.chatName);
                continue;
            }
        } else if(routingTable[index].hopCount > (uint32_t)(((workingArray[OFFSETHOPCOUNT] << 24) | (uint32_t)(workingArray[OFFSETHOPCOUNT+1] << 16) | (uint32_t)(workingArray[OFFSETHOPCOUNT+2] << 8) | (uint32_t)(workingArray[OFFSETHOPCOUNT+3]))+1)){
            printf("Updating entry for %s in routing table\n", routingTable[index].chatName);
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
         for(int j = 0; j < 32; j++){
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

void printRoutingTable(){
    printf("Routing Table:\n");
    for(int i = 0; i < sizeof(routingTable)/80; i++){
            printf("Entry %d:\n", i);
            printf(" Chat Name: %s\n", routingTable[i].chatName);
            printf(" Address: %u.%u.%u.%u\n", (uint8_t)(routingTable[i].adress >> 24), (uint8_t)(routingTable[i].adress >> 16), (uint8_t)(routingTable[i].adress >> 8), (uint8_t)(routingTable[i].adress));
            printf(" Port: %u\n", routingTable[i].port);
            printf(" Next Chat Name: %s\n", routingTable[i].nextChatName);
            printf(" Next Address: %u.%u.%u.%u\n", (uint8_t)(routingTable[i].nextAdress >> 24), (uint8_t)(routingTable[i].nextAdress >> 16), (uint8_t)(routingTable[i].nextAdress >> 8), (uint8_t)(routingTable[i].nextAdress));
            printf(" Next Port: %u\n", routingTable[i].nextPort);
            printf(" Hop Count: %u\n", routingTable[i].hopCount);
    }
}


int main(int argc, char **argv){
    char* testName = "HalloGutenTag";
    initTable(testName, 1234567, 8000);
    uint8_t ergebnis[sizeof(routingTable)] = {0};
    tableToCharArray(ergebnis);
    printf("hallo5\n");
    printf("%d\n", sizeof(ergebnis));
    for(int i = 0; i < sizeof(ergebnis); i++){
    printf("%d", ergebnis[i]);
    }

    uint8_t testMessage[240] = {    /* ========= Eintrag 1 (Alice -> Bob) ========= */

    // Chat Name: "Alice" (32 B)
    65,108,105,99,101,
    0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,

    // Address: 192.168.1.10 (4 B)
    192,168,1,10,

    // Port: 5000 (2 B)
    19,136,

    // Next Chat Name: "Bob" (32 B)
    66,111,98,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,

    // Next Address: 10.0.0.5 (4 B)
    10,0,0,5,

    // Next Port: 6000 (2 B)
    23,112,

    // Hop Count: 3 (4 B)
    0,0,0,3,


    /* ========= Eintrag 2 (Bob -> Carol) ========= */

    // Chat Name: "Bob" (32 B)
    66,111,98,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,

    // Address: 10.0.0.5 (4 B)
    10,0,0,5,

    // Port: 6000 (2 B)
    23,112,

    // Next Chat Name: "Carol" (32 B)
    67,97,114,111,108,
    0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,

    // Next Address: 172.16.0.7 (4 B)
    172,16,0,7,

    // Next Port: 7000 (2 B)
    27,88,

    // Hop Count: 2 (4 B)
    0,0,0,2,


    /* ========= Eintrag 3 (Carol -> Dave) ========= */

    // Chat Name: "Carol" (32 B)
    67,97,114,111,108,
    0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,

    // Address: 172.16.0.7 (4 B)
    172,16,0,7,

    // Port: 7000 (2 B)
    27,88,

    // Next Chat Name: "Dave" (32 B)
    68,97,118,101,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,

    // Next Address: 192.168.0.20 (4 B)
    192,168,0,20,

    // Next Port: 8000 (2 B)
    31,64,

    // Hop Count: 1 (4 B)
    0,0,0,1};

    printf("hallo6\n");
    tableUpdate(testMessage, 240);
    printf("hallo6\n");
    tableToCharArray(ergebnis);
    printf("hallo6\n");
    printf("%d\n", sizeof(ergebnis));
    for(int i = 0; i < sizeof(ergebnis); i++){
    printf("%d", ergebnis[i]);
    }

    printRoutingTable();
    printf("Getting routing for Carol:\n");
    uint64_t adressAndPort = getRouting("Carol");
    if(adressAndPort != (uint64_t)-1){
        uint32_t adress = (uint32_t)(adressAndPort >> 32);
        uint16_t port = (uint16_t)(adressAndPort & 0xFFFFFFFF);
        printf(" Next Address: %u.%u.%u.%u\n", (uint8_t)(adress >> 24), (uint8_t)(adress >> 16), (uint8_t)(adress >> 8), (uint8_t)(adress));
        printf(" Next Port: %u\n", port);
    } else {
        printf(" No routing found for Carol\n");
    }


    //free(routingTable);
    //free(ergebnis);
    return 0;
}


