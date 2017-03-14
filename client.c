#include "client.h"
#include "connect.h"
#include "binlog.h"
#include "query.h"
#include "read_config.h"

int main(int argc, char *argv[])
{
    server_info info;

    read_config(&info);
    // use command line parameter(if any) override configure file
    parse_command_pareters(argc, argv, &info);

    // initial connect to master
    connect_master(&info);

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
