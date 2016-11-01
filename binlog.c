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
int binlog_event_header(struct event_header *header, char *buf)
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
int format_description_event(struct format_description_event *fmt_des_ev, char *buf)
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
int query_event(struct query_event *query_ev, char *buf)
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
int table_map_event(struct table_map_event *ev, char *buf)
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
    cursor += tmp;

#ifdef DEBUG
    print_binlog_table_map_event(ev);
#endif

    return cursor;
}

// WRITE ROWS EVENT V1
int write_rows_event_v1(struct write_rows_event_v1 *ev, const char *buf)
{
    int post_header_length = get_post_header_length(WRITE_ROWS_EVENTv1);
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

    ev->column_count = get_length_encode_number(buf + cursor, &tmp);
    cursor += tmp;

    tmp = (ev->column_count + 7) / 8;
    ev->columns_present_bitmap1 = malloc(sizeof(char) * tmp);
    memcpy(ev->columns_present_bitmap1, buf + cursor, tmp);
    cursor += tmp;

    printf("WRITE_ROWS_EVENTv1, Header length: %02X\n", post_header_length);
    printf("Table id: %ld\n", ev->table_id);
    printf("Flags: ");print_memory((char *)&ev->flags, 2);
    printf("Column count: %d\n", ev->column_count);

    // rows.
    while (cursor < (ev->header.event_size - 19)) {
        // string.var_len       nul-bitmap, length (bits set in 'columns-present-bitmap1'+7)/8
        // string.var_len       value of each field as defined in table-map
        //   if UPDATE_ROWS_EVENTv1 or v2 {
        // string.var_len       nul-bitmap, length (bits set in 'columns-present-bitmap2'+7)/8
        // string.var_len       value of each field as defined in table-map
        //   }
        tmp = (ev->column_count + 7) / 8;
        char *bit_map = malloc(sizeof(char) * tmp);
        memcpy(bit_map, buf + cursor, tmp);
        cursor += tmp;

        printf("Bit map:"); print_memory(bit_map, tmp);

        // loop to get column value
        for (int i = 0; i < ev->column_count; i++) {
            unsigned char column_def = *(ev->table_map.column_def + i);
            printf("Column def: %02x\n", column_def);
            print_memory((char *)buf + cursor, 20);

            switch (column_def) {
            case MYSQL_TYPE_LONG:
            case MYSQL_TYPE_INT24:
                {
                    // long int.
                    int iv = 0;
                    memcpy(&iv, buf + cursor, 4);
                    cursor += 4;

                    printf("col %d, long int value: %d\n", i, iv);
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
            case MYSQL_TYPE_DECIMAL:
            case MYSQL_TYPE_NEWDECIMAL:
                {
                    // varchar
                    int tmp = 0;
                    char *s = get_length_encode_string(buf + cursor, &tmp);
                    cursor += tmp;

                    printf("col %d, varchar value: %s(%d)\n", i, s, tmp);
                }
                break;
            default:
                printf("Other type\n");
            }
        }

    }

    return 0;
}

int run_binlog_stream(server_info *info)
{
    char buf[1024];
    int tmp =0;
    int cursor = 0;

    while ((tmp = read(info->sockfd, buf + cursor, 1024 - cursor)) != -1) {
        // do parse binlog.
        cursor = 0;
        while (cursor < tmp) {
            int length = 0; // packet length
            char sequence_id = 0; // packet sequence id

            memcpy(&length, buf + cursor, 3);
            sequence_id = *(buf + cursor + 3);

            cursor += 4;

#ifdef DEBUG
            printf("\e[31mBinlog packet\e[0m\n");
            printf("Packet length: %d\n", length);
            printf("Sequence ID: %d\n", sequence_id);
            if (sequence_id >= 14) { print_memory(buf + cursor + 4, length - 1); }
#endif
            // skip a 00-OK byte
            cursor += 1;

            // real binlog event
            struct event_header ev_header;
            cursor += binlog_event_header(&ev_header, buf + cursor);

            struct query_event query_ev;
            struct rotate_event rotate_ev;
            struct table_map_event table_map_ev;
            struct format_description_event fmt_des_ev;
            struct write_rows_event_v1 write_rows_ev_v1;

            switch (ev_header.event_type) {
            case UNKNOWN_EVENT:
                printf("UNKNOWN_EVENT\n");
                break;
            case START_EVENT_V3:
                printf("START_EVENT_V3, Header length: %02X\n", get_post_header_length(START_EVENT_V3));
                break;
            case QUERY_EVENT:
                query_ev.header = ev_header;
                query_event(&query_ev, buf + cursor);
                break;
            case STOP_EVENT:
                printf("STOP_EVENT, Header length: %02X\n", get_post_header_length(STOP_EVENT));
                break;
            case ROTATE_EVENT:
                rotate_ev.header = ev_header;
                rotate_event(&rotate_ev, buf + cursor);
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
                format_description_event(&fmt_des_ev, buf + cursor);
                event_types_post_header_length = fmt_des_ev.event_types_post_header_length;
                break;
            case XID_EVENT:
                printf("XID_EVENT, Header length: %02X\n", get_post_header_length(XID_EVENT));
                break;
            case BEGIN_LOAD_QUERY_EVENT:
                printf("BEGIN_LOAD_QUERY_EVENT, Header length: %02X\n", get_post_header_length(BEGIN_LOAD_QUERY_EVENT));
                break;
            case EXECUTE_LOAD_QUERY_EVENT:
                printf("EXECUTE_LOAD_QUERY_EVENT, Header length: %02X\n", get_post_header_length(EXECUTE_LOAD_QUERY_EVENT));
                break;
            case TABLE_MAP_EVENT:
                table_map_ev.header = ev_header;
                table_map_event(&table_map_ev, buf + cursor);
                break;
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
                write_rows_ev_v1.header = ev_header;
                write_rows_ev_v1.table_map = table_map_ev;
                write_rows_event_v1(&write_rows_ev_v1, buf + cursor);
                printf("WRITE_ROWS_EVENTv1, Header length: %02X\n", get_post_header_length(WRITE_ROWS_EVENTv1));
                break;
            case UPDATE_ROWS_EVENTv1:
                printf("UPDATE_ROWS_EVENTv1, Header length: %02X\n", get_post_header_length(UPDATE_ROWS_EVENTv1));
                break;
            case DELETE_ROWS_EVENTv1:
                printf("DELETE_ROWS_EVENTv1, Header length: %02X\n", get_post_header_length(DELETE_ROWS_EVENTv1));
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
            case WRITE_ROWS_EVENTv2:
                printf("WRITE_ROWS_EVENTv2, Header length: %02X\n", get_post_header_length(WRITE_ROWS_EVENTv2));
                break;
            case UPDATE_ROWS_EVENTv2:
                printf("UPDATE_ROWS_EVENTv2, Header length: %02X\n", get_post_header_length(UPDATE_ROWS_EVENTv2));
                break;
            case DELETE_ROWS_EVENTv2:
                printf("DELETE_ROWS_EVENTv2, Header length: %02X\n", get_post_header_length(DELETE_ROWS_EVENTv2));
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

            cursor += ev_header.event_size - 19;
            puts("======================================");
        }

        memcpy(buf, buf + cursor, 1024 - cursor);
    }

    return 0;
}
