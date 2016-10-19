#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "tools.h"

int check_ok_packet(const char *buf)
{
    uint32_t length = 0;
    uint8_t id = 0;
    int cursor = 0;
    int grow = 0;

    memcpy(&length, buf, 3);
    cursor += 3;

    id = *(buf + cursor++);

    printf("Server Response Packet: %d, %d\n", length, id);

    // packet body.
    // 07 00 00 02 00 00 00 02    00 00 00
    int header = *(buf + cursor++);
    uint64_t affect_rows = generate_length_encode_number(buf + cursor, &grow);
    cursor += grow;

    uint64_t last_insert_id = generate_length_encode_number(buf + cursor, &grow);
    cursor += grow;

    // we use CLIENT_PROTOCOL_41
    uint16_t status_flags;
    memcpy(&status_flags, buf + cursor, 2);
    cursor += 2;

    uint16_t warnings;
    memcpy(&warnings, buf + cursor, 2);
    cursor += 2;

    printf("Packet header: %02x\n", header);
    printf("Affect rows: %ld\n", affect_rows);
    printf("Last insert ID: %ld\n", last_insert_id);
    printf("Status Flags: %d\n", status_flags);
    printf("Warnings: %d\n", warnings);
    printf("cursor: %d\n", cursor);

    if (header == 0xFE) {
        // EOF packet
    }

    return (header == 0x00) ? 0 : -1;
}
