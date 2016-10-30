#include <stdio.h>
#include <string.h>

#include "client.h"
#include "handshake.h"
#include "binlog.h"

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
    printf("header.falgs: %d\n", header->flags); print_memory((char *)&header->flags, 2);

    return 0;
}

int print_binlog_rotate_event(struct rotate_event *event)
{
    printf("ROTATE_EVENT, Header length: %02X\n", get_post_header_length(ROTATE_EVENT));
    printf("Position: %ld\n", event->position);
    printf("Next file name: %s\n", event->next_file);

    return 0;
}

int print_binlog_format_description_event(struct format_description_event *fmt_des_ev)
{
    printf("FORMAT_DESCRIPTION_EVENT, Header length: %02X\n", get_post_header_length(FORMAT_DESCRIPTION_EVENT));
    printf("Binlog version: %d\n", fmt_des_ev->binlog_version);
    printf("MySQL version: %s\n", fmt_des_ev->mysql_version);
    printf("Create timestamp: %d\n", fmt_des_ev->create_timestamp);
    printf("Event Header length: %02X\n", fmt_des_ev->event_header_length);
    // printf("Event type header length: ...\n"); print_memory(fmt_des_ev->event_types_post_header_length, rest_length);

    return 0;
}

int print_binlog_query_event(struct query_event *query_ev)
{
    printf("QUERY_EVENT, Header length: %02X\n", get_post_header_length(QUERY_EVENT));
    printf("Slave proxy id: %d\n", query_ev->slave_proxy_id);
    printf("Execution time: %d\n", query_ev->execution_time);
    printf("Schema length: %d\n", query_ev->schema_length);
    printf("Error code: %d\n", query_ev->error_code);
    printf("Status vars length: %d\n", query_ev->status_vars_length);

    printf("status_vars prefix length: %d\n", query_ev->status_vars_length);
    printf("Status vars: %s\n", query_ev->status_vars_length ? query_ev->status_vars : "NULL");
    printf("schema prefix length: %d\n", query_ev->schema_length);
    printf("Schema: %s\n", query_ev->schema_length ? query_ev->schema : "NULL");
    printf("Query: %s\n", query_ev->query);

    return 0;
}

int print_binlog_table_map_event(struct table_map_event *ev)
{
    printf("TABLE_MAP_EVENT, Header length: %02X\n", get_post_header_length(TABLE_MAP_EVENT));
    printf("Table id: %ld\n", ev->table_id);
    printf("Flags: "); print_memory((char *)&ev->flags, 2);
    printf("Schema name length: %d\n", ev->schema_name_length);
    printf("Schema name: %s\n", ev->schema_name);
    printf("Table name length: %d\n", ev->table_name_length);
    printf("Table name: %s\n", ev->table_name);
    printf("Column count: %d\n", ev->column_count);
    printf("Column def: "); print_memory(ev->column_def, ev->column_count);
    // printf("Column meta def: "); print_memory(ev->column_meta_def, tmp_length);

    return 0;
}

#else
int print_memory(char *mem, int len) {return 0;}
int print_server_info(server_info *info) {return 0;}
int print_binlog_event_header(struct event_header *header) {return 0;}
int print_binlog_rotate_event(struct rotate_event *event) {return 0;}
int print_binlog_format_description_event(struct format_description_event *fmt_des_ev) {return 0;}
int print_binlog_query_event(struct query_event *query_ev) {return 0;}
int print_binlog_table_map_event(struct table_map_event *ev) {return 0;}
#endif
