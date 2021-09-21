CC := gcc
CFLAGS :=
INSTALL := install
PREFIX := /usr/local
bindir := $(PREFIX)/bin

l2_sat_dlog_updater: main.c util.c util.h
	$(CC) $(CFLAGS) -Wall main.c util.c -o $@

.PHONY: all clean install

all: l2_sat_dlog_updater

install: l2_sat_dlog_updater
	$(INSTALL) -d $(bindir)
	$(INSTALL) l2_sat_dlog_updater $(bindir)

clean:
	rm -f l2_sat_dlog_updater
