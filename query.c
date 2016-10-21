#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "client.h"
#include "capability_flags.h"
#include "packet.h"
#include "debug.h"

int send_query(server_info *info, const char *sql)
{
    char buf[1024];
    int cursor = 4;
    int tmp;

    // COM_QUERY
    *(buf + cursor++) = 0x03;
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
    // char *sql = "SHOW GLOBAL VARIABLES LIKE 'BINLOG_CHECKSUM'";
    char *sql = "select Host,User,Password from mysql.user";

    send_query(info, sql);
    fetch_query_row(info);

    return 0;
}
