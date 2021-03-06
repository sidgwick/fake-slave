#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <json.h>

#include "client.h"
#include "log.h"

#define BUFFER_SIZE 1024

static void config_log(json_object *json)
{
    const char *filename = NULL;
    int level = LOG_ERROR;
    json_object_object_foreach(json, key, val) {
        if (strcmp(key, "file") == 0) {
            filename = json_object_get_string(val);
        }

        if (strcmp(key, "level") == 0) {
            level = json_object_get_int(val);
        }
    }

    if (!filename) {
        fprintf(stderr, "which log file do you want to write to?\n");
        exit(2);
    }

    init_logger(filename, level);
}

static int config_master(struct mysql_server *server, json_object *json)
{
    enum json_type item_type;
    const char *tmp_char;
    char *buffer;

    json_object_object_foreach(json, key, val) {
        item_type = json_object_get_type(val);
        if (item_type == json_type_string) {
            tmp_char = json_object_get_string(val);
            buffer = malloc(strlen(tmp_char) + 1);
            strcpy(buffer, tmp_char);
        }

        if (strcmp(key, "host") == 0) {
            server->host = buffer;
            continue;
        }
        if (strcmp(key, "port") == 0) {
            server->port = (short)json_object_get_int(val);
            continue;
        }
        if (strcmp(key, "user") == 0) {
            server->user = buffer;
            continue;
        }
        if (strcmp(key, "password") == 0) {
            server->password = buffer;
            continue;
        }
        if (strcmp(key, "database") == 0) {
            server->database = buffer;
            continue;
        }
        if (strcmp(key, "binlog") == 0) {
            server->binlog = buffer;
            continue;
        }
        if (strcmp(key, "position") == 0) {
            server->position = json_object_get_int(val);
            continue;
        }
        if (strcmp(key, "debug") == 0) {
            server->debug = (short int)json_object_get_boolean(val);
            continue;
        }
    }
    return 0;
}

int read_config(server_info *config)
{
    FILE *fp;
    char *buffer;
    int cursor = 0;
    json_object *json;
    enum json_tokener_error jerr;

    buffer = malloc(sizeof(char) * BUFFER_SIZE);
    fp = fopen("fake-slave.json", "r");
    while ((fread(buffer + cursor, 1, BUFFER_SIZE, fp) == BUFFER_SIZE)) {
        cursor += BUFFER_SIZE;
        // read more
        buffer = realloc(buffer, cursor + BUFFER_SIZE);
    }

    json = json_tokener_parse_verbose(buffer, &jerr);
    if (!json) {
        // error occured
        printf("json parse filed: %s\n", json_tokener_error_desc(jerr));
        printf("%s\n", buffer);
        exit(5);
    }

    json_object_object_foreach(json, key, val) {
        // master configure
        if (strcmp(key, "master") == 0) {
            if (strcmp(key, "master") == 0) {
                config_master(&config->master, val);
            }
            continue;
        }

        // log config
        if (strcmp(key, "log") == 0) {
            config_log(val);
        }
    }

    // free memory
    json_object_put(json);
    free(buffer);
    logger(LOG_INFO, "read config from file completed.\n");
    return 0;
}

static void usage(char *name)
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

// command line configure
void parse_command_pareters(int argc, char *argv[], server_info *info)
{
    char opt;
    // command line parameters
    while ((opt = getopt(argc, argv, "dh:u:p:D:P:")) != -1) {
        switch (opt) {
        case 'd':
            info->master.debug = 1;
            break;
        case 'h':
            info->master.host = optarg;
            break;
        case 'u':
            info->master.user = optarg;
            break;
        case 'p':
            info->master.password = optarg;
            break;
        case 'P':
            info->master.port = atoi(optarg);
            break;
        case 'D':
            info->master.database = optarg;
            break;
        default:
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    logger(LOG_INFO, "read parameters from command line completed.\n");
}
