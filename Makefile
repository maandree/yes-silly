.POSIX:

CC = cc

CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700 -D_GNU_SOURCE
CFLAGS   = -std=c99 -Wall -O2
LDFLAGS  = -s

BIN = yes limit measure


all: $(BIN)

.o:
	$(CC) -o $@ $< $(LDFLAGS)

.c.o:
	$(CC) -c -o $@ $< $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f -- *.o *.su $(BIN)

.SUFFIXES:
.SUFFIXES: .c .o
