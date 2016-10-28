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


int run_binlog_stream(server_info *info)
{
    char buf[1024];
    int tmp =0;
    int cursor = 0;

    while ((tmp = read(info->sockfd, buf + cursor, 1024 - cursor)) != -1) {
        // do parse binlog.
        cursor = 0;
        while (cursor < tmp) {
            int length = 0;
            char sequence_id = 0;

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
            // 4              timestamp
            // 1              event type
            // 4              server-id
            // 4              event-size
            // if binlog-version > 1:
            // 4              log pos
            // 2              flags
            uint32_t timestamp = 0;
            memcpy(&timestamp, buf + cursor, 4);
            cursor += 4;

            char event_type = 0;
            event_type = *(buf + cursor++);

            uint32_t server_id = 0;
            memcpy(&server_id, buf + cursor, 4);
            cursor += 4;

            uint32_t event_size = 0;
            memcpy(&event_size, buf + cursor, 4);
            cursor += 4;

            // position of the next event
            uint32_t log_pos = 0;
            memcpy(&log_pos, buf + cursor, 4);
            cursor += 4;

            uint16_t flags;
            memcpy(&flags, buf + cursor, 2);
            cursor += 2;

#ifdef DEBUG
            printf("\e[32mBinlog event\e[0m\n");
            printf("header.timestamp: %d\n", timestamp);
            printf("header.event_type: %02X\n", event_type);
            printf("header.event_size: %d\n", event_size);
            printf("header.log_pos (the next event position): %d\n", log_pos);
            printf("header.falgs: %d\n", flags); print_memory(buf + cursor - 2, 2);
#endif

            int tmp_length = 0;
            char post_header_length;
            switch (event_type) {
            case UNKNOWN_EVENT:
                cursor += (length - 20);
                printf("UNKNOWN_EVENT\n");
                break;
            case START_EVENT_V3:
                cursor += (length - 20);
                printf("START_EVENT_V3, Header length: %02X\n", get_post_header_length(START_EVENT_V3));
                break;
            case QUERY_EVENT:
                /*
                4              slave_proxy_id
                4              execution time
                1              schema length
                2              error-code
                  if binlog-version ≥ 4:
                2              status-vars length
                */
                printf("QUERY_EVENT, Header length: %02X\n", get_post_header_length(QUERY_EVENT));
                uint32_t slave_proxy_id;
                memcpy(&slave_proxy_id, buf + cursor, 4);
                cursor += 4;

                uint32_t execution_time;
                memcpy(&execution_time, buf + cursor, 4);
                cursor += 4;

                uint8_t schema_length;
                schema_length = *(buf + cursor++);

                uint16_t error_code;
                memcpy(&error_code, buf + cursor, 2);
                cursor += 2;

                uint16_t status_vars_length;
                memcpy(&status_vars_length, buf + cursor, 2);
                cursor += 2;

                /*
                string[$len]   status-vars
                string[$len]   schema
                1              [00]
                string[EOF]    query
                */

                char *status_vars = malloc(sizeof(char) * status_vars_length);
                memcpy(status_vars, buf + cursor, status_vars_length);
                cursor += status_vars_length;
                printf("status_vars prefix length: %d\n", status_vars_length);

                char *schema = malloc(sizeof(char) * schema_length);
                cursor += schema_length;
                printf("schema prefix length: %d\n", schema_length);

                cursor++;

                tmp_length = event_size - (19 + 13 + status_vars_length + schema_length + 1);
                char *query = malloc(tmp_length);
                memcpy(query, buf + cursor, tmp_length);
                cursor += tmp_length;

#ifdef DEBUG
                printf("Slave proxy id: %d\n", slave_proxy_id);
                printf("Execution time: %d\n", execution_time);
                printf("Schema length: %d\n", schema_length);
                printf("Error code: %d\n", error_code);
                printf("Status vars length: %d\n", status_vars_length);

                printf("Status vars: %s\n", status_vars_length ? status_vars : "NULL");
                printf("Schema: %s\n", schema_length ? schema : "NULL");
                printf("Query: %s\n", query);
#endif
                break;
            case STOP_EVENT:
                cursor += (length - 20);
                printf("STOP_EVENT, Header length: %02X\n", get_post_header_length(STOP_EVENT));
                break;
            case ROTATE_EVENT:
                printf("ROTATE_EVENT, Header length: %02X\n", get_post_header_length(ROTATE_EVENT));
                uint64_t position;
                memcpy(&position, buf + cursor, 8);
                cursor += 8;
                tmp_length = sizeof(char) * (event_size - 27);
                char *next_file = malloc(tmp_length);
                memcpy(next_file, buf + cursor, tmp_length);
                *(next_file + tmp_length) = 0;
                cursor += tmp_length;

#ifdef DEBUG
                printf("Tmp length: %d\n", tmp_length);
                printf("Position: %ld\n", position);
                printf("Next file name: %s\n", next_file);
#endif
                break;
            case INTVAR_EVENT:
                cursor += (length - 20);
                printf("INTVAR_EVENT, Header length: %02X\n", get_post_header_length(INTVAR_EVENT));
                break;
            case LOAD_EVENT:
                cursor += (length - 20);
                printf("LOAD_EVENT, Header length: %02X\n", get_post_header_length(LOAD_EVENT));
                break;
            case SLAVE_EVENT:
                cursor += (length - 20);
                printf("SLAVE_EVENT, Header length: %02X\n", get_post_header_length(SLAVE_EVENT));
                break;
            case CREATE_FILE_EVENT:
                cursor += (length - 20);
                printf("CREATE_FILE_EVENT, Header length: %02X\n", get_post_header_length(CREATE_FILE_EVENT));
                break;
            case APPEND_BLOCK_EVENT:
                cursor += (length - 20);
                printf("APPEND_BLOCK_EVENT, Header length: %02X\n", get_post_header_length(APPEND_BLOCK_EVENT));
                break;
            case EXEC_LOAD_EVENT:
                cursor += (length - 20);
                printf("EXEC_LOAD_EVENT, Header length: %02X\n", get_post_header_length(EXEC_LOAD_EVENT));
                break;
            case DELETE_FILE_EVENT:
                cursor += (length - 20);
                printf("DELETE_FILE_EVENT, Header length: %02X\n", get_post_header_length(DELETE_FILE_EVENT));
                break;
            case NEW_LOAD_EVENT:
                cursor += (length - 20);
                printf("NEW_LOAD_EVENT, Header length: %02X\n", get_post_header_length(NEW_LOAD_EVENT));
                break;
            case RAND_EVENT:
                cursor += (length - 20);
                printf("RAND_EVENT, Header length: %02X\n", get_post_header_length(RAND_EVENT));
                break;
            case USER_VAR_EVENT:
                cursor += (length - 20);
                printf("USER_VAR_EVENT, Header length: %02X\n", get_post_header_length(USER_VAR_EVENT));
                break;
            case FORMAT_DESCRIPTION_EVENT:
                printf("FORMAT_DESCRIPTION_EVENT, Header length: %02X\n", get_post_header_length(FORMAT_DESCRIPTION_EVENT));
                uint16_t binlog_version;
                memcpy(&binlog_version, buf + cursor, 2);
                cursor += 2;

                char mysql_version[50];
                strcpy(mysql_version, buf + cursor);
                cursor += 50;

                uint32_t create_timestamp;
                memcpy(&create_timestamp, buf + cursor, 4);
                cursor += 4;

                uint8_t event_header_length;
                event_header_length = *(buf + cursor++);

                tmp_length = sizeof(char) * (event_size - (19 + 2 + 50 + 4 + 1));
                free(event_types_post_header_length);
                event_types_post_header_length = realloc(event_types_post_header_length, tmp_length);
                memcpy(event_types_post_header_length, buf + cursor, tmp_length);
                cursor += tmp_length;

#ifdef DEBUG
                printf("Tmp length: %d\n", tmp_length);
                printf("Binlog version: %d\n", binlog_version);
                printf("MySQL version: %s\n", mysql_version);
                printf("Create timestamp: %d\n", create_timestamp);
                printf("Event Header length: %02X\n", event_header_length);
                printf("Event type header length: ...\n"); print_memory(event_types_post_header_length, tmp_length);
#endif

                break;
            case XID_EVENT:
                cursor += (length - 20);
                printf("XID_EVENT, Header length: %02X\n", get_post_header_length(XID_EVENT));
                break;
            case BEGIN_LOAD_QUERY_EVENT:
                cursor += (length - 20);
                printf("BEGIN_LOAD_QUERY_EVENT, Header length: %02X\n", get_post_header_length(BEGIN_LOAD_QUERY_EVENT));
                break;
            case EXECUTE_LOAD_QUERY_EVENT:
                cursor += (length - 20);
                printf("EXECUTE_LOAD_QUERY_EVENT, Header length: %02X\n", get_post_header_length(EXECUTE_LOAD_QUERY_EVENT));
                break;
            case TABLE_MAP_EVENT:
                post_header_length = get_post_header_length(TABLE_MAP_EVENT);

                uint64_t table_id = 0;
                if (post_header_length == 6) {
                    memcpy(&table_id, buf + cursor, 4);
                    cursor += 4;
                } else {
                    memcpy(&table_id, buf + cursor, 6);
                    cursor += 6;
                }

                uint16_t flags;
                memcpy(&flags, buf + cursor, 2);
                cursor += 2;

                // schema name length
                uint8_t schema_name_length = *(buf + cursor++);
                char *schema_name = malloc(sizeof(char) * schema_name_length);
                memcpy(schema_name, buf + cursor, schema_name_length);
                cursor += schema_name_length;

                cursor++; // [00] byte

                // table name length
                uint8_t table_name_length = *(buf + cursor++);
                char *table_name = malloc(sizeof(char) * table_name_length);
                memcpy(table_name, buf + cursor, table_name_length);
                cursor += table_name_length;

                cursor++; // [00] byte

                // lenenc-int     column-count
                // string.var_len [length=$column-count] column-def
                // lenenc-str     column-meta-def
                // n              NULL-bitmask, length: (column-count + 8) / 7

                int column_count = generate_length_encode_number(buf + cursor, &tmp_length);
                cursor += tmp_length;

                char *column_def = malloc(sizeof(char) * column_count);
                memcpy(column_def, buf + cursor, column_count);
                cursor += column_count;

                char *column_meta_def = generate_length_encode_string(buf + cursor, &tmp_length);
                cursor += tmp_length;

                printf("TABLE_MAP_EVENT, Header length: %02X\n", post_header_length);
                printf("Table id: %ld\n", table_id);
                printf("Flags: "); print_memory((char *)&flags, 2);
                printf("Schema name length: %d\n", schema_name_length);
                printf("Schema name: %s\n", schema_name);
                printf("Table name length: %d\n", table_name_length);
                printf("Table name: %s\n", table_name);
                printf("Column count: %d\n", column_count);
                printf("Column def: "); print_memory(column_def, column_count);
                printf("Column meta def: "); print_memory(column_meta_def, tmp_length);

                tmp_length = (column_count + 7) / 8;
                cursor += tmp_length;

                break;
            case WRITE_ROWS_EVENTv0:
                cursor += (length - 20);
                printf("WRITE_ROWS_EVENTv0, Header length: %02X\n", get_post_header_length(WRITE_ROWS_EVENTv0));
                break;
            case UPDATE_ROWS_EVENTv0:
                cursor += (length - 20);
                printf("UPDATE_ROWS_EVENTv0, Header length: %02X\n", get_post_header_length(UPDATE_ROWS_EVENTv0));
                break;
            case DELETE_ROWS_EVENTv0:
                cursor += (length - 20);
                printf("DELETE_ROWS_EVENTv0, Header length: %02X\n", get_post_header_length(DELETE_ROWS_EVENTv0));
                break;
            case WRITE_ROWS_EVENTv1:
                cursor += (length - 20);
                printf("WRITE_ROWS_EVENTv1, Header length: %02X\n", get_post_header_length(WRITE_ROWS_EVENTv1));
                break;
            case UPDATE_ROWS_EVENTv1:
                cursor += (length - 20);
                printf("UPDATE_ROWS_EVENTv1, Header length: %02X\n", get_post_header_length(UPDATE_ROWS_EVENTv1));
                break;
            case DELETE_ROWS_EVENTv1:
                cursor += (length - 20);
                printf("DELETE_ROWS_EVENTv1, Header length: %02X\n", get_post_header_length(DELETE_ROWS_EVENTv1));
                break;
            case INCIDENT_EVENT:
                cursor += (length - 20);
                printf("INCIDENT_EVENT, Header length: %02X\n", get_post_header_length(INCIDENT_EVENT));
                break;
            case HEARTBEAT_EVENT:
                cursor += (length - 20);
                printf("HEARTBEAT_EVENT, Header length: %02X\n", get_post_header_length(HEARTBEAT_EVENT));
                break;
            case IGNORABLE_EVENT:
                cursor += (length - 20);
                printf("IGNORABLE_EVENT, Header length: %02X\n", get_post_header_length(IGNORABLE_EVENT));
                break;
            case ROWS_QUERY_EVENT:
                cursor += (length - 20);
                printf("ROWS_QUERY_EVENT, Header length: %02X\n", get_post_header_length(ROWS_QUERY_EVENT));
                break;
            case WRITE_ROWS_EVENTv2:
                cursor += (length - 20);
                printf("WRITE_ROWS_EVENTv2, Header length: %02X\n", get_post_header_length(WRITE_ROWS_EVENTv2));
                break;
            case UPDATE_ROWS_EVENTv2:
                cursor += (length - 20);
                printf("UPDATE_ROWS_EVENTv2, Header length: %02X\n", get_post_header_length(UPDATE_ROWS_EVENTv2));
                break;
            case DELETE_ROWS_EVENTv2:
                cursor += (length - 20);
                printf("DELETE_ROWS_EVENTv2, Header length: %02X\n", get_post_header_length(DELETE_ROWS_EVENTv2));
                break;
            case GTID_EVENT:
                cursor += (length - 20);
                printf("GTID_EVENT, Header length: %02X\n", get_post_header_length(GTID_EVENT));
                break;
            case ANONYMOUS_GTID_EVENT:
                cursor += (length - 20);
                printf("ANONYMOUS_GTID_EVENT, Header length: %02X\n", get_post_header_length(ANONYMOUS_GTID_EVENT));
                break;
            case PREVIOUS_GTIDS_EVENT:
                cursor += (length - 20);
                printf("PREVIOUS_GTIDS_EVENT, Header length: %02X\n", get_post_header_length(PREVIOUS_GTIDS_EVENT));
                break;
            default:
                cursor += (length - 20);
                printf("Unknow binlog event.\n");
                break;
            }

            puts("======================================");
        }

        memcpy(buf, buf + cursor, 1024 - cursor);
    }

    return 0;
}
