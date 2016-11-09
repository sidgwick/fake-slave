#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

#include "client.h"
#include "query.h"
#include "binlog.h"
#include "tools.h"
#include "debug.h"

#define DIG_PER_DEC1 9
#define SIGNED_NUMBER 1

// 从大端序的内存里向小端序整型里面读len个字节
int big_endian_number(const char *buf, char len)
{
    int num = 0;
    unsigned char rest = 4 - len;

    while (--len >= 0) {
        *((char *)&num + 3 - len) = *(buf + len);
    }

    return num >> (rest * 8);
}

// 根据binlog里面decimal的存储结构, 得到对应的数字
long int decimal_number_part(const char *buf, char len, int mask)
{
    int tmp;
    unsigned char comp = len % 4;
    long int number = 0;

    tmp = big_endian_number(buf, comp);
    number= tmp ^ mask;

    while (comp < len) {
        tmp = big_endian_number(buf + comp, 4);
        comp += 4;

        number *= 1000000000;
        number += tmp ^ mask;
    }

    return number;
}

// TODO: negative number
int decimal_number(const char *buf, unsigned char precision, unsigned char scale)
{
    int intdig2byte[DIG_PER_DEC1 + 1] = {0, 1, 1, 2, 2, 3, 3, 4, 4, 4};
    long int integral = 0;
    long int fractional = 0;
    unsigned char int_l = 0;
    unsigned char frac_l = 0;
    long int mask = (*buf & 0x80) ? 0 : -1;

    int_l = intdig2byte[(precision - scale) % DIG_PER_DEC1] + (precision - scale) / DIG_PER_DEC1 * 4;
    frac_l = intdig2byte[scale % DIG_PER_DEC1] + scale / DIG_PER_DEC1 * 4;

    char *nbuf = malloc(sizeof(char) * (int_l + frac_l));
    memcpy(nbuf, buf, int_l + frac_l);
    *nbuf ^= 0x80;

    integral = decimal_number_part(nbuf, int_l, mask);
    fractional = decimal_number_part(nbuf + int_l, frac_l, mask);

    free(nbuf);

    // 保存为浮点数会出现精度损失, 所以保存为字符串表示.
    char zero[10];
    int tmp = scale; // 需要前置多少个0
    long int frac = fractional;
    while (frac > 0) {
        frac /= 10;
        tmp--;
    }

    zero[tmp] = '\0';
    while (tmp-- > 0) {
        zero[tmp] = '0';
    }

    printf("Decimal: %c%ld.%s%ld\n", (mask ? '-' : '\0'), integral, zero, fractional);

    return int_l + frac_l;
}