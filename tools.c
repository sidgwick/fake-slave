#include <string.h>
#include <stdint.h>
#include <stdlib.h>

// 读取2位整型
int16_t read_int2(const char *buf)
{
    return *(int16_t *)buf;
}

// 读取3位整型
int32_t read_int3(const char *buf)
{
    int32_t tmp = 0;

    memcpy(&tmp, buf, 3);

    // 如果 buf 里面存储的是 ABCD, 那么, 此时 tmp 里面存储的是 ABC0
    // 由于机器是小端序的, 所以这个数字应该理解成: 0CBA
    // 于是我们要检测 00C0 与 00(80)0 的与, 以判定是否为负数, 如果是负数, tmp 里面
    // 未能填充的 0 位将会带来错误, 就需要把它手动置为 1
    // if number is negative
    if (tmp & 0x00800000) {
        // 负数, 熟悉下负数在内存里面的表示方法就知道这里为什么要这样写了.
        tmp |= 0xFF000000;
    }

    return tmp;
}

// 读取一个4位整型
int32_t read_int4(const char *buf)
{
    return *(int32_t *)buf;
}

// 读取6位整型
int64_t read_int6(const char *buf)
{
    int64_t tmp = 0;

    memcpy(&tmp, buf + cursor, 6);

    return tmp;
}

// 读取8位整型
int64_t read_int8(const char *buf)
{
    return *(int64_t *)buf;
}

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
    while ((a /= 10)) {
        i++;
    }

    return i;
}
