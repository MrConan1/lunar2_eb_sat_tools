CC := gcc
CFLAGS :=
INSTALL := install
PREFIX := /usr/local
bindir := $(PREFIX)/bin

l2_cmn_psx_decode: main.c 
	$(CC) $(CFLAGS) -Wall main.c -o $@

.PHONY: all clean install

all: l2_cmn_psx_decode

install: l2_cmn_psx_decode
	$(INSTALL) -d $(bindir)
	$(INSTALL) l2_cmn_psx_decode $(bindir)

clean:
	rm -f l2_cmn_psx_decode
