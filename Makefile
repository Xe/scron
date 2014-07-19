CFLAGS = -Os -std=c99 -Wall -Wextra -pedantic -D_POSIX_C_SOURCE=200809L -D_BSD_SOURCE
LDFLAGS = -s # -static
DESTDIR = /usr/local

all: crond

install: all
	mkdir -p ${DESTDIR}/bin
	mkdir -p ${DESTDIR}/etc
	install -m 755 crond ${DESTDIR}/bin/
	install -m 644 crontab ${DESTDIR}/etc/

uninstall:
	rm -f ${DESTDIR}/bin/crond
	rm ${DESTDIR}/etc/crontab

clean:
	rm -f crond
