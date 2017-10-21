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
        printf("%.*s\n", (int)message_data.sz, message_data.data);
    }

    return 0;
}
