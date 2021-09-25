CC := gcc
CFLAGS :=
INSTALL := install
PREFIX := /usr/local
bindir := $(PREFIX)/bin

l2combine: l2combine.c
	$(CC) $(CFLAGS) -Wall l2combine.c -o $@

.PHONY: all clean install

all: l2combine

install: l2combine
	$(INSTALL) -d $(bindir)
	$(INSTALL) l2combine $(bindir)

clean:
	rm -f l2combine
