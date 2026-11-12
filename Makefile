CC = gcc
CFLAGS = -Wall -Wextra -std=c11

all: banker

banker: banker.c
	$(CC) $(CFLAGS) -o banker banker.c

clean:
	rm -f banker banker.exe

.PHONY: all clean
