CC = gcc
CFLAGS = -Wall -I./include -pthread
SRC = src/proxy.c src/proxy_parse.c src/proxy_filter.c
OBJ = proxy

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OBJ)

clean:
	rm -f $(OBJ) proxy.log