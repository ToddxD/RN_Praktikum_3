#ifndef PROTOCOL_HEADER_H
#define PROTOCOL_HEADER_H

#include <stdint.h>
#include <string.h>

#define NAME_LEN 32

#define TYPE_LOGIN 1
#define TYPE_CHAT 2
#define TYPE_LOGOUT 3
#define TYPE_ROUTE 4
#define TYPE_HEART 5
#define TYPE_HEARTRESPONSE 6
#define TYPE_ERROR 7

#define MSG_SIZE 1500

typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t version;
    char sendername[NAME_LEN];
    char targetname[NAME_LEN];
} Header;

void getName(char* dest, const char* src);

void setName(char* dest, const char* src);

void protocol_create_header(Header* header, const char* sendername, const char* targetname,
                            uint8_t type);

size_t msglen(const char* msg);

#endif  // PROTOCOL_HEADER_H