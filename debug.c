#include <stdio.h>
#include <string.h>

#include "client.h"
#include "handshake.h"

#ifdef DEBUG
int print_memory(char *mem, int len)
{
    int i = 0;

    while (i < len) {
        printf("%02X ", (unsigned char)mem[i++]);

        if (i % 8 == 0) {
            puts("    ");
        } else if (i % 16 == 0) {
            putchar('\n');
        }
    }

    return i - 1;
}


int print_server_info(struct server_info *info)
{
    printf("Protocol version: %02x\n", info->protocol_version);
    printf("Server version: %s\n", info->server_version);

    printf("Connection ID: %u => ", info->connection_id);
    print_memory((char *)&info->connection_id, 4);
    putchar('\n');

    printf("Auth plugin data : %s => ", info->auth_plugin_data);
    print_memory(info->auth_plugin_data, info->auth_plugin_data_len);
    putchar('\n');
    printf("Auth data plugin len: %d(0x%02X)\n", info->auth_plugin_data_len, info->auth_plugin_data_len);

    printf("Filter: %02x\n", info->filter);
    printf("Capability_flag_1: %d => ", info->capability_flags);
    print_memory((char *)&info->capability_flags, 4);
    putchar('\n');

    printf("Charset: %02X\n", info->character_set);

    printf("Status flag: %d => ", info->status_flags);
    print_memory((char *)&info->status_flags, 2);
    putchar('\n');

    printf("Reversed: ");
    print_memory(info->reserved, 10);
    putchar('\n');

    printf("Auth plugin name: %s => ", info->auth_plugin_name);
    print_memory(info->auth_plugin_name, strlen(info->auth_plugin_name));
    putchar('\n');

    return 0;
}
#else
int print_memory(char *mem, int len) {return 0;}
int print_server_info(struct server_info *info) {return 0;}
#endif
