CC := gcc
CFLAGS :=
INSTALL := install
PREFIX := /usr/local
bindir := $(PREFIX)/bin

l2_vdp1txt_decode: main.c util.c util.h script_node_types.h
	$(CC) $(CFLAGS) -Wall main.c util.c -o $@

.PHONY: all clean install

all: l2_vdp1txt_decode

install: l2_vdp1txt_decode
	$(INSTALL) -d $(bindir)
	$(INSTALL) l2_vdp1txt_decode $(bindir)

clean:
	rm -f l2_vdp1txt_decode
