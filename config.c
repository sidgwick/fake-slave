#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <json.h>

#include "client.h"

#define BUFFER_SIZE 1024

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
        }
        if (strcmp(key, "port") == 0) {
            server->port = (short)json_object_get_int(val);
        }
        if (strcmp(key, "user") == 0) {
            server->user = buffer;
        }
        if (strcmp(key, "password") == 0) {
            server->password = buffer;
        }
        if (strcmp(key, "database") == 0) {
            server->database = buffer;
        }
        if (strcmp(key, "debug") == 0) {
            server->debug = (int)json_object_get_boolean(val);
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
    enum json_type item_type;

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
        item_type = json_object_get_type(val);

        switch (item_type) {
        case json_type_null:
            break;
        case json_type_boolean:
            break;
        case json_type_double:
            break;
        case json_type_int:
            break;
        case json_type_array:
            break;
        case json_type_string:
            break;
        case json_type_object:
            if (strcmp(key, "master") == 0) {
                // master configure
                config_master(&config->master, val);
            }
            break;
        }
    }

    // free memory
    json_object_put(json);
    free(buffer);
    return 0;
}
