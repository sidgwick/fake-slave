#ifndef BINARY_TYPE_H
#define BINARY_TYPE_H

#include "client.h"

// MySQL data type
#define MYSQL_TYPE_DECIMAL 0x00
#define MYSQL_TYPE_TINY 0x01
#define MYSQL_TYPE_SHORT 0x02
#define MYSQL_TYPE_LONG 0x03
#define MYSQL_TYPE_FLOAT 0x04
#define MYSQL_TYPE_DOUBLE 0x05
#define MYSQL_TYPE_NULL 0x06
#define MYSQL_TYPE_TIMESTAMP 0x07
#define MYSQL_TYPE_LONGLONG 0x08
#define MYSQL_TYPE_INT24 0x09
#define MYSQL_TYPE_DATE 0x0a
#define MYSQL_TYPE_TIME 0x0b
#define MYSQL_TYPE_DATETIME 0x0c
#define MYSQL_TYPE_YEAR 0x0d
#define MYSQL_TYPE_NEWDATE 0x0e
#define MYSQL_TYPE_VARCHAR 0x0f
#define MYSQL_TYPE_BIT 0x10
#define MYSQL_TYPE_TIMESTAMP2 0x11
#define MYSQL_TYPE_DATETIME2 0x12
#define MYSQL_TYPE_TIME2 0x13
#define MYSQL_TYPE_NEWDECIMAL 0xf6
#define MYSQL_TYPE_ENUM 0xf7
#define MYSQL_TYPE_SET 0xf8
#define MYSQL_TYPE_TINY_BLOB 0xf9
#define MYSQL_TYPE_MEDIUM_BLOB 0xfa
#define MYSQL_TYPE_LONG_BLOB 0xfb
#define MYSQL_TYPE_BLOB 0xfc
#define MYSQL_TYPE_VAR_STRING 0xfd
#define MYSQL_TYPE_STRING 0xfe
#define MYSQL_TYPE_GEOMETRY 0xff

typedef struct {
    uint16_t year;
    int8_t sign;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} bin_datetime2;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
} bin_date;

typedef struct {
    int8_t sign;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} bin_time;

typedef struct {
    uint8_t sign;
    uint8_t zeros;
    uint64_t integral;
    uint64_t fractional;
} bin_decimal;

bin_time read_mysql_time(const char *buf);
bin_time read_mysql_time2(const char *buf);
bin_date read_mysql_date(const char *buf);
bin_datetime2 read_mysql_datetime2(const char *buf);
int read_mysql_timestamp2(const char *buf);

int read_mysql_long(const char *buf);
int read_mysql_int24(const char *buf);
int read_mysql_tiny(const char *buf);
int read_mysql_short(const char *buf);
int read_mysql_longlong(const char *buf);
float read_mysql_float(const char *buf);
double read_mysql_double(const char *buf);
bin_decimal read_mysql_newdecimal(const char *buf, int *cursor, const char *meta);

int read_mysql_varchar(const char *buf, const char *meta);

#endif
