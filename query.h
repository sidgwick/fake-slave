#ifndef QUERY_H
#define QUERY_H

#include "client.h"

int send_query(server_info *info, const char *sql);
int checksum_binlog(server_info *info);

#endif