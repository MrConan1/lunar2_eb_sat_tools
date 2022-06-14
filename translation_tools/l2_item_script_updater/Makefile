CC := gcc
CFLAGS :=
INSTALL := install
PREFIX := /usr/local
bindir := $(PREFIX)/bin

l2_itemScriptUpdater: main.c util.c util.h
	$(CC) $(CFLAGS) -Wall main.c util.c -o $@

.PHONY: all clean install

all: l2_itemScriptUpdater

install: l2_itemScriptUpdater
	$(INSTALL) -d $(bindir)
	$(INSTALL) l2_itemScriptUpdater $(bindir)

clean:
	rm -f l2_itemScriptUpdater
