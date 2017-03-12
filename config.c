#include <stdio.h>
#include <stdlib.h>
#include <json.h>

#include "client.h"

#define BUFFER_SIZE 1024

int read_config(struct configure *config)
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

    printf("Json:\n%s\n", json_object_to_json_string(json));

    // free memory
    json_object_put(json);
    free(buffer);
    return 0;
}

int main()
{
    server_info info;
    read_config(&info.config);
}
