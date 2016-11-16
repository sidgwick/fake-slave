#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

#include "client.h"
#include "query.h"
#include "binlog.h"
#include "decimal.h"
#include "tools.h"
#include "debug.h"

char *event_types_post_header_length = NULL;

// return specific event_type post_header length.
// return -1 if event_types post header length array is not set.
int get_post_header_length(int event_type)
{
    if (event_types_post_header_length == NULL) {
        return -1;
    } else {
        return *(event_types_post_header_length + event_type - 1);
    }
}

// return header length
int binlog_event_header(struct event_header *header, const char *buf)
{
    int cursor = 0;

    memcpy(&header->timestamp, buf + cursor, 4);
    cursor += 4;

    header->event_type = *(buf + cursor++);

    memcpy(&header->server_id, buf + cursor, 4);
    cursor += 4;

    memcpy(&header->event_size, buf + cursor, 4);
    cursor += 4;

    // position of the next event
    memcpy(&header->log_pos, buf + cursor, 4);
    cursor += 4;

    memcpy(&header->flags, buf + cursor, 2);
    cursor += 2;

#ifdef DEBUG
    print_binlog_event_header(header);
#endif

    return cursor;
}

// ROTATE_EVENT: return the event body(include the post header) length.
int rotate_event(struct rotate_event *ev, const char *buf)
{
    int cursor = 0;
    int rest_length = 0;

    memcpy(&ev->position, buf + cursor, 8);
    cursor += 8;

    rest_length = sizeof(char) * (ev->header.event_size - (19 + cursor));
    ev->next_file = malloc(rest_length);
    memcpy(ev->next_file, buf + cursor, rest_length);
    *(ev->next_file + rest_length) = 0;
    cursor += rest_length;

#ifdef DEBUG
    print_binlog_rotate_event(ev);
#endif

    return cursor;
}

// FORMAT_DESCRIPTION_EVENT
int format_description_event(struct format_description_event *fmt_des_ev, const char *buf)
{
    int cursor = 0;
    int rest_length = 0;

    memcpy(&fmt_des_ev->binlog_version, buf + cursor, 2);
    cursor += 2;

    strcpy(fmt_des_ev->mysql_version, buf + cursor);
    cursor += 50;

    memcpy(&fmt_des_ev->create_timestamp, buf + cursor, 4);
    cursor += 4;

    fmt_des_ev->event_header_length = *(buf + cursor++);

    rest_length = sizeof(char) * (fmt_des_ev->header.event_size - (19 + 2 + 50 + 4 + 1));
    fmt_des_ev->event_types_post_header_length = realloc(event_types_post_header_length, rest_length);
    memcpy(fmt_des_ev->event_types_post_header_length, buf + cursor, rest_length);
    cursor += rest_length;

#ifdef DEBUG
    print_binlog_format_description_event(fmt_des_ev);
#endif

    return cursor;
}

// QUERY_EVENT
int query_event(struct query_event *query_ev, const char *buf)
{
    int cursor = 0;
    int rest_length = 0;

    memcpy(&query_ev->slave_proxy_id, buf + cursor, 4);
    cursor += 4;

    memcpy(&query_ev->execution_time, buf + cursor, 4);
    cursor += 4;

    query_ev->schema_length = *(buf + cursor++);

    memcpy(&query_ev->error_code, buf + cursor, 2);
    cursor += 2;

    memcpy(&query_ev->status_vars_length, buf + cursor, 2);
    cursor += 2;

    query_ev->status_vars = malloc(sizeof(char) * query_ev->status_vars_length);
    memcpy(query_ev->status_vars, buf + cursor, query_ev->status_vars_length);
    cursor += query_ev->status_vars_length;

    query_ev->schema = malloc(sizeof(char) * query_ev->schema_length);
    cursor += query_ev->schema_length;

    cursor++;

    rest_length = query_ev->header.event_size - (19 + cursor);
    query_ev->query = malloc(rest_length);
    memcpy(query_ev->query, buf + cursor, rest_length);
    cursor += rest_length;

#ifdef DEBUG
    print_binlog_query_event(query_ev);
#endif

    return cursor;
}

// TABLE MAP EVENT
int table_map_event(struct table_map_event *ev, const char *buf)
{
    int post_header_length = get_post_header_length(TABLE_MAP_EVENT);
    int cursor = 0;
    int tmp = 0;

    if (post_header_length == 6) {
        memcpy(&ev->table_id, buf + cursor, 4);
        cursor += 4;
    } else {
        memcpy(&ev->table_id, buf + cursor, 6);
        cursor += 6;
    }

    memcpy(&ev->flags, buf + cursor, 2);
    cursor += 2;

    // schema name length
    ev->schema_name_length = *(buf + cursor++);
    ev->schema_name = malloc(sizeof(char) * ev->schema_name_length);
    memcpy(ev->schema_name, buf + cursor, ev->schema_name_length);
    cursor += ev->schema_name_length;

    cursor++; // [00] byte

    // table name length
    ev->table_name_length = *(buf + cursor++);
    ev->table_name = malloc(sizeof(char) * ev->table_name_length);
    memcpy(ev->table_name, buf + cursor, ev->table_name_length);
    cursor += ev->table_name_length;

    cursor++; // [00] byte

    ev->column_count = get_length_encode_number(buf + cursor, &tmp);
    cursor += tmp;

    ev->column_def = malloc(sizeof(char) * ev->column_count);
    memcpy(ev->column_def, buf + cursor, ev->column_count);
    cursor += ev->column_count;

    ev->column_meta_def = get_length_encode_string(buf + cursor, &tmp);
    cursor += tmp;

    tmp = (ev->column_count + 7) / 8;
    ev->null_bitmask = malloc(sizeof(char) * tmp);
    memcpy(ev->null_bitmask, buf + cursor, tmp);
    cursor += tmp;

#ifdef DEBUG
    print_binlog_table_map_event(ev);
#endif

    return cursor;
}

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

int get_column_val(struct rows_event *ev, int column_id, const char *buf)
{
    int cursor = 0;

    unsigned char column_def = *(ev->table_map.column_def + column_id);
    printf("Column def: %02x, ID: %d value: ", column_def, column_id);

    switch (column_def) {
    case MYSQL_TYPE_LONG:
        {
            // long int.
            int iv = 0;
            memcpy(&iv, buf, 4);
            cursor += 4;

            printf("LONG %d\n", iv);
        }
        break;
    case MYSQL_TYPE_INT24:
        {
            // long int.
            int iv = 0;
            memcpy(&iv, buf, 3);
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
            uint16_t num = 0;
            memcpy(&num, buf + cursor, 2);
            cursor += 2;
            printf("SHORT INT: %d\n", num);
        }
        break;
    case MYSQL_TYPE_LONGLONG:
        {
            uint64_t num = 0;
            memcpy(&num, buf + cursor, 8);
            cursor += 8;
            printf("LONG LONG INT: %ld\n", num);
        }
        break;
    case MYSQL_TYPE_DOUBLE:
        {
            double d;
            memcpy(&d, buf + cursor, 8);
            cursor += 8;
            printf("DOUBLE: %lf\n", d);
        }
        break;
    case MYSQL_TYPE_FLOAT:
        {
            float f;
            memcpy(&f, buf + cursor, 4);
            cursor += 4;
            printf("FLOAT: %f\n", f);
        }
        break;
    case MYSQL_TYPE_DATE:
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_TIMESTAMP:
        {
        }
        break;
    case MYSQL_TYPE_TIME:
        {
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
int rows_event(struct rows_event *ev, const char *buf)
{
    int post_header_length = get_post_header_length(ev->header.event_type);
    int cursor = 0;
    int tmp;

    if (post_header_length == 6) {
        memcpy(&ev->table_id, buf + cursor, 4);
        cursor += 4;
    } else {
        memcpy(&ev->table_id, buf + cursor, 6);
        cursor += 6;
    }

    memcpy(&ev->flags, buf + cursor, 2);
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

int parse_binlog_events(struct event_header ev_header, const char *buf)
{
    static struct query_event query_ev;
    static struct rotate_event rotate_ev;
    static struct table_map_event table_map_ev;
    static struct format_description_event fmt_des_ev;
    static struct rows_event rows_ev;

    switch (ev_header.event_type) {
    case UNKNOWN_EVENT:
        printf("UNKNOWN_EVENT\n");
        break;
    case START_EVENT_V3:
        printf("START_EVENT_V3, Header length: %02X\n", get_post_header_length(START_EVENT_V3));
        break;
    case QUERY_EVENT:
        query_ev.header = ev_header;
        query_event(&query_ev, buf);
        break;
    case STOP_EVENT:
        printf("STOP_EVENT, Header length: %02X\n", get_post_header_length(STOP_EVENT));
        break;
    case ROTATE_EVENT:
        rotate_ev.header = ev_header;
        rotate_event(&rotate_ev, buf);
        break;
    case INTVAR_EVENT:
        printf("INTVAR_EVENT, Header length: %02X\n", get_post_header_length(INTVAR_EVENT));
        break;
    case LOAD_EVENT:
        printf("LOAD_EVENT, Header length: %02X\n", get_post_header_length(LOAD_EVENT));
        break;
    case SLAVE_EVENT:
        printf("SLAVE_EVENT, Header length: %02X\n", get_post_header_length(SLAVE_EVENT));
        break;
    case CREATE_FILE_EVENT:
        printf("CREATE_FILE_EVENT, Header length: %02X\n", get_post_header_length(CREATE_FILE_EVENT));
        break;
    case APPEND_BLOCK_EVENT:
        printf("APPEND_BLOCK_EVENT, Header length: %02X\n", get_post_header_length(APPEND_BLOCK_EVENT));
        break;
    case EXEC_LOAD_EVENT:
        printf("EXEC_LOAD_EVENT, Header length: %02X\n", get_post_header_length(EXEC_LOAD_EVENT));
        break;
    case DELETE_FILE_EVENT:
        printf("DELETE_FILE_EVENT, Header length: %02X\n", get_post_header_length(DELETE_FILE_EVENT));
        break;
    case NEW_LOAD_EVENT:
        printf("NEW_LOAD_EVENT, Header length: %02X\n", get_post_header_length(NEW_LOAD_EVENT));
        break;
    case RAND_EVENT:
        printf("RAND_EVENT, Header length: %02X\n", get_post_header_length(RAND_EVENT));
        break;
    case USER_VAR_EVENT:
        printf("USER_VAR_EVENT, Header length: %02X\n", get_post_header_length(USER_VAR_EVENT));
        break;
    case FORMAT_DESCRIPTION_EVENT:
        fmt_des_ev.header = ev_header;
        format_description_event(&fmt_des_ev, buf);
        event_types_post_header_length = fmt_des_ev.event_types_post_header_length;
        break;
    case XID_EVENT:
        {
            uint64_t xid = 0;
            memcpy(&xid, buf, 8);
            printf("XID_EVENT: xid = %04ld, Header length: 0x%02X\n", xid, get_post_header_length(XID_EVENT));
        }
        break;
    case BEGIN_LOAD_QUERY_EVENT:
        printf("BEGIN_LOAD_QUERY_EVENT, Header length: %02X\n", get_post_header_length(BEGIN_LOAD_QUERY_EVENT));
        break;
    case EXECUTE_LOAD_QUERY_EVENT:
        printf("EXECUTE_LOAD_QUERY_EVENT, Header length: %02X\n", get_post_header_length(EXECUTE_LOAD_QUERY_EVENT));
        break;
    case TABLE_MAP_EVENT:
        table_map_ev.header = ev_header;
        table_map_event(&table_map_ev, buf);
        break;
    case WRITE_ROWS_EVENTv0:
    case UPDATE_ROWS_EVENTv0:
    case DELETE_ROWS_EVENTv0:
    case WRITE_ROWS_EVENTv1:
    case UPDATE_ROWS_EVENTv1:
    case DELETE_ROWS_EVENTv1:
    case WRITE_ROWS_EVENTv2:
    case UPDATE_ROWS_EVENTv2:
    case DELETE_ROWS_EVENTv2:
        rows_ev.header = ev_header;
        rows_ev.table_map = table_map_ev;
        rows_event(&rows_ev, buf);
        break;
    case INCIDENT_EVENT:
        printf("INCIDENT_EVENT, Header length: %02X\n", get_post_header_length(INCIDENT_EVENT));
        break;
    case HEARTBEAT_EVENT:
        printf("HEARTBEAT_EVENT, Header length: %02X\n", get_post_header_length(HEARTBEAT_EVENT));
        break;
    case IGNORABLE_EVENT:
        printf("IGNORABLE_EVENT, Header length: %02X\n", get_post_header_length(IGNORABLE_EVENT));
        break;
    case ROWS_QUERY_EVENT:
        printf("ROWS_QUERY_EVENT, Header length: %02X\n", get_post_header_length(ROWS_QUERY_EVENT));
        break;
    case GTID_EVENT:
        printf("GTID_EVENT, Header length: %02X\n", get_post_header_length(GTID_EVENT));
        break;
    case ANONYMOUS_GTID_EVENT:
        printf("ANONYMOUS_GTID_EVENT, Header length: %02X\n", get_post_header_length(ANONYMOUS_GTID_EVENT));
        break;
    case PREVIOUS_GTIDS_EVENT:
        printf("PREVIOUS_GTIDS_EVENT, Header length: %02X\n", get_post_header_length(PREVIOUS_GTIDS_EVENT));
        break;
    default:
        printf("Unknow binlog event.\n");
        break;
    }

    return 0;
}

int run_binlog_stream(server_info *info)
{
    char buf[BUF_SIZE];
    int rbuflen = 0;
    int cursor = 0;

    while ((rbuflen += read(info->sockfd, buf + rbuflen, BUF_SIZE - rbuflen)) != -1) {
        // do parse binlog.
        cursor = 0;
        // 1. 最少需要能把packet头部, binlog event头部解析出来, 没有的话, 就需要再向服务器读
        while (cursor + (4 + 19 + 1) < rbuflen) {
            printf("\e[34mCUROSR\e[0m: %d\n", cursor);
            int length = 0; // packet length
            unsigned char sequence_id = 0; // packet sequence id

            memcpy(&length, buf + cursor, 3);
            sequence_id = *(buf + cursor + 3);

            cursor += 4;

#ifdef DEBUG
            printf("\e[31mBinlog packet\e[0m: length = %04d, sequence_id = %u\n", length, sequence_id);
#endif
            // skip a 00-OK byte
            cursor += 1;

            // real binlog event
            struct event_header ev_header;
            cursor += binlog_event_header(&ev_header, buf + cursor);

            // 2. 判断剩下的buffer里面, 是不是有这个完整的binlog event.
            // 没有的话, buf游标往回退, 等服务器返回更多的数据后, 再处理
            if (cursor + (ev_header.event_size - 19) > rbuflen) {
                cursor -= (19 + 1 + 4);
                break;
            }

            parse_binlog_events(ev_header, buf + cursor);

            cursor += ev_header.event_size - 19;
            puts("======================================");
        }

        memcpy(buf, buf + cursor, rbuflen - cursor);
        rbuflen -= cursor;
    }

    return 0;
}
