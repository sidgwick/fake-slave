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
#include "log.h"
#include "debug.h"

bin_time read_mysql_time(const char *buf)
{
    /*
     * time = self.packet.read_uint24()
     * hours=int(time / 10000),
     * minutes=int((time % 10000) / 100),
     * seconds=int(time % 100))
     */
    uint32_t tmp = 0;
    bin_time value;

    tmp = read_uint3(buf);
    tmp = ntohl(tmp);

    value.hour = tmp / 10000;
    value.minute = ((tmp % 10000) / 100);
    value.second = tmp % 100;

    logger(LOG_DEBUG, "TIME: %d:%d:%d\n", value.hour, value.minute, value.second);

    return value;
}

bin_time read_mysql_time2(const char *buf)
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
    bin_time value;

    tmp = read_uint3(buf);

    // 确定符号位
    if (*(char *)&tmp & 0x80) {
        value.sign = 1;
    } else {
        value.sign = -1;
    }

    tmp = ntohl(tmp);
    value.hour = ((tmp >> 20) & 0x00000FFF);
    value.minute = ((tmp >> 14) & 0x0000003F);
    value.second = ((tmp >> 8) & 0x0000003F);

    logger(LOG_DEBUG, "TIME2: %c%d:%d:%d\n", (value.sign == 1) ? '+' : '-', value.hour, value.minute, value.second);

    return value;
}

bin_date read_mysql_date(const char *buf)
{
    /*
     * time = self.packet.read_uint24()
     *
     * year = (time & ((1 << 15) - 1) << 9) >> 9
     * month = (time & ((1 << 4) - 1) << 5) >> 5
     * day = (time & ((1 << 5) - 1))
     */
    uint32_t tmp = 0;
    bin_date value;

    tmp = read_uint3(buf);

    value.year = (tmp & ((1 << 15) - 1) << 9) >> 9;
    value.month = (tmp & ((1 << 4) - 1) << 5) >> 5;
    value.day = (tmp & ((1 << 5) - 1));

    logger(LOG_DEBUG, "DATE: %4u-%02u-%02u\n", value.year, value.month, value.day);

    return value;
}

bin_datetime2 read_mysql_datetime2(const char *buf)
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
    bin_datetime2 value;
    uint32_t tmp = 0;

    // 计算得到数据的符号
    tmp = read_uint3(buf);
    value.sign = *(int8_t *)&tmp;
    if (value.sign & 0x80) {
        value.sign = 1;
    } else {
        value.sign = -1;
    }

    // 计算 year * 13 + month
    tmp = tmp & 0x00C0FF7F;
    tmp = ntohl(tmp);
    tmp = tmp >> 14; // 没有用的垃圾位数清楚掉

    value.year = tmp / 13;
    value.month = tmp % 13;

    // 计算 day
    value.day = *(buf + 2);
    value.day = (value.day & 0x3E) >> 1;

    tmp = read_uint4(buf + 1);
    tmp = ntohl(tmp);
    value.hour = (tmp >> 12) & 0x0000001F;
    value.minute = (tmp >> 6) & 0x0000003F;
    value.second = tmp & 0x0000003F;

    logger(LOG_DEBUG, "DATETIME2: %c%04u-%02u-%02u %02u:%02u:%02u\n", (value.sign == 1) ? '+' : '-', value.year, value.month, value.day, value.hour, value.minute, value.second);

    return value;
}

// TIMESTAMP: A four-byte integer representing seconds UTC since the epoch ('1970-01-01 00:00:00' UTC)
// TIMESTAMP encoding for nonfractional part: Same as before 5.6.4, except big endian rather than little endian
int read_mysql_timestamp2(const char *buf)
{
    uint32_t value = 0;

    // big endian
    value = read_int4(buf);
    value = ntohl(value);

    logger(LOG_DEBUG, "TIMESTAMP2: %d\n", value);

    return value;
}

int read_mysql_long(const char *buf)
{
    // long int.
    int iv = 0;
    iv = read_int4(buf);

    logger(LOG_DEBUG, "LONG %d\n", iv);

    return iv;
}

int read_mysql_int24(const char *buf)
{
    // long int.
    int iv = 0;
    iv = read_int3(buf);

    logger(LOG_DEBUG, "INT24 %d\n", iv);

    return iv;
}

int read_mysql_tiny(const char *buf)
{
    logger(LOG_DEBUG, "TINY INT: %d\n", *buf);

    return *buf;
}

int read_mysql_short(const char *buf)
{
    uint16_t num = 0;

    num = read_uint2(buf);
    logger(LOG_DEBUG, "SHORT INT: %d\n", num);

    return 0;
}

int read_mysql_longlong(const char *buf)
{
    int64_t num = 0;

    num = read_int8(buf);
    logger(LOG_DEBUG, "LONG LONG INT: %ld\n", num);

    return 0;
}

bin_decimal read_mysql_newdecimal(const char *buf, int *cursor, const char *meta)
{
    // 1st byte is precision, 2nd byte is scale.
    unsigned char precision = *meta;
    unsigned char scale = *(meta + 1);

    return decimal_number(buf, precision, scale, cursor);
}

float read_mysql_float(const char *buf)
{
    float f;

    f = read_float(buf);
    logger(LOG_DEBUG, "FLOAT: %f\n", f);

    return f;
}

double read_mysql_double(const char *buf)
{
    double d;

    d = read_double(buf);
    logger(LOG_DEBUG, "DOUBLE %lf\n", d);

    return d;
}

bin_varchar read_mysql_varchar(const char *buf, int *cursor, const char *meta)
{
    bin_varchar value;

    // 1st byte is type, 2nd byte is length.
    unsigned char prefix_num_length = *(meta + 1);

    if (prefix_num_length > 0) {
        // length of length encode string #mysql bug 37426#
        memcpy(&value.len, buf, prefix_num_length);

        value.s = malloc(sizeof(char) * value.len);
        memcpy(value.s, buf + prefix_num_length, value.len);
        value.len += prefix_num_length;
    } else {
        value.s = get_length_encode_string(buf, &value.len);
    }

    logger(LOG_DEBUG, "VARCHAR %s(%d-%d)\n", value.s, prefix_num_length, value.len);

    return value;
}
