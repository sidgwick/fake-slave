#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "packet.h"
#include "tools.h"

// parse ok packet
// return OK/EOF packet header or -1 if not a OK/EOF packet.
int parse_ok_packet(const char *buf, ok_packet *pkt)
{
    int cursor = 0;
    int grow = 0;

    memcpy(&pkt->length, buf, 3);
    cursor += 3;

    pkt->id = *(buf + cursor++);

    // packet body.
    pkt->header = *(buf + cursor++);
    pkt->affected_rows = generate_length_encode_number(buf + cursor, &grow);
    cursor += grow;

    pkt->last_insert_id = generate_length_encode_number(buf + cursor, &grow);
    cursor += grow;

    // we use CLIENT_PROTOCOL_41
    memcpy(&pkt->status_flags, buf + cursor, 2);
    cursor += 2;

    memcpy(&pkt->warnings, buf + cursor, 2);
    cursor += 2;

#ifdef DEBUG
    printf("Server Response Packet: %d, %d\n", pkt->length, pkt->id);
    printf("Packet header: %02x\n", pkt->header);
    printf("Affected rows: %ld\n", pkt->affected_rows);
    printf("Last insert ID: %ld\n", pkt->last_insert_id);
    printf("Status Flags: %d\n", pkt->status_flags);
    printf("Warnings: %d\n", pkt->warnings);
    printf("cursor: %d\n", cursor);
#endif

    if (pkt->header == 0xFE) {
        // EOF packet
    }

    if (pkt->header == 0x00 || pkt->header == 0xFE) {
        return pkt->header;
    } else {
        return -1;
    }
}

int parse_column_define_packet(const char *buf)
{
    // lenenc_str     catalog
    // lenenc_str     schema
    // lenenc_str     table
    // lenenc_str     org_table
    // lenenc_str     name
    // lenenc_str     org_name
    // lenenc_int     length of fixed-length fields [0c]
    // 2              character set
    // 4              column length
    // 1              type
    // 2              flags
    // 1              decimals
    // 2              filler [00] [00]
    //   if command was COM_FIELD_LIST {
    //   lenenc_int     length of default-values
    //   string[$len]   default values
    //     }

    int string_length;

    char *catalog;
    catalog = generate_length_encode_string(buf, &string_length);
    buf += string_length;

    char *schema;
    schema = generate_length_encode_string(buf, &string_length);
    buf += string_length;

    char *table;
    table = generate_length_encode_string(buf, &string_length);
    buf += string_length;

    char *org_table;
    org_table = generate_length_encode_string(buf, &string_length);
    buf += string_length;

    char *name;
    name = generate_length_encode_string(buf, &string_length);
    buf += string_length;

    char *org_name;
    org_name = generate_length_encode_string(buf, &string_length);
    buf += string_length;

    buf += 1; // skip byte length of content fixed-length fields

    uint16_t charset;
    memcpy(&charset, buf, 2);
    buf += 2;

    uint32_t column_length;
    memcpy(&column_length, buf, 4);
    buf += 4;

    uint8_t type;
    type = *buf++;

    uint16_t flags;
    memcpy(&flags, buf, 2);
    buf += 2;

    uint8_t decimals;
    decimals = *buf++;

    uint16_t filter = 0x0000;

    // more data if COM_FIELD_LIST command
    // lenenc_int     length of default-values
    // string[$len]   default values

    printf("catalog: %s\n", catalog);
    printf("schema: %s\n", schema);
    printf("table: %s\n", table);
    printf("org_table: %s\n", org_table);
    printf("name: %s\n", name);
    printf("org_name: %s\n", org_name);

    return 0;
}
