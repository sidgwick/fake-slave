#ifndef DECIMAL_H
#define DECIMAL_H

#include "binary_type.h"

#define DIG_PER_DEC1 9

bin_decimal decimal_number(const char *buf, unsigned char precision, unsigned char scale, int *cursor);

#endif
