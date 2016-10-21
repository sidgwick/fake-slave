CC = gcc

CFLAGS += -DDEBUG -g -Wall
LDFLAGS += -lssl -lcrypto

client: client.o handshake.o packet.o tools.o query.o binlog.o debug.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $^

.PHONY: clean

clean:
	-rm *.o client
