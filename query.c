#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "client.h"
#include "capability_flags.h"
#include "query.h"
#include "packet.h"
#include "binlog.h"
#include "tools.h"
#include "debug.h"

int send_query(server_info *info, const char *sql)
{
    char buf[1024];
    int cursor = 4;
    int tmp;

    // COM_QUERY protocol
    *(buf + cursor++) = COM_QUERY;
    // QUERY_STRING
    printf("SQL: %s\n", sql);
    strcpy(buf + cursor, sql);
    cursor += strlen(sql) + 1; // append the '\0' byte.

    tmp = cursor - 4;
    memcpy(buf, &tmp, 3);
    *(buf + 3) = 0;

    write(info->sockfd, buf, cursor);

#ifdef DEBUG
    printf("Query data send to server:\n");
    print_memory(buf, cursor);
#endif

    return 0;
}

int fetch_query_row(server_info *info)
{
    char buf[1024];
    char *cursor;
    int datalen;
    // get response from server.
    if ((datalen = read(info->sockfd, buf, 1024)) == -1) {
        perror("Unable to read from server: ");
        exit(0);
    }

    uint8_t field_count = *(buf + 4);

    if (field_count == 0x00) {
        // next is a OK packet
        ok_packet ok_pkt;
        if (parse_ok_packet(buf + 5, &ok_pkt) == -1 || ok_pkt.header == 0xFE) {
            printf("Unexpect packet: expect OK_Packet\n");
            exit(2);
        }
    } else if (field_count == 0xFF) {
        // next is a ERR packet
        return 0;
    } else if (field_count == 0xFB) {
        // get more client data.
        return 0;
    }

    // parse column define packet
    cursor = buf + 5;
    while (cursor - buf < datalen) {
        // here a new packet
        uint32_t length = 0;
        uint8_t id = 0;

        memcpy(&length, cursor, 3);
        cursor += 3;
        id = *cursor++;


        if (id <= field_count) {
            // column definition packet
            parse_column_define_packet(cursor);
        } else if (!(CLIENT_DEPRECATE_EOF & info->capability_flags) && id == (field_count + 1)) {
            // a EOF packet
            ok_packet eof_pkt;
            parse_ok_packet(cursor, &eof_pkt);
        } else {
            // column value.
            // parse_column_value();
        }


#ifdef DEBUG
        printf("Query result packet length: %d\n", length);
        printf("Query result packet id: %d\n", id);

        printf("Query result packet content:\n");
        print_memory(cursor - 4, length + 4);
#endif

        cursor += length;
    }

    return 0;
}

int checksum_binlog(server_info *info)
{
    // TODO: query from server.
    return 0;

    // char *sql = "SHOW GLOBAL VARIABLES LIKE 'BINLOG_CHECKSUM'";
    char *sql = "select Host,User,Password from mysql.user";

    send_query(info, sql);
    fetch_query_row(info);

    return 0;
}

int register_as_slave(server_info *info)
{
    char buf[1024];
    int cursor = 4;
    int tmp = 0;

    // COM_REGISTER_SLAVE
    *(buf + cursor) = COM_REGISTER_SLAVE;
    cursor += 1;

    // slave id
    tmp = 1001;
    memcpy(buf + cursor, &tmp, 4);
    cursor += 4;

    // host name.
    set_length_encode_string(buf + cursor, "song_localhost", &tmp);
    cursor += tmp;

    // username
    set_length_encode_string(buf + cursor, "root", &tmp);
    cursor += tmp;

    // password
    set_length_encode_string(buf + cursor, "1111", &tmp);
    cursor += tmp;

    // slave port
    tmp = 20001;
    memcpy(buf + cursor, &tmp, 2);
    cursor += 2;

    // replication rank
    tmp = 0;
    memcpy(buf + cursor, &tmp, 4);
    cursor += 4;

    // master id
    tmp = 0;
    memcpy(buf + cursor, &tmp, 4);
    cursor += 4;

    cursor -= 4;
    memcpy(buf, &cursor, 3);
    *(buf + 3) = 0;

    write(info->sockfd, buf, cursor + 4);

    // expect a OK packet here.
    read(info->sockfd, buf, 1024);
    ok_packet ok_pkt;
    if (parse_ok_packet(buf, &ok_pkt) != 0x00) {
        // ERR packet.
        printf("Unable to register as a slave.\n");
        exit(3);
    }

    return 0;
}

int send_binlog_dump_request(server_info *info)
{
    char buf[1024];
    int cursor = 4;
    int tmp;

    // COM_BINLOG_DUMP
    *(buf + cursor) = COM_BINLOG_DUMP;
    cursor += 1;

    // binlog postion
    tmp = 659;
    memcpy(buf + cursor, &tmp, 4);
    cursor += 4;

    // flag
    tmp = 0;
    memcpy(buf + cursor, &tmp, 2);
    cursor += 2;

    // slave server id
    tmp = 1001;
    memcpy(buf + cursor, &tmp, 4);
    cursor += 4;

    // binlog file name.
    char binlog_filename[] = "mysql-bin.000028";
    strcpy(buf + cursor, binlog_filename);
    cursor += strlen(binlog_filename) + 1; // include the '\0' term null byte.

    tmp = cursor - 4;
    memcpy(buf, &tmp, 3);
    *(buf + 3) = 0;

    write(info->sockfd, buf, cursor);

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
            //    if binlog-version > 1:
            //    4              log pos
            //    2              flags
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
