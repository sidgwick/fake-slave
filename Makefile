CC = gcc

CFLAGS += -DDEBUG -g -Wall `mysql_config --cflags` `pkg-config --cflags json-c`
LDFLAGS += -lssl -lcrypto `mysql_config --libs` `pkg-config --libs json-c`

client: client.o connect.o packet.o tools.o query.o binary_type.o binlog.o rows_event.o decimal.o debug.o read_config.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $^

.PHONY: clean

clean:
	-rm *.o client
