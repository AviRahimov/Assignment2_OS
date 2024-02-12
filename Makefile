CC=gcc
# CFLAGS=-Wall
LIBS=-lcrypto -lm

# Default target
all: server client

# Build server only
server:
	$(CC) $(CFLAGS) -o server server.c $(LIBS)

# Build client only
client:
	$(CC) $(CFLAGS) -o client client.c $(LIBS)

# Clean up binaries
clean:
	rm -f server client

.PHONY: all server client clean
