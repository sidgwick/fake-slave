#ifndef TOOLS_H

#include <stdint.h>

#define TOOLS_H

int16_t read_int2(const char *);
int32_t read_int3(const char *);
int32_t read_int4(const char *);
int64_t read_int8(const char *);

int get_length_encode_number(const char *buf, int *length);
char *get_length_encode_string(const char *buf, int *length);

int set_length_encode_string(char *buf, char *string, int *length);
int int_length(int i);

#endif
