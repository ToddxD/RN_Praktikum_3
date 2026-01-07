#include "protocol_header.h"

#include <string.h>

void getName(char* dest, char* src) {
    int count = NAME_LEN;
    for (int i = 0; i < NAME_LEN && *src == 0; i++) {
        src++;
        count--;
    }
    memcpy(dest, src, count);
    dest[NAME_LEN] = 0;
}

void setName(char* dest, const char* src) {
    memset(dest, 0, NAME_LEN);
    int len = strlen(src);
    memcpy(dest + NAME_LEN - len, src, len);
}

void protocol_create_header(Header* header, const char* sendername, const char* targetname,
                            uint8_t type) {
    memset(header, 0, sizeof(Header));
    setName(header->sendername, sendername);
    setName(header->targetname, targetname);
    header->type = type;
}

size_t msglen(const char* msg) {
    size_t len = 0;
    while (msg[len] != '\004' && len < MSG_SIZE - 1) {
        len++;
    }
    return len + 1;
}