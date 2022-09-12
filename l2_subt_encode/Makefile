CC := gcc
CFLAGS :=
INSTALL := install
PREFIX := /usr/local
bindir := $(PREFIX)/bin

l2_subt_extract: main.c
	$(CC) $(CFLAGS) -Wall main.c -o $@

.PHONY: all clean install

all: l2_subt_encode

install: l2_subt_encode
	$(INSTALL) -d $(bindir)
	$(INSTALL) l2_subt_encode $(bindir)

clean:
	rm -f l2_subt_encode
