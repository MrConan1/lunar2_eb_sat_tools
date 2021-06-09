CC := gcc
CFLAGS :=
INSTALL := install
PREFIX := /usr/local
bindir := $(PREFIX)/bin

l2_txt_decode: main.c util.c util.h
	$(CC) $(CFLAGS) -Wall main.c util.c -o $@

.PHONY: all clean install

all: l2_txt_decode

install: l2_txt_decode
	$(INSTALL) -d $(bindir)
	$(INSTALL) l2_txt_decode $(bindir)

clean:
	rm -f l2_txt_decode
