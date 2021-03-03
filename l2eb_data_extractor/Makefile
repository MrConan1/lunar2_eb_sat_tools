CC := gcc
CFLAGS :=
INSTALL := install
PREFIX := /usr/local
bindir := $(PREFIX)/bin

l2extract: l2extract.c
	$(CC) $(CFLAGS) -Wall l2extract.c -o $@

.PHONY: all clean install

all: l2extract

install: l2extract
	$(INSTALL) -d $(bindir)
	$(INSTALL) l2extract $(bindir)

clean:
	rm -f l2extract
