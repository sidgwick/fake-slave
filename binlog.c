#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "client.h"
#include "query.h"

int run_binlog_stream(server_info *info)
{
    char buf[1024];
    int tmp =0;
    int cursor = 0;

    while ((tmp = read(info->sockfd, buf, 1024)) != -1) {
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
            print_memory(buf + cursor + 4, length - 1);
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
            printf("header.falgs: %d\n", flags);
            print_memory(buf + cursor - 2, 2);
#endif

            switch (event_type) {
            case UNKNOWN_EVENT:
                printf("UNKNOWN_EVENT\n");
                break;
            case START_EVENT_V3:
                printf("START_EVENT_V3\n");
                break;
            case QUERY_EVENT:
                printf("QUERY_EVENT\n");
                break;
            case STOP_EVENT:
                printf("STOP_EVENT\n");
                break;
            case ROTATE_EVENT:
                printf("ROTATE_EVENT\n");
                break;
            case INTVAR_EVENT:
                printf("INTVAR_EVENT\n");
                break;
            case LOAD_EVENT:
                printf("LOAD_EVENT\n");
                break;
            case SLAVE_EVENT:
                printf("SLAVE_EVENT\n");
                break;
            case CREATE_FILE_EVENT:
                printf("CREATE_FILE_EVENT\n");
                break;
            case APPEND_BLOCK_EVENT:
                printf("APPEND_BLOCK_EVENT\n");
                break;
            case EXEC_LOAD_EVENT:
                printf("EXEC_LOAD_EVENT\n");
                break;
            case DELETE_FILE_EVENT:
                printf("DELETE_FILE_EVENT\n");
                break;
            case NEW_LOAD_EVENT:
                printf("NEW_LOAD_EVENT\n");
                break;
            case RAND_EVENT:
                printf("RAND_EVENT\n");
                break;
            case USER_VAR_EVENT:
                printf("USER_VAR_EVENT\n");
                break;
            case FORMAT_DESCRIPTION_EVENT:
                printf("FORMAT_DESCRIPTION_EVENT\n");
                break;
            case XID_EVENT:
                printf("XID_EVENT\n");
                break;
            case BEGIN_LOAD_QUERY_EVENT:
                printf("BEGIN_LOAD_QUERY_EVENT\n");
                break;
            case EXECUTE_LOAD_QUERY_EVENT:
                printf("EXECUTE_LOAD_QUERY_EVENT\n");
                break;
            case TABLE_MAP_EVENT:
                printf("TABLE_MAP_EVENT\n");
                break;
            case WRITE_ROWS_EVENTv0:
                printf("WRITE_ROWS_EVENTv0\n");
                break;
            case UPDATE_ROWS_EVENTv0:
                printf("UPDATE_ROWS_EVENTv0\n");
                break;
            case DELETE_ROWS_EVENTv0:
                printf("DELETE_ROWS_EVENTv0\n");
                break;
            case WRITE_ROWS_EVENTv1:
                printf("WRITE_ROWS_EVENTv1\n");
                break;
            case UPDATE_ROWS_EVENTv1:
                printf("UPDATE_ROWS_EVENTv1\n");
                break;
            case DELETE_ROWS_EVENTv1:
                printf("DELETE_ROWS_EVENTv1\n");
                break;
            case INCIDENT_EVENT:
                printf("INCIDENT_EVENT\n");
                break;
            case HEARTBEAT_EVENT:
                printf("HEARTBEAT_EVENT\n");
                break;
            case IGNORABLE_EVENT:
                printf("IGNORABLE_EVENT\n");
                break;
            case ROWS_QUERY_EVENT:
                printf("ROWS_QUERY_EVENT\n");
                break;
            case WRITE_ROWS_EVENTv2:
                printf("WRITE_ROWS_EVENTv2\n");
                break;
            case UPDATE_ROWS_EVENTv2:
                printf("UPDATE_ROWS_EVENTv2\n");
                break;
            case DELETE_ROWS_EVENTv2:
                printf("DELETE_ROWS_EVENTv2\n");
                break;
            case GTID_EVENT:
                printf("GTID_EVENT\n");
                break;
            case ANONYMOUS_GTID_EVENT:
                printf("ANONYMOUS_GTID_EVENT\n");
                break;
            case PREVIOUS_GTIDS_EVENT:
                printf("PREVIOUS_GTIDS_EVENT\n");
                break;
            default:
                printf("Unknow binlog event.\n");
                break;
            }

            cursor += (length - 20);
        }
    }

    return 0;
}