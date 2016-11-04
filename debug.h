#ifndef DEBUG_H
#define DEBUG_H

#include "client.h"
#include "binlog.h"

int print_memory(const char *mem, int len);
int print_server_info(server_info *info);
int print_binlog_event_header(struct event_header *header);
int print_binlog_rotate_event(struct rotate_event *event);
int print_binlog_format_description_event(struct format_description_event *fmt_des_ev);
int print_binlog_query_event(struct query_event *query_ev);
int print_binlog_table_map_event(struct table_map_event *ev);

#endif