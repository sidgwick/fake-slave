#ifndef QUERY_H
#define QUERY_H

#include "client.h"

int send_query(struct server_info *info, const char *sql);
int checksum_binlog(struct server_info *info);

#endif