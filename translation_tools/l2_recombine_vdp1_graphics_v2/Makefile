CC := gcc
CFLAGS :=
INSTALL := install
PREFIX := /usr/local
bindir := $(PREFIX)/bin

l2replaceMenuGraphics: main.c
	$(CC) $(CFLAGS) -Wall main.c -o $@

.PHONY: all clean install

all: l2replaceMenuGraphics

install: l2replaceMenuGraphics
	$(INSTALL) -d $(bindir)
	$(INSTALL) l2replaceMenuGraphics $(bindir)

clean:
	rm -f l2replaceMenuGraphics
