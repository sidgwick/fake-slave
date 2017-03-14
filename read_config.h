#ifndef READ_CONFIG_H
#define READ_CONFIG_H

#include "client.h"

int read_config(server_info *info);
void parse_command_pareters(int argc, char *argv[], server_info *info);

#endif
