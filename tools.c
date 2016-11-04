#include <string.h>
#include <stdint.h>
#include <stdlib.h>

int get_length_encode_number(const char *buf, int *length)
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


// assume length less than 251, just for now
char *get_length_encode_string(const char *buf, int *length)
{
    char *string;
    int prefix_num_len;
    int len = get_length_encode_number(buf, &prefix_num_len);
    // prefix_num_len++;

    string = malloc(sizeof(char) * len + 1); // append a '\0' byte.
    memset(string, 0, len + 1);
    memcpy(string, buf + prefix_num_len, len);

    *length = prefix_num_len + len + 1;

    return string;
}

// assume length less than 251, just for now.
int set_length_encode_string(char *buf, char *string, int *length)
{
    *length = strlen(string);
    memcpy(buf, length, 1);
    memcpy(buf + 1, string, *length);
    *length += 1;

    return *length;
}

int set_length_encode_integer(char *buf, int num, int *length)
{
    // do some thing.
    return 0;
}

int int_length(int a)
{
    int i = 1;
    while ((a = a % 10) > 10) {
        i++;
    }

    return i;
}
