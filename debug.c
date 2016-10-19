#include <stdio.h>

int print_memory(char *mem, int len)
{
    int i = 0;

    while (i < len) {
        printf("%02X ", (unsigned char)mem[i++]);

        if (i % 8 == 0) {
            puts("    ");
        } else if (i % 16 == 0) {
            putchar('\n');
        }
    }

    return i - 1;
}
