#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "handshake.h"

int connect_master(server_info *info)
{
    int sockfd;
    struct sockaddr_in server_address;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket: ");
    }
    bzero((char *)&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_aton(info->master.host, &server_address.sin_addr);
    server_address.sin_port = htons(info->master.port);

    if (connect(sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        perror("ERROR connecting: ");
    }

    // save this sock file descriptor to server_info struct.
    info->sockfd = sockfd;
    // now you can comminute with the mysql server.
    handshake_with_server(info);

    return 0;
}
