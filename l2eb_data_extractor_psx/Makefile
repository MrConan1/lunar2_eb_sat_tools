CC := gcc
CFLAGS :=
INSTALL := install
PREFIX := /usr/local
bindir := $(PREFIX)/bin

l2extract_psx: l2extract_psx.c
	$(CC) $(CFLAGS) -Wall l2extract_psx.c -o $@

.PHONY: all clean install

all: l2extract_psx

install: l2extract_psx
	$(INSTALL) -d $(bindir)
	$(INSTALL) l2extract_psx $(bindir)

clean:
	rm -f l2extract_psx
