#ifndef TOOLS_H
#define TOOLS_H

int get_length_encode_number(const char *buf, int *length);
char *get_length_encode_string(const char *buf, int *length);

int set_length_encode_string(char *buf, char *string, int *length);
int int_length(int i);

#endif
