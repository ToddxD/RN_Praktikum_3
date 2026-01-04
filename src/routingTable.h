#ifndef ROUTINGTABLE
#define ROUTINGTABLE

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


void initTable(char* ownName, int ownAdress, int ownPort);

int checkNameInTable(char* chatName);

void tableUpdate(uint8_t* message, int length);

uint64_t getRouting(char* chatName);

int deleteFromTable(char* chatName);

void tableToCharArray(uint8_t* ergebnis);

#endif