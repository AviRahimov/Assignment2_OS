CC = gcc
CFLAGS = -lcrypto -lm

all: server async_client

server:
	$(CC) -o server server.c $(CFLAGS)

async_client:
	$(CC) -o async_client async_client.c $(CFLAGS)

clean:
	rm -f server async_client
