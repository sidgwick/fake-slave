#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

struct ok_packet {
    uint32_t length;
    uint8_t id;
    char header;
    uint64_t affected_rows;
    uint64_t last_insert_id;
    uint16_t status_flags;
    uint16_t warnings;
    char *info;
    char *session_state_changes;
    char eof;
};

int parse_ok_packet(const char *);
int parse_column_define_packet(const char *);

#endif
