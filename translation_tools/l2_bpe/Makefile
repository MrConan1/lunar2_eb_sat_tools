CC := gcc
CFLAGS :=
INSTALL := install
PREFIX := /usr/local
bindir := $(PREFIX)/bin

bpe: main.c text_list.c util.c text_list.h util.h
	$(CC) $(CFLAGS) -Wall main.c text_list.c util.c -o $@

.PHONY: all clean install

all: bpe

install: bpe
	$(INSTALL) -d $(bindir)
	$(INSTALL) bpe $(bindir)

clean:
	rm -f bpe
