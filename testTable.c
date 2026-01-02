
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

routingTableEntry routingTable[10] = {0}; 


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
    memcpy(routingTable[1].chatName, ownName, strlen(ownName)+1);
    printf("hallo2\n");
    routingTable[1].adress = ownAdress;
    routingTable[1].port = ownPort;
    routingTable[1].hopCount = 0;
    printf("%d hallo\n", sizeof(routingTable));
}

void tableUpdate(uint8_t* message, int length){
    u_int8_t workingArray[length] = {0};
    for (size_t i = 0; i < length/OFFSETMESSAGECOUNT; i++)
    {
        workingArray = memcpy(workingArray, message + i*OFFSETMESSAGECOUNT, OFFSETMESSAGECOUNT);
        char* charName = memcpy(charName, workingArray + OFFSETCHATNAME, 32);
        if(!checkNameInTable(charName)){
            routingTableEntry newEntry;
            memcpy(newEntry.chatName, workingArray + OFFSETCHATNAME, 32);
            newEntry.adress = (uint32_t)(workingArray[OFFSETADRESS] << 24) | (uint32_t)(workingArray[OFFSETADRESS+1] << 16) | (uint32_t)(workingArray[OFFSETADRESS+2] << 8) | (uint32_t)(workingArray[OFFSETADRESS+3]);
            newEntry.port = (uint16_t)(workingArray[OFFSETPORT] << 8) | (uint16_t)(workingArray[OFFSETPORT+1]);
            memcpy(newEntry.nextChatName, workingArray + OFFSETNEXTCHATNAME, 32);
            newEntry.nextAdress = (uint32_t)(workingArray[OFFSETNEXTADRESS] << 24) | (uint32_t)(workingArray[OFFSETNEXTADRESS+1] << 16) | (uint32_t)(workingArray[OFFSETNEXTADRESS+2] << 8) | (uint32_t)(workingArray[OFFSETNEXTADRESS+3]);
            newEntry.nextPort = (uint16_t)(workingArray[OFFSETNEXTPORT] << 8) | (uint16_t)(workingArray[OFFSETNEXTPORT+1]);
            newEntry.hopCount = (uint32_t)(workingArray[OFFSETHOPCOUNT] << 24) | (uint32_t)(workingArray[OFFSETHOPCOUNT+1] << 16) | (uint32_t)(workingArray[OFFSETHOPCOUNT+2] << 8) | (uint32_t)(workingArray[OFFSETHOPCOUNT+3]);
            for(int j = 0; j < sizeof(routingTable)/80; j++){
                if(routingTable[j].hopCount == 0 && strcmp(routingTable[j].chatName, "") == 0){
                    routingTable[j] = newEntry;
                    break;
                }
            }
        }
    }
    
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

bool checkNameInTable(char* chatName){
    for(int i = 0; i < sizeof(routingTable)/80; i++){
        if(strcmp(routingTable[i].chatName, chatName) == 0){
            return true;
        }
    }
    return false;
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
    //free(routingTable);
    //free(ergebnis);
    return 0;
}


