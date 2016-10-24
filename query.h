#ifndef QUERY_H
#define QUERY_H

#include "client.h"

#define COM_QUERY 0x03
#define COM_REGISTER_SLAVE 0x15
#define COM_BINLOG_DUMP 0x12

int send_query(server_info *info, const char *sql);
int checksum_binlog(server_info *info);
int register_as_slave(server_info *info);
int send_binlog_dump_request(server_info *info);

#endif
