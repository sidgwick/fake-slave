#ifndef CLIENT_H
#define CLIENT_H

#include <stdint.h>

#define CLIENT_PROTOCOL_41 0x00000200

#define CLIENT_PLUGIN_AUTH 0x00080000
#define CLIENT_SECURE_CONNECTION 0x00008000
#define CLIENT_CONNECT_WITH_DB 0x00000008
#define CLIENT_DEPRECATE_EOF 0x01000000

#define CHARSET_UTF_8 0x21 // 33, utf-8

#define BUF_SIZE 1024

struct mysql_server {
    int debug;
    char *host;
    char *user;
    char *password;
    char *database;
    short int port;
};

typedef struct {
    struct mysql_server master;
    int sockfd; // server socket
    int8_t protocol_version;
    char *server_version;
    int32_t connection_id;
    char filter;
    int32_t capability_flags;
    char character_set;
    int16_t status_flags;
    char auth_plugin_data_len;
    char *auth_plugin_data; // length of (auth_plugin_data + '\0') == auth_plugin_data_len.
    char reserved[10];
    char *auth_plugin_name;
} server_info;

#endif