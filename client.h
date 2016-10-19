#ifndef CLIENT_H
#define CLIENT_H

#include <stdint.h>

#define DEBUG 1

struct server_info {
    int sockfd; // server socket
    uint8_t protocol_version;
    char *server_version;
    uint32_t connection_id;
    char *auth_plugin_data;
    char filter;
    uint32_t capability_flag;
    char character_set;
    uint16_t status_flags;
    char auth_plugin_data_part_2_len;
    char *reserved;
    char *auth_plugin_name;
};

#endif