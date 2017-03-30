#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

#include "client.h"
#include "query.h"
#include "binary_type.h"
#include "binlog.h"
#include "tools.h"
#include "log.h"
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
bin_decimal decimal_number(const char *buf, unsigned char precision, unsigned char scale, int *cursor)
{
    int intdig2byte[DIG_PER_DEC1 + 1] = {0, 1, 1, 2, 2, 3, 3, 4, 4, 4};
    unsigned char int_l = 0;
    unsigned char frac_l = 0;
    long int mask = (*buf & 0x80) ? 0 : -1;
    bin_decimal value;

    int_l = intdig2byte[(precision - scale) % DIG_PER_DEC1] + (precision - scale) / DIG_PER_DEC1 * 4;
    frac_l = intdig2byte[scale % DIG_PER_DEC1] + scale / DIG_PER_DEC1 * 4;

    char *nbuf = malloc(sizeof(char) * (int_l + frac_l));
    memcpy(nbuf, buf, int_l + frac_l);
    *nbuf ^= 0x80;

    value.integral = decimal_number_part(nbuf, int_l, mask);
    value.fractional = decimal_number_part(nbuf + int_l, frac_l, mask);

    free(nbuf);

    // 需要前置多少个0
    long int frac = value.fractional;
    while (frac > 0) {
        frac /= 10;
        value.zeros++;
    }

    *cursor = int_l + frac_l;

    value.sign = mask ? '-' : '\0';
    // return decimal_as_string(mask, integral, zero, fractional);
    return value;
}

char *decimal_as_string(bin_decimal number)
{
    char tmp_buf[100];
    char zero[10];
    int tmp = number.zeros;

    zero[tmp] = '\0';
    while (tmp-- > 0) {
        zero[tmp] = '0';
    }

    sprintf(tmp_buf, "%c%ld.%s%ld", (number.sign ? '-' : '\0'), number.integral, zero, number.fractional);

    char *tmpp;
    tmpp = malloc(sizeof(char) * strlen(tmp_buf) + 1);
    strcpy(tmpp, tmp_buf);

    return tmpp;
}

void display_decimal(bin_decimal number)
{
    char zero[10];
    int tmp = number.zeros;

    zero[tmp] = '\0';
    while (tmp-- > 0) {
        zero[tmp] = '0';
    }

    logger(LOG_DEBUG, "DECIMAL: %c%ld.%s%ld\n", (number.sign ? '-' : '\0'), number.integral, zero, number.fractional);
}
