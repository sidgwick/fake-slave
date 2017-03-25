#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/sha.h>

#include "packet.h"
#include "client.h"
#include "tools.h"

#ifdef DEBUG
#include "debug.h"
#endif

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static int parse_handshake_body(char *buf, int length, int sequence_id, server_info *info)
{
    char *buf_start = buf;
    char auth_plugin_data[BUF_SIZE];

    // Protocol version
    info->protocol_version = *buf++;

    // server version
    info->server_version = malloc(sizeof(char) * strlen(buf) + 1);
    strcpy(info->server_version, buf);
    buf += strlen(info->server_version) + 1; // additional null byte

    // connection id
    info->connection_id = read_int4(buf);
    buf += 4;

    // auth_plugin_data_part_1
    memcpy(auth_plugin_data, buf, 8);
    buf += 8;

    // filter
    info->filter = *buf++;

    if (buf - buf_start >= length) {
        // no more data.
        return 0;
    }

    // capability_flag_1
    memcpy((char *)(&info->capability_flags), buf, 2);
    buf += 2;

    // character_set
    info->character_set = *buf++;

    // status_flags
    info->status_flags = read_int2(buf);
    buf += 2;

    // capability_flags_2
    memcpy((char *)(&info->capability_flags + 2), buf, 2);
    buf += 2;

    // auth_plugin_data_len
    info->auth_plugin_data_len = *buf++;

    // reserved
    memcpy(info->reserved, buf, 10);
    buf += 10;

    // auth-plugin-data-part-2
    memcpy(auth_plugin_data + 8, buf, MAX(13, info->auth_plugin_data_len - 8));
    buf += MAX(13, info->auth_plugin_data_len - 8);
    info->auth_plugin_data = malloc(sizeof(char) * info->auth_plugin_data_len);
    memcpy(info->auth_plugin_data, auth_plugin_data, info->auth_plugin_data_len);

    // auth_plugin_name
    info->auth_plugin_name = malloc(sizeof(char) * strlen(buf));
    strcpy(info->auth_plugin_name, buf);

    fprintf(logfp, "[Info] parse handshake package completed\n");

    return 0;
}

static char *calculate_password_sha1(const server_info *info, const char *password, unsigned char *buf)
{
    unsigned char *r1 = buf + SHA_DIGEST_LENGTH;
    unsigned char *r2 = buf + SHA_DIGEST_LENGTH * 2;
    unsigned char *r3 = buf + SHA_DIGEST_LENGTH * 3;

    SHA1((unsigned char *)password, strlen(password), buf); // Round 1

    // put the first step result to buf + SHA_DIGEST_LENGTH *2
    // second step result to buf + SHA_DIGEST_LENGTH
    // to avoid copy memory.
    SHA1(buf, SHA_DIGEST_LENGTH, r2); // Round 2

    // concatenate password salt
    memcpy(r1, info->auth_plugin_data, info->auth_plugin_data_len - 1); // do not include the '\0' term

    SHA1(r1, SHA_DIGEST_LENGTH * 2, r3); // Round 4
    // one pass password sha1 hash ^ password
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        *(buf + i) ^= *(r3 + i);
    }

    return 0;
}

static int response_handshake_to_server(server_info *info)
{
    char buf[BUF_SIZE];
    int tmp;
    int cursor = 4;
    int sockfd = info->sockfd;

    // capability flags
    tmp = CLIENT_PROTOCOL_41 | CLIENT_SECURE_CONNECTION | CLIENT_CONNECT_WITH_DB;
    memcpy(buf + cursor, &tmp, 4);
    cursor += 4;

    // max-packet size
    tmp = 0xFFFFFFFF;
    memcpy(buf + cursor, &tmp, 4);
    cursor += 4;

    // character set
    tmp = CHARSET_UTF_8;
    memcpy(buf + cursor, &info->character_set, 1);
    cursor += 1;

    // reversed 23 bytes
    memset(buf + cursor, 0, 23);
    cursor += 23;

    // user name
    tmp = strlen(info->master.user) + 1; // copy w/ null byte
    memcpy(buf + cursor, info->master.user, tmp);
    cursor += tmp;

    // hash password
    unsigned char *password_hash;
    password_hash = malloc(sizeof(char) * BUF_SIZE);
    calculate_password_sha1(info, info->master.password, password_hash);
    // CLIENT_SECURE_CONNECTION set, password hash length is the SHA_DIGEST_LENGTH.
    *(buf + cursor) = SHA_DIGEST_LENGTH;
    // copy password hash to buffer.
    memcpy(buf + cursor + 1, password_hash, SHA_DIGEST_LENGTH);
    free(password_hash);
    cursor += SHA_DIGEST_LENGTH + 1;

    // CLIENT_CONNECT_WITH_DB set, database
    tmp = strlen(info->master.database) + 1;
    memcpy(buf + cursor, info->master.database, tmp);
    cursor += tmp;

    // CLIENT_SECURE_CONNECTION auth plug name
    memcpy(buf + cursor, "mysql_native_password", 21);
    cursor += 21;

    // make a packet.
    // 大小端字节序问题
    tmp = (cursor - 4) | (1 << 24);
    memcpy(buf, &tmp, 4);

    // send handshake response to server.
    write(sockfd, buf, cursor);

    fprintf(logfp, "[Info] handshake response has been sent to server.\n");

    // make soure we login success
    if ((tmp = read(sockfd, buf, BUF_SIZE)) == -1) {
        perror("Can not read from server: ");
        exit(1);
    }

    // server will response a OK_Packet if we success login.
    // check OK packet.
    ok_packet ok_pkt;
    if (parse_ok_packet(buf, &ok_pkt) == -1 || ok_pkt.header == 0xFE) {
        fprintf(stderr, "Unexpect response handshake packet, expect OK_Packet.\n");
        exit(2);
    }

    fprintf(logfp, "[Info] handshake master response ok.\n");

    return 0;
}

static int handshake_with_server(server_info *info) {
    uint32_t length = 0;
    uint32_t sequence_id = 0;
    int datalen;
    char buf[BUF_SIZE];

    // read the first packet from mysql server.
    datalen = read(info->sockfd, buf, BUF_SIZE);
    if (datalen == -1) {
        fprintf(stderr, "Unable to read from the server, server socket is: %d", info->sockfd);
        perror(NULL);
        exit(1);
    } else if (datalen == 0) {
        fprintf(stderr, "Nothing to read.");
        exit(2);
    }

    length = read_int3(buf);
    sequence_id = *(int *)(buf + 3);

    fprintf(logfp, "[Info] handshake with master package length: %d\n", length);
    fprintf(logfp, "[Info] handshake with master package sequence ID: %d\n", sequence_id);

    parse_handshake_body(buf + 4, length - 4, sequence_id, info);

    response_handshake_to_server(info);

    return 0;
}

int connect_master(server_info *info)
{
    int sockfd;
    struct sockaddr_in server_address;

    if (!info->master.host || !info->master.port) {
        fprintf(stderr, "Configure error: check your master server host/port configure");
        exit(2);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket: ");
    }

    bzero((char *)&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_aton(info->master.host, &server_address.sin_addr);
    server_address.sin_port = htons(info->master.port);

    if (connect(sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        perror("ERROR connecting: ");
    }

    // save this sock file descriptor to server_info struct.
    info->sockfd = sockfd;
    // now you can comminute with the mysql server.
    handshake_with_server(info);

    return 0;
}
