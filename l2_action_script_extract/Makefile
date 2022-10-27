CC := gcc
CFLAGS :=
INSTALL := install
PREFIX := /usr/local
bindir := $(PREFIX)/bin

l2_actionExtr: main.c util.c util.h
	$(CC) $(CFLAGS) -Wall main.c util.c -o $@

.PHONY: all clean install

all: l2_actionExtr

install: l2_actionExtr
	$(INSTALL) -d $(bindir)
	$(INSTALL) l2_actionExtr $(bindir)

clean:
	rm -f l2_actionExtr
