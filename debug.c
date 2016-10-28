#include <stdio.h>
#include <string.h>

#include "client.h"
#include "handshake.h"

#ifdef DEBUG
int print_memory(char *mem, int len)
{
    int i = 0;

    putchar('\n');
    while (i < len) {
        printf("%02X ", (unsigned char)mem[i++]);

        if (i % 16 == 0) {
            putchar('\n');
        } else if (i % 8 == 0) {
            putchar('\t');
        }
    }

    if (i % 16 != 0) {
        putchar('\n');
    }
    putchar('\n');

    return i - 1;
}


int print_server_info(server_info *info)
{
    printf("Protocol version: %02x\n", info->protocol_version);
    printf("Server version: %s\n", info->server_version);

    printf("Connection ID: %u\n", info->connection_id);
    print_memory((char *)&info->connection_id, 4);

    printf("Auth data plugin len: %d(0x%02X)\n", info->auth_plugin_data_len, info->auth_plugin_data_len);
    printf("Auth plugin data : %s", info->auth_plugin_data);
    print_memory(info->auth_plugin_data, info->auth_plugin_data_len);

    printf("Filter: %02x\n", info->filter);

    printf("Capability_flags: %d", info->capability_flags);
    print_memory((char *)&info->capability_flags, 4);

    printf("Charset: %02X\n", info->character_set);

    printf("Status flag: %d", info->status_flags);
    print_memory((char *)&info->status_flags, 2);

    printf("Reversed: ");
    print_memory(info->reserved, 10);

    printf("Auth plugin name: %s", info->auth_plugin_name);
    print_memory(info->auth_plugin_name, strlen(info->auth_plugin_name));

    return 0;
}

int print_binlog_event_header(struct event_header *header)
{
    printf("\e[32mBinlog event\e[0m\n");
    printf("header.timestamp: %d\n", header->timestamp);
    printf("header.event_type: %02X\n", header->event_type);
    printf("header.event_size: %d\n", header->event_size);
    printf("header.log_pos (the next event position): %d\n", header->log_pos);
    printf("header.falgs: %d\n", header->flags); print_memory(header->flags, 2);

    return 0;
}

int print_binlog_rotate_event(struct rotate_event *event)
{
    printf("ROTATE_EVENT, Header length: %02X\n", get_post_header_length(ROTATE_EVENT));
    printf("Position: %ld\n", event->position);
    printf("Next file name: %s\n", event->next_file);
}

#else
int print_memory(char *mem, int len) {return 0;}
int print_server_info(server_info *info) {return 0;}
int print_binary_event_header(struct event_header *header) {return 0;}
int print_binlog_rotate_event(struct rotate_event *event) {return 0;}
#endif
