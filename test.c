#include <stdlib.h>

typedef struct __attribute__((packed)) {
	u_int8_t type;
	u_int8_t version;
    char sendername[32];
    char targetname[32];
} Header;


void getName(char* dest, char* src){
    int count = 32;
    while (*src == 0) {
        src++;
        count--;
    }
    memcpy(dest, src, count);
}

int main(void) {

    char read_buf[] = {
        1, 
        1, 
        'x',
        'b',
        'c', 
        'a',
        'a',
        'b',
        'c', 
        'a',
        'a',
        'b',
        'c', 
        'a',
        'a',
        'b',
        'c', 
        'a',
        'a',
        'b',
        'c', 
        'a',
        'a',
        'b',
        'c', 
        'a',
        'b',
        'c', 
        'a',
        'b',
        'c', 
        'a',
        'b',
        'z', 
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        'd',
        'e',
        'f'
    };
    Header header;
    memcpy(&header, &read_buf, sizeof(Header));

    printf("Type: %d\n", header.type);
    printf("Version: %d\n", header.version);

    char sender[33];
    char target[33];
    getName(sender, header.sendername);
    getName(target, header.targetname);

    printf("Sender Name: %s\n", sender);
    printf("Target Name: %s\n", target);

    return 0;
}
