CC := gcc
CFLAGS :=
INSTALL := install
PREFIX := /usr/local
bindir := $(PREFIX)/bin

l2_saveld_encode: main.c bpe_compression.c bpe_compression.h  util.c util.h 
	$(CC) $(CFLAGS) -Wall main.c bpe_compression.c util.c -o $@

.PHONY: all clean install

all: l2_saveld_encode

install: l2_saveld_encode
	$(INSTALL) -d $(bindir)
	$(INSTALL) l2_saveld_encode $(bindir)

clean:
	rm -f l2_saveld_encode
