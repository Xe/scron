CFLAGS=-std=c99 -Wall -Wpedantic -Wextra

all: cron

install: cron
	mkdir -p ${DESTDIR}/sbin
	mkdir -p ${DESTDIR}/etc/init.d
	install -m 755 cron ${DESTDIR}/sbin/
	install -m 755 init.d/cron ${DESTDIR}/etc/init.d/
	install -m 644 crontab ${DESTDIR}/etc/

uninstall:
	rm -f ${DESTDIR}/sbin/cron
	rm ${DESTDIR}/etc/init.d/cron
	rm ${DESTDIR}/etc/crontab

clean:
	rm cron
