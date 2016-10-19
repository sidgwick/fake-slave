#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "client.h"
#include "debug.h"

int send_query(struct server_info *info, const char *sql)
{
    char buf[1024];
    int cursor = 4;
    int tmp;

    // COM_QUERY
    *(buf + cursor++) = 0x03;
    // QUERY_STRING
    printf("SQL: %s\n", sql);
    strcpy(buf + cursor, sql);
    cursor += strlen(sql) + 1; // append the '\0' byte.

    tmp = cursor - 4;
    memcpy(buf, &tmp, 3);
    *(buf + 3) = 0;

    write(info->sockfd, buf, cursor);

#ifdef DEBUG
    printf("Query data send to server:\n");
    print_memory(buf, cursor);
#endif
    read(info->sockfd, buf, 1024);

    return 0;
}

int checksum_binlog(struct server_info *info)
{
    char *sql = "SHOW GLOBAL VARIABLES LIKE 'BINLOG_CHECKSUM'";

    send_query(info, sql);

    return 0;
}
