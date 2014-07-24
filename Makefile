CC = gcc # x86_64-linux-musl-gcc
CFLAGS = -Os -std=c99 -Wall -Wextra -pedantic -D_POSIX_C_SOURCE=200809L -D_BSD_SOURCE
LDFLAGS = -s # -static
PREFIX = /usr/local

all: crond

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(PREFIX)/etc
	install -m 755 crond $(DESTDIR)$(PREFIX)/bin/
	install -m 644 crontab $(DESTDIR)$(PREFIX)/etc/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/crond
	rm $(DESTDIR)$(PREFIX)/etc/crontab

clean:
	rm -f crond
