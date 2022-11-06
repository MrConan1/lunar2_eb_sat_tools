CC := gcc
CFLAGS :=
INSTALL := install
PREFIX := /usr/local
bindir := $(PREFIX)/bin

l2_actionEnc: main.c util.c util.h
	$(CC) $(CFLAGS) -Wall main.c util.c -o $@

.PHONY: all clean install

all: l2_actionEnc

install: l2_actionEnc
	$(INSTALL) -d $(bindir)
	$(INSTALL) l2_actionEnc $(bindir)

clean:
	rm -f l2_actionEnc
