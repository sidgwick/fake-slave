#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>

#include "client.h"
#include "query.h"
#include "binary_type.h"
#include "binlog.h"
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
    char *meta = NULL;
    field_val va;
    field_val *v = &va;

    unsigned char column_def = *(ev->table_map.column_def + column_id);
    printf("DEF: %02x, ID: %d, VALUE: ", column_def, column_id);
    // print_memory(buf, 20);

    v->type = column_def;
    switch (column_def) {
    case MYSQL_TYPE_LONG:
        v->val.integer = read_mysql_long(buf + cursor);
        cursor += 4;
        break;
    case MYSQL_TYPE_INT24:
        v->val.integer = read_mysql_int24(buf + cursor);
        cursor += 3;
        break;
    case MYSQL_TYPE_NEWDECIMAL:
        meta = get_column_meta_def(ev->table_map, column_id);
        v->val.decimal = read_mysql_newdecimal(buf + cursor, meta, &cursor);
        break;
    case MYSQL_TYPE_VARCHAR:
        meta = get_column_meta_def(ev->table_map, column_id);
        cursor += read_mysql_varchar(buf + cursor, meta);
        break;
    case MYSQL_TYPE_TINY:
        read_mysql_tiny(buf + cursor++);
        break;
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_YEAR:
        read_mysql_short(buf + cursor);
        cursor += 2;
        break;
    case MYSQL_TYPE_LONGLONG:
        read_mysql_longlong(buf + cursor);
        cursor += 8;
        break;
    case MYSQL_TYPE_DOUBLE:
        read_mysql_double(buf + cursor);
        cursor += 8;
        break;
    case MYSQL_TYPE_FLOAT:
        read_mysql_float(buf + cursor);
        cursor += 4;
        break;
    case MYSQL_TYPE_TIMESTAMP2:
        read_mysql_timestamp2(buf + cursor);
        cursor += 4;
        break;
    case MYSQL_TYPE_DATETIME2:
        read_mysql_datetime2(buf + cursor);
        cursor += 5;
        break;
    case MYSQL_TYPE_DATE:
        read_mysql_date(buf + cursor);
        cursor += 3;
        break;
    case MYSQL_TYPE_TIME:
        read_mysql_time(buf + cursor);
        cursor += 3;
        break;
    case MYSQL_TYPE_TIME2:
        read_mysql_time2(buf + cursor);
        cursor += 3;
        break;
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_ENUM:
    case MYSQL_TYPE_SET:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_BLOB:
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_GEOMETRY:
    case MYSQL_TYPE_BIT:
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_NULL:
        printf("Not support yet\n");
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
    static int update_row = 1; // update 操作, 新老数据区分
    update_row++;

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

    // NOT UPDATE NEW, for pretty output
    if (ev->header.event_type != UPDATE_ROWS_EVENTv1 || (update_row % 2 == 0)) {
        printf("NORMAL ROW:\n");
    }

    // loop to get column value
    for (int i = 0; i < ev->column_count; i++) {
        cursor += get_column_val(ev, i, buf + cursor);
    }

    // UPDATE NEW VALUES
    if (ev->header.event_type == UPDATE_ROWS_EVENTv1 && (update_row % 2 == 0)) {
        printf("UPDATE NEW:\n");
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

    printf("Table id: %ld(%ld-[%s, %s])\n", ev->table_id, ev->table_map.table_id, ev->table_map.schema_name, ev->table_map.table_name);
    printf("Flags: ");print_memory((char *)&ev->flags, 2);
    printf("Column count: %d\n", ev->column_count);
#endif

    // rows.
    while (cursor < (ev->header.event_size - 19)) {
#ifdef DEBUG
        printf("Next row coluser\n");
#endif
        cursor += fetch_a_row(ev, buf + cursor);
    }

    return 0;
}
