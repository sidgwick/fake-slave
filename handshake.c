#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <openssl/sha.h>

#include "capability_falgs.h"
#include "character_set.h"
#include "handshake.h"
#include "client.h"

extern int print_memory(char *, int);

static char character_set;
static char auth_plugin_data_len;
static char *auth_plugin_data_part_2;
static char auth_plugin_data_part_1[8];

static int parse_handshake_body(char *buf, int length, int sequence_id, struct server_info *server_info)
{
    print_memory(buf, length); putchar('\n');

    char *buf_start = buf;
    
    uint8_t protocol_version;
    // Protocol version
    protocol_version = *buf++;

    char *server_version;
    // server version 
    server_version = buf;
    buf += strlen(server_version) + 1; // additional null byte

    uint32_t connection_id;
    // connection id
    memcpy(&connection_id, buf, 4);
    buf += sizeof(uint32_t);

    // auth_plugin_data_part_1
    memcpy(auth_plugin_data_part_1, buf, 8);
    buf += 8;

    char filter;
    // filter
    filter = *buf++;

    if (buf - buf_start >= length) {
        // no more data.
        return 0;
    }

    uint16_t capability_flag_1;
    // capability_flag_1
    memcpy(&capability_flag_1, buf, 2);
    buf += 2;

    // character_set
    character_set = *buf++;

    uint16_t status_flags;
    // status_flags
    memcpy(&status_flags, buf, 2);
    buf += 2;

    uint16_t capability_flags_2;
    // capability_flags_2
    memcpy(&capability_flags_2, buf, 2);
    buf += 2;

    // auth_plugin_data_len
    auth_plugin_data_len = *buf++;

    char *reserved;
    // reserved
    reserved = buf;
    buf += 10;

    // auth-plugin-data-part-2
    auth_plugin_data_part_2 = buf;
    buf += MAX(12, auth_plugin_data_len - 9);

    // auth_plugin_name
    char *auth_plugin_name;
    auth_plugin_name = buf;

#ifdef DEBUG
    printf("Protocol version: %02x\n", protocol_version);
    printf("Server version: %s\n", server_version);
    printf("Connection ID: %u => ", connection_id); print_memory((char *)&connection_id, 4); putchar('\n');
    printf("Auth plugin data part 1: %s => ", auth_plugin_data_part_1); print_memory(auth_plugin_data_part_1, 8); putchar('\n');
    printf("Filter: %02x\n", filter);
    printf("Capability_flag_1: %d => ", capability_flag_1); print_memory((char *)&capability_flag_1, 2); putchar('\n');
    printf("Charset: %02X\n", character_set);
    printf("Status flag: %d => ", status_flags); print_memory((char *)&status_flags, 2); putchar('\n');
    printf("Capability_flag_2: %d => ", capability_flags_2); print_memory((char *)&capability_flags_2, 2); putchar('\n');
    printf("Auth data plugin len: %d(0x%02X)\n", auth_plugin_data_len, auth_plugin_data_len);
    printf("Reversed: "); print_memory(reserved, 10); putchar('\n');
    printf("Auth plugin data part 2: %s => ", auth_plugin_data_part_2); print_memory(auth_plugin_data_part_2, MAX(12, auth_plugin_data_len - 9)); putchar('\n');
    printf("Auth plugin name: %s => ", auth_plugin_name); print_memory(auth_plugin_name, strlen(auth_plugin_name)); putchar('\n');
#endif

    return 0;
}

char *calculate_password_sha1(const char *password, unsigned char *buf)
{
    unsigned char *r1 = buf + SHA_DIGEST_LENGTH;
    unsigned char *r2 = buf + SHA_DIGEST_LENGTH * 2;
    unsigned char *r3 = buf + SHA_DIGEST_LENGTH * 3;

    SHA1((unsigned char *)password, strlen(password), buf); // Round 1

    char *auth_plugin_data_part_1 = "_F#yY45N";
    auth_plugin_data_part_2 = "f7!>RbuHFgRD";

    // put the first step result to buf + SHA_DIGEST_LENGTH *2
    // second step result to buf + SHA_DIGEST_LENGTH
    // to avoid copy memory.
    SHA1(buf, SHA_DIGEST_LENGTH, r2); // Round 2

    // concatenate password salt
    memcpy(r1, auth_plugin_data_part_1, 8);
    memcpy(r1 + 8, auth_plugin_data_part_2, MAX(12, auth_plugin_data_len - 9));

    SHA1(r1, SHA_DIGEST_LENGTH * 2, r3); // Round 4
    // one pass password sha1 hash ^ password
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        *(buf + i) ^= *(r3 + i);
    }

    return 0;
}

static int response_handshake_to_server(struct server_info *server_info)
{
    char buf[1024];
    int tmp;
    int cursor = 4;
    int sockfd = server_info->sockfd;

    tmp = CLIENT_PROTOCOL_41;
    memcpy(buf + cursor, &tmp, 4);
    cursor += 4;

    tmp = 0xFFFFFFFF;
    memcpy(buf + cursor, &tmp, 4);
    cursor += 4;

    tmp = CHARSET_UTF_8;
    memcpy(buf + cursor, &character_set, 1);
    cursor += 1;

    // reversed 23 bytes
    memset(buf + cursor, 0, 23);
    cursor += 23;

    memcpy(buf + cursor, "root\0", 5);
    cursor += 5;

    // hash password
    unsigned char *password_hash;
    password_hash = malloc(sizeof(char) * 1024);
    calculate_password_sha1("1111", password_hash);
    *(buf + cursor) = SHA_DIGEST_LENGTH;
    memcpy(buf + cursor + 1, password_hash, SHA_DIGEST_LENGTH);
    free(password_hash);
    cursor += SHA_DIGEST_LENGTH + 1;

    // database
    memcpy(buf + cursor, "test\0", 5);
    cursor += 5;

    // auth plug name
    memcpy(buf + cursor, "mysql_native_password", 21);
    cursor += 21;

    // make a packet.
    // 大小端字节序问题
    tmp = (cursor - 4) | (1 << 24);
    memcpy(buf, &tmp, 4);


    printf("Data send to server: \n");
    print_memory(buf, cursor); putchar('\n');

    write(sockfd, buf, cursor);


    if ((tmp = read(sockfd, buf, 1024)) == -1) {
        perror("Can not read from server: ");
    }
    write(fileno(stdout), buf, tmp);

    return 0;
}

int handshake_with_server(struct server_info *server_info) {
    uint32_t length = 0;
    uint32_t sequence_id = 0;
    int datalen;
    char buf[1024];

    // read the first packet from mysql server.
    datalen = read(server_info->sockfd, buf, 1024);
    if (datalen == -1) {
        printf("Unable to read from the server, server socket is: %d", server_info->sockfd);
        perror(NULL);
        exit(1);
    } else if (datalen == 0) {
        printf("Nothing to read.");
        exit(2);
    }

    memcpy(&length, buf, 3);
    memcpy(&sequence_id, buf + 3, 1);

#ifdef DEBUG
    printf("Package length: %d\n", length);
    printf("Package sequence ID: %d\n", sequence_id);
#endif

    parse_handshake_body(buf + 4, length - 4, sequence_id, server_info);

    response_handshake_to_server(server_info);

    return 0;
}
