CC = gcc

CFLAGS += -DDEBUG -g -Wall
LDFLAGS += -lssl -lcrypto

client: client.o handshake.o packet.o tools.o query.o binary_type.o binlog.o rows_event.o decimal.o debug.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $^

.PHONY: clean

clean:
	-rm *.o client
