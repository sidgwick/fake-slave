#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>

#include "client.h"
#include "query.h"
#include "binary_type.h"
#include "rows_event.h"
#include "binlog.h"
#include "decimal.h"
#include "tools.h"
#include "debug.h"

int read_time(const char *buf)
{
    /*
     * time = self.packet.read_uint24()
     * hours=int(time / 10000),
     * minutes=int((time % 10000) / 100),
     * seconds=int(time % 100))
     */
    uint32_t tmp = 0;
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;

    tmp = read_uint3(buf);
    tmp = ntohl(tmp);

    hour = tmp / 10000;
    minute = ((tmp % 10000) / 100);
    second = tmp % 100;

    printf("TIME: %d:%d:%d\n", hour, minute, second);
    
    return 0;
}

int read_time2(const char *buf)
{
    /*
     * 1 bit sign    (1= non-negative, 0= negative)
     * 1 bit unused  (reserved for future extensions)
     * 10 bits hour   (0-838)
     * 6 bits minute (0-59)
     * 6 bits second (0-59)
     * ---------------------
     * 24 bits = 3 bytes
     */
    uint32_t tmp = 0;
    int8_t sign = 0;
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;

    tmp = read_uint3(buf);

    // 确定符号位
    if (*(char *)&tmp & 0x80) {
        sign = 1;
    } else {
        sign = -1;
    }

    tmp = ntohl(tmp);
    hour = ((tmp >> 20) & 0x00000FFF);
    minute = ((tmp >> 14) & 0x0000003F);
    second = ((tmp >> 8) & 0x0000003F);

    printf("TIME2: %c%d:%d:%d\n", (sign == 1) ? '+' : '-', hour, minute, second);
    
    return 0;
}

int read_date(const char *buf)
{
    /*
     * time = self.packet.read_uint24()
     *
     * year = (time & ((1 << 15) - 1) << 9) >> 9
     * month = (time & ((1 << 4) - 1) << 5) >> 5
     * day = (time & ((1 << 5) - 1))
     */
    uint32_t tmp = 0;

    tmp = read_uint3(buf);
    uint16_t year = 0;
    uint8_t month = 0;
    uint8_t day = 0;

    year = (tmp & ((1 << 15) - 1) << 9) >> 9;
    month = (tmp & ((1 << 4) - 1) << 5) >> 5;
    day = (tmp & ((1 << 5) - 1));

    printf("DATE: %4u-%02u-%02u\n", year, month, day);
    
    return 0;
}

int read_datetime2(const char *buf)
{
    /*
     * 1 bit  sign            (1= non-negative, 0= negative)
     * 17 bits year*13+month  (year 0-9999, month 0-12)
     * 5 bits day             (0-31)
     * 5 bits hour            (0-23)
     * 6 bits minute          (0-59)
     * 6 bits second          (0-59)
     * ---------------------------
     * 40 bits = 5 bytes
     *
     * TXT: 2017-01-05 13:25:46
     * HEX: 99 9B 8A D6 6E
     * BIN: 1^0011001 10011011 10^00101^0 1101^0110 01^101110
     */

    // 这部分的位移操作是为了修正数据在内存中的'bits'到相应的正确位置
    int8_t sign = 0;

    uint32_t tmp = 0;
    uint16_t year = 0;
    uint8_t month = 0;
    uint8_t day = 0;
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;

    // 计算得到数据的符号
    tmp = read_uint3(buf);
    sign = *(int8_t *)&tmp;
    if (sign & 0x80) {
        sign = 1;
    } else {
        sign = -1;
    }

    // 计算 year * 13 + month
    tmp = tmp & 0x00C0FF7F;
    tmp = ntohl(tmp);
    tmp = tmp >> 14; // 没有用的垃圾位数清楚掉

    year = tmp / 13;
    month = tmp % 13;

    // 计算 day
    day = *(buf + 2);
    day = (day & 0x3E) >> 1;

    tmp = read_uint4(buf + 1);
    tmp = ntohl(tmp);
    hour = (tmp >> 12) & 0x0000001F;
    minute = (tmp >> 6) & 0x0000003F;
    second = tmp & 0x0000003F;


    printf("DATETIME2: %c%04u-%02u-%02u %02u:%02u:%02u\n", (sign == 1) ? '+' : '-', year, month, day, hour, minute, second);
    
    return 0;
}

// TIMESTAMP: A four-byte integer representing seconds UTC since the epoch ('1970-01-01 00:00:00' UTC)
// TIMESTAMP encoding for nonfractional part: Same as before 5.6.4, except big endian rather than little endian
int read_timestamp2(const char *buf)
{
    uint32_t value = 0;

    // big endian
    value = read_int4(buf);
    value = ntohl(value);

    printf("TIMESTAMP2: %u\n", value);
    
    return 0;
}

int read_longint(const char *buf)
{
    // long int.
    int iv = 0;
    iv = read_int4(buf);
    iv = ntohl(iv);

    printf("LONG %d\n", iv);
    
    return 0;
}