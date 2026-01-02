
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

typedef struct routingTableEntry {
    char chatName[32];
    uint32_t adress;
    uint16_t port;
    char nextChatName[32];
    uint32_t nextAdress;
    uint16_t nextPort;
    uint32_t hopCount;
} routingTableEntry;

routingTableEntry *routingTable; 


void initTable(char* ownName, int ownAdress, int ownPort){
    routingTable = calloc(1, sizeof(routingTableEntry));
    *routingTable[0].chatName = *ownName;
    routingTable[0].adress = ownAdress;
    routingTable[0].port = ownPort;
    routingTable[0].hopCount = 0;
}

void tabeUpdate(uint8_t* message){

}

int tableToCharArray(uint8_t* result){
    uint8_t* ergebnis = calloc(sizeof(routingTable), sizeof(char));
    for(int i = 0; i <= sizeof(routingTable)/80; i++){
         for(int j = 0; j< 32; j++){
            ergebnis[j+i*80] = (uint8_t)routingTable[i].chatName[j];
        }
    ergebnis[i*80+OFFSETADRESS] = (uint8_t)(routingTable[i].adress >> 24);
    ergebnis[i*80+OFFSETADRESS+1] = (uint8_t)(routingTable[i].adress >> 16);
    ergebnis[i*80+OFFSETADRESS+2] = (uint8_t)(routingTable[i].adress >> 8);
    ergebnis[i*80+OFFSETADRESS+3] = (uint8_t)(routingTable[i].adress);
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
}



