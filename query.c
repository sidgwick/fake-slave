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

    return 0;
}
