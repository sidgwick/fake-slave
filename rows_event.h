#ifndef ROW_EVENT_H

#include <stdint.h>

#include "binlog.h"

#define ROW_EVENT_H

int rows_event(struct rows_event *ev, const char *buf);
char *get_column_meta_def(struct table_map_event ev, int col_num);

#endif
