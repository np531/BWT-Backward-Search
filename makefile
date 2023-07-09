CC = gcc
CFLAGS = -g

clean:
	rm -f bwtsearch

bwtsearch: bwtsearch.c
	cc bwtsearch.c -o bwtsearch

