#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

#include "client.h"
#include "query.h"
#include "binlog.h"
#include "debug.h"

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

            switch (event_type) {
            case UNKNOWN_EVENT:
                cursor += (length - 20);
                printf("UNKNOWN_EVENT\n");
                break;
            case START_EVENT_V3:
                cursor += (length - 20);
                printf("START_EVENT_V3\n");
                break;
            case QUERY_EVENT:
                cursor += (length - 20);
                printf("QUERY_EVENT\n");
                break;
            case STOP_EVENT:
                cursor += (length - 20);
                printf("STOP_EVENT\n");
                break;
            case ROTATE_EVENT:
                puts("======================================");
                printf("ROTATE_EVENT\n");
                uint64_t position;
                memcpy(&position, buf + cursor, 8);
                cursor += 8;
                int tmp_length = sizeof(char) * (event_size - 27);
                char *next_file = malloc(tmp_length);
                memcpy(next_file, buf + cursor, tmp_length);
                *(next_file + tmp_length) = 0;
                cursor += tmp_length;
                printf("Tmp length: %ld\n", tmp_length);
                printf("Position: %ld\n", position);
                printf("Next file name: %s\n", next_file);
                puts("======================================");
                break;
            case INTVAR_EVENT:
                cursor += (length - 20);
                printf("INTVAR_EVENT\n");
                break;
            case LOAD_EVENT:
                cursor += (length - 20);
                printf("LOAD_EVENT\n");
                break;
            case SLAVE_EVENT:
                cursor += (length - 20);
                printf("SLAVE_EVENT\n");
                break;
            case CREATE_FILE_EVENT:
                cursor += (length - 20);
                printf("CREATE_FILE_EVENT\n");
                break;
            case APPEND_BLOCK_EVENT:
                cursor += (length - 20);
                printf("APPEND_BLOCK_EVENT\n");
                break;
            case EXEC_LOAD_EVENT:
                cursor += (length - 20);
                printf("EXEC_LOAD_EVENT\n");
                break;
            case DELETE_FILE_EVENT:
                cursor += (length - 20);
                printf("DELETE_FILE_EVENT\n");
                break;
            case NEW_LOAD_EVENT:
                cursor += (length - 20);
                printf("NEW_LOAD_EVENT\n");
                break;
            case RAND_EVENT:
                cursor += (length - 20);
                printf("RAND_EVENT\n");
                break;
            case USER_VAR_EVENT:
                cursor += (length - 20);
                printf("USER_VAR_EVENT\n");
                break;
            case FORMAT_DESCRIPTION_EVENT:
                cursor += (length - 20);
                printf("FORMAT_DESCRIPTION_EVENT\n");
                break;
            case XID_EVENT:
                cursor += (length - 20);
                printf("XID_EVENT\n");
                break;
            case BEGIN_LOAD_QUERY_EVENT:
                cursor += (length - 20);
                printf("BEGIN_LOAD_QUERY_EVENT\n");
                break;
            case EXECUTE_LOAD_QUERY_EVENT:
                cursor += (length - 20);
                printf("EXECUTE_LOAD_QUERY_EVENT\n");
                break;
            case TABLE_MAP_EVENT:
                cursor += (length - 20);
                printf("TABLE_MAP_EVENT\n");
                break;
            case WRITE_ROWS_EVENTv0:
                cursor += (length - 20);
                printf("WRITE_ROWS_EVENTv0\n");
                break;
            case UPDATE_ROWS_EVENTv0:
                cursor += (length - 20);
                printf("UPDATE_ROWS_EVENTv0\n");
                break;
            case DELETE_ROWS_EVENTv0:
                cursor += (length - 20);
                printf("DELETE_ROWS_EVENTv0\n");
                break;
            case WRITE_ROWS_EVENTv1:
                cursor += (length - 20);
                printf("WRITE_ROWS_EVENTv1\n");
                break;
            case UPDATE_ROWS_EVENTv1:
                cursor += (length - 20);
                printf("UPDATE_ROWS_EVENTv1\n");
                break;
            case DELETE_ROWS_EVENTv1:
                cursor += (length - 20);
                printf("DELETE_ROWS_EVENTv1\n");
                break;
            case INCIDENT_EVENT:
                cursor += (length - 20);
                printf("INCIDENT_EVENT\n");
                break;
            case HEARTBEAT_EVENT:
                cursor += (length - 20);
                printf("HEARTBEAT_EVENT\n");
                break;
            case IGNORABLE_EVENT:
                cursor += (length - 20);
                printf("IGNORABLE_EVENT\n");
                break;
            case ROWS_QUERY_EVENT:
                cursor += (length - 20);
                printf("ROWS_QUERY_EVENT\n");
                break;
            case WRITE_ROWS_EVENTv2:
                cursor += (length - 20);
                printf("WRITE_ROWS_EVENTv2\n");
                break;
            case UPDATE_ROWS_EVENTv2:
                cursor += (length - 20);
                printf("UPDATE_ROWS_EVENTv2\n");
                break;
            case DELETE_ROWS_EVENTv2:
                cursor += (length - 20);
                printf("DELETE_ROWS_EVENTv2\n");
                break;
            case GTID_EVENT:
                cursor += (length - 20);
                printf("GTID_EVENT\n");
                break;
            case ANONYMOUS_GTID_EVENT:
                cursor += (length - 20);
                printf("ANONYMOUS_GTID_EVENT\n");
                break;
            case PREVIOUS_GTIDS_EVENT:
                cursor += (length - 20);
                printf("PREVIOUS_GTIDS_EVENT\n");
                break;
            default:
                cursor += (length - 20);
                printf("Unknow binlog event.\n");
                break;
            }
        }

        memcpy(buf, buf + cursor, 1024 - cursor);
    }

    return 0;
}
