#include "protocol_header.h"

void getName(char* dest, char* src) {
    int count = NAME_LEN;
    while (*src == 0) {
        src++;
        count--;
    }
    memcpy(dest, src, count);
    dest[NAME_LEN] = 0;
}

void setName(char* dest, const char* src) {
    memset(dest, 0, NAME_LEN);
    int len = strlen(src);
    memcpy(dest+NAME_LEN-len, src, len);
}