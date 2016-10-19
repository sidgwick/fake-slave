#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "packet.h"
#include "tools.h"

int check_ok_packet(const char *buf)
{
    struct ok_packet pkt;
    int cursor = 0;
    int grow = 0;

    memcpy(&pkt.length, buf, 3);
    cursor += 3;

    pkt.id = *(buf + cursor++);

    // packet body.
    pkt.header = *(buf + cursor++);
    pkt.affected_rows = generate_length_encode_number(buf + cursor, &grow);
    cursor += grow;

    pkt.last_insert_id = generate_length_encode_number(buf + cursor, &grow);
    cursor += grow;

    // we use CLIENT_PROTOCOL_41
    memcpy(&pkt.status_flags, buf + cursor, 2);
    cursor += 2;

    memcpy(&pkt.warnings, buf + cursor, 2);
    cursor += 2;

#ifdef DEBUG
    printf("Server Response Packet: %d, %d\n", pkt.length, pkt.id);
    printf("Packet header: %02x\n", pkt.header);
    printf("Affected rows: %ld\n", pkt.affected_rows);
    printf("Last insert ID: %ld\n", pkt.last_insert_id);
    printf("Status Flags: %d\n", pkt.status_flags);
    printf("Warnings: %d\n", pkt.warnings);
    printf("cursor: %d\n", cursor);
#endif

    if (pkt.header == 0xFE) {
        // EOF packet
    }

    return (pkt.header == 0x00) ? 0 : -1;
}
