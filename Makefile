CFLAGS = -Os -std=c99 -Wall -Wextra -pedantic -D_GNU_SOURCE
LDFLAGS = -s -static

all: crond

install: all
	mkdir -p ${DESTDIR}/sbin
	mkdir -p ${DESTDIR}/etc
	install -m 755 crond ${DESTDIR}/sbin/
	install -m 644 crontab ${DESTDIR}/etc/

uninstall:
	rm -f ${DESTDIR}/sbin/crond
	rm ${DESTDIR}/etc/crontab

clean:
	rm -f crond
