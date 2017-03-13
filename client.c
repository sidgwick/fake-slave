#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "client.h"
#include "binlog.h"
#include "query.h"

server_info info;

extern int handshake_with_server(server_info *);

void usage(char *name)
{
    fprintf(stderr, "%s\n", name);
    fprintf(stderr,
            "-P <num>      database port\n"
            "-D <database> database name\n"
            "-h <host>     database host\n"
            "-u <user>     database username\n"
            "-p <password> database password\n"
            "-d            enable debug output\n"
            );
}

void parse_command_pareters(int argc, char *argv[])
{
    char opt;
    // command line parameters
    while ((opt = getopt(argc, argv, "dh:u:p:D:P:")) != -1) {
        switch (opt) {
        case 'd':
            info.master.debug = 1;
            break;
        case 'h':
            info.master.host = optarg;
            break;
        case 'u':
            info.master.user = optarg;
            break;
        case 'p':
            info.master.password = optarg;
            break;
        case 'P':
            info.master.port = atoi(optarg);
            break;
        case 'D':
            info.master.database = optarg;
            break;
        default:
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in server_address;

    parse_command_pareters(argc, argv);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket: ");
    }
    bzero((char *)&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_aton(info.master.host, &server_address.sin_addr);
    server_address.sin_port = htons(info.master.port);

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
