#ifndef BINLOG_H
#define BINLOG_H

#include "client.h"

#define UNKNOWN_EVENT 0x00
#define START_EVENT_V3 0x01
#define QUERY_EVENT 0x02
#define STOP_EVENT 0x03
#define ROTATE_EVENT 0x04
#define INTVAR_EVENT 0x05
#define LOAD_EVENT 0x06
#define SLAVE_EVENT 0x07
#define CREATE_FILE_EVENT 0x08
#define APPEND_BLOCK_EVENT 0x09
#define EXEC_LOAD_EVENT 0x0a
#define DELETE_FILE_EVENT 0x0b
#define NEW_LOAD_EVENT 0x0c
#define RAND_EVENT 0x0d
#define USER_VAR_EVENT 0x0e
#define FORMAT_DESCRIPTION_EVENT 0x0f
#define XID_EVENT 0x10
#define BEGIN_LOAD_QUERY_EVENT 0x11
#define EXECUTE_LOAD_QUERY_EVENT 0x12
#define TABLE_MAP_EVENT 0x13
#define WRITE_ROWS_EVENTv0 0x14
#define UPDATE_ROWS_EVENTv0 0x15
#define DELETE_ROWS_EVENTv0 0x16
#define WRITE_ROWS_EVENTv1 0x17
#define UPDATE_ROWS_EVENTv1 0x18
#define DELETE_ROWS_EVENTv1 0x19
#define INCIDENT_EVENT 0x1a
#define HEARTBEAT_EVENT 0x1b
#define IGNORABLE_EVENT 0x1c
#define ROWS_QUERY_EVENT 0x1d
#define WRITE_ROWS_EVENTv2 0x1e
#define UPDATE_ROWS_EVENTv2 0x1f
#define DELETE_ROWS_EVENTv2 0x20
#define GTID_EVENT 0x21
#define ANONYMOUS_GTID_EVENT 0x22
#define PREVIOUS_GTIDS_EVENT 0x23

// 4              timestamp
// 1              event type
// 4              server-id
// 4              event-size
// if binlog-version > 1:
// 4              log pos
// 2              flags
struct event_header {
    uint32_t timestamp;
    char event_type;
    uint32_t server_id;
    uint32_t event_size;
    uint32_t log_pos;
    uint16_t flags;
};

/*
Post-header
      if binlog-version > 1 {
    8              position
      }
Payload
    string[p]      name of the next binlog
*/
struct rotate_event {
    struct event_header header;
    uint64_t position;
    char *next_file;
};

/*
2                binlog-version
string[50]       mysql-server version
4                create timestamp
1                event header length
string[p]        event type header lengths
*/
struct format_description_event {
    struct event_header header;
    uint16_t binlog_version;
    char mysql_version[50];
    uint32_t create_timestamp;
    uint8_t event_header_length;
    char *event_types_post_header_length;
};

/*
4              slave_proxy_id
4              execution time
1              schema length
2              error-code
  if binlog-version ≥ 4:
2              status-vars length

string[$len]   status-vars
string[$len]   schema
1              [00]
string[EOF]    query

*/
struct query_event {
    struct event_header header;
    uint32_t slave_proxy_id;
    uint32_t execution_time;
    uint8_t schema_length;
    uint16_t error_code;
    uint16_t status_vars_length;
    char *status_vars;
    char *schema;
    char *query;
};

/*
post-header:
    if post_header_len == 6 {
  4              table id
    } else {
  6              table id
    }
  2              flags

payload:
  1              schema name length
  string         schema name
  1              [00]
  1              table name length
  string         table name
  1              [00]
  lenenc-int     column-count
  string.var_len [length=$column-count] column-def
  lenenc-str     column-meta-def
  n              NULL-bitmask, length: (column-count + 8) / 7
*/
struct table_map_event {
    struct event_header header;
    uint64_t table_id;
    uint16_t flags;
    uint8_t schema_name_length;
    uint8_t table_name_length;
    int column_count;
    char *schema_name;
    char *table_name;
    char *column_def;
    char *column_meta_def;
    char *null_bitmask;
};

struct column_data {
        // string.var_len       nul-bitmap, length (bits set in 'columns-present-bitmap1'+7)/8
        // string.var_len       value of each field as defined in table-map
        //   if UPDATE_ROWS_EVENTv1 or v2 {
        // string.var_len       nul-bitmap, length (bits set in 'columns-present-bitmap2'+7)/8
        // string.var_len       value of each field as defined in table-map
};

struct rows_event {
    struct event_header header;
    struct table_map_event table_map;
    uint64_t table_id;
    uint16_t flags;
    int column_count;
    char *columns_present_bitmap1;
    char *columns_present_bitmap2;
    struct column_data *data;
};

int run_binlog_stream(server_info *);
int get_post_header_length(int);

#endif