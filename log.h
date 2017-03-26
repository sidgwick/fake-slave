#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_WARNING 2
#define LOG_ERROR 3

int logger(int level, char *fmt, ...);
void init_logger(const char *filename, int log_level);

#endif
