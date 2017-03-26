#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "log.h"

FILE *fp = NULL;
int log_level = 0;
char fmt_buf[1024];

static char *pretty_log_string(int level, char *raw)
{
    char *level_label;

    switch (level) {
    case LOG_DEBUG:
        level_label = "[Debug]";
        break;
    case LOG_INFO:
        level_label = "[Info]";
        break;
    case LOG_WARNING:
        level_label = "[Warning]";
        break;
    case LOG_ERROR:
        level_label = "[Error]";
        break;
    }

    sprintf(fmt_buf, "%s %s", level_label, raw);

    return fmt_buf;
}

int logger(int level, char *fmt, ...)
{
    if (level < log_level) {
        return 0;
    }

    char *log_fmt;
    va_list ap;

    log_fmt = pretty_log_string(level, fmt);

    va_start(ap, fmt);
    vfprintf(fp, log_fmt, ap);
    va_end(ap);

    return 0;
}

void init_logger(const char *file_name, int level)
{
    // open log for write
    fp = fopen(file_name, "a");
    if (!fp) {
        perror("unable to open log file for write");
        exit(2);
    }

    // line buffered log file pointer
    setlinebuf(fp);

    log_level = level;
}
