#ifndef CLIENT_H
#define CLIENT_H

#include <stdint.h>

#define DEBUG 1

struct server_info {
    int sockfd; // server socket
    uint8_t protocol_version;
    char *server_version;
    uint32_t connection_id;
    char filter;
    uint32_t capability_flags;
    char character_set;
    uint16_t status_flags;
    char auth_plugin_data_len;
    char *auth_plugin_data; // length of (auth_plugin_data + '\0') == auth_plugin_data_len.
    char reserved[10];
    char *auth_plugin_name;
};

#endif