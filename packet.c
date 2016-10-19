#include <stdio.h>
#include <stdint.h>

#define PACKAGE_LENGTH(x) (((x) & 0xFFFFFF00) >> 8)
#define PACKAGE_SEQUENCE_ID(x) ((x) & 0x000000FF)
#define CAL_PACKAGE_LENGTH_AND_ID(length, id) ((((length) << 8) & 0xFFFFFF00) + ((id) & 0x000000FF))

struct packet_head {
    // int8_t packet_length[3]; /* 3 bytes length */
    // int8_t sequence_id; /* 1 byte sequence id */
    int32_t length_and_id; /* 3 bytes length and 1 byte sequence id */
};

struct ok_packet {
    struct packet_head packet_head;
    int8_t header;
    int8_t *affect_rows;
    int8_t *last_insert_id;
}

int test_packet()
{
    packet p;
    int i;

    for (i = 0; i < 100; i++) {
        p.length_and_id = CAL_PACKAGE_LENGTH_AND_ID(i+i, i);
        p.payload = "This is the payload content";

        printf("[Length] %d\n", PACKAGE_LENGTH(p.length_and_id));
        printf("[Seq id] %d\n", PACKAGE_SEQUENCE_ID(p.length_and_id));
        printf("[Payload] %s\n", p.payload);
    }

    return 0;
}
