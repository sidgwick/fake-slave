#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "client.h"
#include "binlog.h"
#include "query.h"

server_info info;

extern int handshake_with_server(server_info *);

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in server_address;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket: ");
    }
    bzero((char *)&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_aton("127.0.0.1", &server_address.sin_addr);
    server_address.sin_port = htons(3306);

    if (connect(sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        perror("ERROR connecting: ");
    }

    // save this sock file descriptor to server_info struct.
    info.sockfd = sockfd;
    // now you can comminute with the mysql server.
    handshake_with_server(&info);

    // send checksum query to server.
    checksum_binlog(&info);

    // register as a slave.
    register_as_slave(&info);

    // send binlog dump request
    send_binlog_dump_request(&info);

    // binlog event loop
    run_binlog_stream(&info);

    return 0;
}
