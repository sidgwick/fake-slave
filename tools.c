#include <string.h>
#include <stdint.h>
#include <stdlib.h>

int generate_length_encode_number(const char *buf, int *length)
{
    int tmp = 0;
    uint64_t data = 0;
    *length = 0; // 置0, 防止函数外面错误的增加长度

    tmp = *buf++;
    if (tmp < 0xFB) { // tmp never be 0xfb(251)
        // one byte number.
        data = tmp;
        *length = 1;
    } else {
        if (tmp == 0xFC) {
            // 2 bytes
            tmp = 2;
        } else if (tmp == 0xFD) {
            // 3 bytes
            tmp = 3;
        } else if (tmp == 0xFE) {
            // 4 bytes
            tmp = 8;
        }
        memcpy(&data, buf, tmp);
        *length = tmp + 1;
    }

    return data;
}


char *generate_length_encode_string(const char *buf, int *length)
{
    uint8_t len = *buf++;
    char *string;

    *length = 0;
    string = malloc(sizeof(char) * len + 1); // append a '\0' byte.
    memset(string, 0, len + 1);
    memcpy(string, buf, len);

    *length = len + 1;

    return string;
}
