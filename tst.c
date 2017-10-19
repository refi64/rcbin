#include <rcbin.h>

#include <stdlib.h>
#include <stdio.h>


RCBIN_IMPORT(abc)


int main() {
    if (!rcbin_init()) {
        puts("Failed to initialize rcbin.");
    }

    rcbin_entry message_data = abc();
    if (message_data.data == NULL) {
        puts("Failed to retrieve message.");
    } else {
        char* s = malloc(message_data.sz+1);
        memcpy(s, message_data.data, message_data.sz);
        s[message_data.sz] = 0;
        puts(s);
        free(s);
    }

    return 0;
}
