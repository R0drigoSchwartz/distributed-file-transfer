CC      = gcc
CFLAGS  = -Wall -Wextra -g
LDFLAGS = -pthread -lcrypto

TARGETS = server client
COMMON  = utils.o

all: $(TARGETS)

server: server.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

client: client.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c defs.h utils.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean
