#ifndef CLIENT_H
#define CLIENT_H

#include <stdint.h>

#define BUF_SIZE 1024

struct configure {
    int debug;
    char *host;
    char *user;
    char *password;
    char *database;
    short int port;
};

typedef struct {
    struct configure config;
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