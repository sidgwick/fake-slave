#ifndef DECIMAL_H
#define DECIMAL_H

#define DIG_PER_DEC1 9

char *decimal_number(const char *buf, unsigned char precision, unsigned char scale, int *cursor);

#endif
