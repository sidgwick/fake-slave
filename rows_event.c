#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>

#include "client.h"
#include "query.h"
#include "binlog.h"
#include "decimal.h"
#include "tools.h"
#include "debug.h"

// 取某个字段对应的 column_def(2 bytes)
char *get_column_meta_def(struct table_map_event ev, int col_num)
{
    unsigned char column_def;
    char *meta = ev.column_meta_def;
    int cursor = 0;
    int meta_len = 0;

    for (int i = 0; i < ev.column_count; i++) {
        column_def = *(ev.column_def + i);

        // skip the column meta def before the target
        switch (column_def) {
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VAR_STRING:
        case MYSQL_TYPE_VARCHAR:
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL:
        case MYSQL_TYPE_ENUM:
        case MYSQL_TYPE_SET:
            meta_len = 2;
            break;
        case MYSQL_TYPE_BLOB:
        case MYSQL_TYPE_DOUBLE:
        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_TIME2:
        case MYSQL_TYPE_DATETIME2:
        case MYSQL_TYPE_TIMESTAMP2:
            meta_len = 1;
            break;
        case MYSQL_TYPE_BIT:
        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_DATETIME:
        case MYSQL_TYPE_TIMESTAMP:
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_LONGLONG:
        case MYSQL_TYPE_TIME:
        default:
            meta_len = 0;
            break;
        }

        if (col_num == i) {
            // target column
            if (meta_len > 0) {
                return meta + cursor;
            }

            break;
        }

        cursor += meta_len;
    }

    return NULL;
}

// 取 row 里面对应某个 column_id 的数据
int get_column_val(struct rows_event *ev, int column_id, const char *buf)
{
    int cursor = 0;

    unsigned char column_def = *(ev->table_map.column_def + column_id);
    printf("\e[43;31mCOLUMN DEF\e[0m: %02x, ID: %d value: ", column_def, column_id);
    print_memory(buf, 20);

    switch (column_def) {
    case MYSQL_TYPE_LONG:
        {
            // long int.
            int iv = 0;
            iv = read_int4(buf);
            cursor += 4;

            printf("LONG %d\n", iv);
        }
        break;
    case MYSQL_TYPE_INT24:
        {
            // long int.
            int iv = 0;
            iv = read_int3(buf);
            cursor += 3;

            printf("MEDIUM %d\n", iv);
        }
        break;
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL:
        {
            // column meta info
            // 1st byte is precision, 2nd byte is scale.
            char *meta = get_column_meta_def(ev->table_map, column_id);
            unsigned char precision = *meta;
            unsigned char scale = *(meta + 1);

            cursor += decimal_number(buf + cursor, precision, scale);
        }
        break;
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_ENUM:
    case MYSQL_TYPE_SET:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_BLOB:
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_GEOMETRY:
    case MYSQL_TYPE_BIT:
        {
            // varchar
            int tmp = 0;
            char *val;

            // column meta info
            char *meta = get_column_meta_def(ev->table_map, column_id);
            unsigned char prefix_num_length = *(meta + 1); // 1st byte is type, 2nd byte is length.

            if (prefix_num_length > 0) {
                // length of length encode string #mysql bug 37426#
                memcpy(&tmp, buf, prefix_num_length);
                cursor += prefix_num_length;

                val = malloc(sizeof(char) * tmp);
                memcpy(val, buf + cursor, tmp);
                cursor += tmp;
            } else {
                val = get_length_encode_string(buf + cursor, &tmp);
                cursor += tmp;
            }

            printf("VARCHAR %s(%d-%d)\n", val, prefix_num_length, tmp);
        }
        break;
    case MYSQL_TYPE_TINY:
        {
            char num = *(buf + cursor++);
            printf("TINY INT: %d\n", num);
        }
        break;
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_YEAR:
        {
            int16_t num = 0;
            num = read_int2(buf + cursor);
            cursor += 2;
            printf("SHORT INT: %d\n", num);
        }
        break;
    case MYSQL_TYPE_LONGLONG:
        {
            int64_t num = 0;
            num = read_int8(buf + cursor);
            cursor += 8;
            printf("LONG LONG INT: %ld\n", num);
        }
        break;
    case MYSQL_TYPE_DOUBLE:
        {
            double d;
            d = read_int8(buf + cursor);
            cursor += 8;
            printf("DOUBLE %lf\n", d);
        }
        break;
    case MYSQL_TYPE_FLOAT:
        {
            float f;
            f = read_int4(buf + cursor);
            cursor += 4;
            printf("FLOAT: %f\n", f);
        }
        break;
    case MYSQL_TYPE_TIMESTAMP2:
        {
            uint32_t value = 0;
            uint32_t nano = 0;
            uint8_t nano_len = 0;

            // column meta info
            char *meta = get_column_meta_def(ev->table_map, column_id);
            nano_len = (*meta + 1) / 2;

            // 此处小端序? 我擦
            value = read_int4(buf + cursor);
            cursor += 4;
            value = ntohl(value);
            memcpy(&nano, buf + cursor, nano_len);
            cursor += nano_len;

            printf("TIMESTAMP2: %u.%u\n", value, nano);
        }
        break;
    case MYSQL_TYPE_DATETIME2:
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
            tmp = read_uint3(buf + cursor);
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

            tmp = read_uint4(buf + cursor + 1);
            tmp = ntohl(tmp);
            hour = (tmp >> 12) & 0x0000001F;
            minute = (tmp >> 6) & 0x0000003F;
            second = tmp & 0x0000003F;

            cursor += 5;
            printf("DATETIME2: %c%04u-%02u-%02u %02u:%02u:%02u\n", (sign == 1) ? '+' : '-', year, month, day, hour, minute, second);
        }
        break;
    case MYSQL_TYPE_DATE:
        {
            /*
             * time = self.packet.read_uint24()
             *
             * year = (time & ((1 << 15) - 1) << 9) >> 9
             * month = (time & ((1 << 4) - 1) << 5) >> 5
             * day = (time & ((1 << 5) - 1))
             */
            uint32_t tmp = 0;

            tmp = read_uint3(buf + cursor);
            uint16_t year = 0;
            uint8_t month = 0;
            uint8_t day = 0;

            year = (tmp & ((1 << 15) - 1) << 9) >> 9;
            month = (tmp & ((1 << 4) - 1) << 5) >> 5;
            day = (tmp & ((1 << 5) - 1));

            cursor += 3;
            printf("DATE: %4u-%02u-%02u\n", year, month, day);
        }
        break;
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:
        {
            uint8_t len;
            uint16_t year = 0;
            uint8_t month = 0;
            uint8_t day = 0;
            uint8_t hour = 0;
            uint8_t minute = 0;
            uint8_t second = 0;
            uint32_t micro_second = 0;

            len = *(buf + cursor++);
            if (len > 0) {
                year = read_uint2(buf + cursor);
                cursor += 2;
                month = *(buf + cursor++);
                day = *(buf + cursor++);
            }
            if (len > 4) {
                hour = *(buf + cursor++);
                minute = *(buf + cursor++);
                second = *(buf + cursor++);
            }
            if (len > 7) {
                micro_second = read_uint4(buf + cursor);
                cursor += 4;
            }

            printf("DATETIME(%d): %u-%u-%u %u:%u:%u.%u\n", len, year, month, day, hour, minute, second, micro_second);
        }
        break;
    case MYSQL_TYPE_TIME:
        {
            /*
               def __read_time(self):
               time = self.packet.read_uint24()
               date = datetime.timedelta(
               hours=int(time / 10000),
               minutes=int((time % 10000) / 100),
               seconds=int(time % 100))
               return date
               */
        }
        break;
    case MYSQL_TYPE_TIME2:
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

            tmp = read_uint3(buf + cursor);

            // 确定符号位
            if (*(char *)&tmp & 0x80) {
                sign = 1;
            } else {
                sign = -1;
            }

            // 计算小时数
            tmp = ntohl(tmp);
            hour = ((tmp >> 20) & 0x00000FFF);
            minute = ((tmp >> 14) & 0x0000003F);
            second = ((tmp >> 8) & 0x0000003F);

            cursor += 3;
            printf("TIME: %c %d:%d:%d\n", (sign == 1) ? '+' : '-', hour, minute, second);
        }
        break;
    case MYSQL_TYPE_NULL:
        {
        }
        break;
    default:
        printf("Other type\n");
    }

    return cursor;
}

// 从 rows event 里面循环拿到 row 数据
int fetch_a_row(struct rows_event *ev, const char *buf)
{
    int cursor = 0;
    int tmp = 0;
    static int update_row = 1;

    // string.var_len       nul-bitmap, length (bits set in 'columns-present-bitmap1'+7)/8
    // string.var_len       value of each field as defined in table-map
    //   if UPDATE_ROWS_EVENTv1 or v2 {
    // string.var_len       nul-bitmap, length (bits set in 'columns-present-bitmap2'+7)/8
    // string.var_len       value of each field as defined in table-map
    //   }
    tmp = (ev->column_count + 7) / 8;
    char *null_bitmap = malloc(sizeof(char) * tmp);
    memcpy(null_bitmap, buf + cursor, tmp);
    cursor += tmp;

    printf("Bit map:"); print_memory(null_bitmap, tmp);

    // loop to get column value
    for (int i = 0; i < ev->column_count; i++) {
        cursor += get_column_val(ev, i, buf + cursor);
    }

    // UPDATE NEW VALUES
    if (ev->header.event_type == UPDATE_ROWS_EVENTv1 && (++update_row % 2 == 0)) {
        printf("Update new row:\n");
        cursor += fetch_a_row(ev, buf + cursor);
    }

    return cursor;
}

// ROWS EVENT
// TODO: malloc
int rows_event(struct rows_event *ev, const char *buf)
{
    int post_header_length = get_post_header_length(ev->header.event_type);
    int cursor = 0;
    int tmp;

    if (post_header_length == 6) {
        ev->table_id = read_int4(buf + cursor);
        cursor += 4;
    } else {
        ev->table_id = read_int6(buf + cursor);
        cursor += 6;
    }

    ev->flags = read_int2(buf + cursor);
    cursor += 2;

    // body
    ev->column_count = get_length_encode_number(buf + cursor, &tmp);
    cursor += tmp;

    tmp = (ev->column_count + 7) / 8;
    ev->columns_present_bitmap1 = malloc(sizeof(char) * tmp);
    memcpy(ev->columns_present_bitmap1, buf + cursor, tmp);
    cursor += tmp;

    if (ev->header.event_type == UPDATE_ROWS_EVENTv1) {
        ev->columns_present_bitmap2 = malloc(sizeof(char) * tmp);
        memcpy(ev->columns_present_bitmap2, buf + cursor, tmp);
        cursor += tmp;
    }

#ifdef DEBUG
    switch (ev->header.event_type) {
    case WRITE_ROWS_EVENTv0:
        printf("WRITE_ROWS_EVENTv0, Header length: %02X\n", get_post_header_length(WRITE_ROWS_EVENTv0));
        break;
    case UPDATE_ROWS_EVENTv0:
        printf("UPDATE_ROWS_EVENTv0, Header length: %02X\n", get_post_header_length(UPDATE_ROWS_EVENTv0));
        break;
    case DELETE_ROWS_EVENTv0:
        printf("DELETE_ROWS_EVENTv0, Header length: %02X\n", get_post_header_length(DELETE_ROWS_EVENTv0));
        break;
    case WRITE_ROWS_EVENTv1:
        printf("WRITE_ROWS_EVENTv1, Header length: %02X\n", get_post_header_length(WRITE_ROWS_EVENTv1));
        break;
    case UPDATE_ROWS_EVENTv1:
        printf("UPDATE_ROWS_EVENTv1, Header length: %02X\n", get_post_header_length(UPDATE_ROWS_EVENTv1));
        break;
    case DELETE_ROWS_EVENTv1:
        printf("DELETE_ROWS_EVENTv1, Header length: %02X\n", get_post_header_length(DELETE_ROWS_EVENTv1));
        break;
    case WRITE_ROWS_EVENTv2:
        printf("WRITE_ROWS_EVENTv2, Header length: %02X\n", get_post_header_length(WRITE_ROWS_EVENTv2));
        break;
    case UPDATE_ROWS_EVENTv2:
        printf("UPDATE_ROWS_EVENTv2, Header length: %02X\n", get_post_header_length(UPDATE_ROWS_EVENTv2));
        break;
    case DELETE_ROWS_EVENTv2:
        printf("DELETE_ROWS_EVENTv2, Header length: %02X\n", get_post_header_length(DELETE_ROWS_EVENTv2));
        break;
    }

    printf("Table id: %ld\n", ev->table_id);
    printf("Flags: ");print_memory((char *)&ev->flags, 2);
    printf("Column count: %d\n", ev->column_count);
#endif

    // rows.
    while (cursor < (ev->header.event_size - 19)) {
        cursor += fetch_a_row(ev, buf + cursor);
    }

    return 0;
}
