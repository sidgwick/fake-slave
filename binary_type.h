#ifndef BINARY_TYPE_H
#define BINART_TYPE_H


#include "binlog.h"
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

int read_time(const char *buf);
int read_time2(const char *buf);
int read_date(const char *buf);
int read_datetime2(const char *buf);
int read_timestamp2(const char *buf);
int read_longint(const char *buf);

#endif
