CFLAGS+=-std=c99 -Wall -pedantic

all: crond

install: all
	mkdir -p ${DESTDIR}/sbin
	mkdir -p ${DESTDIR}/etc/init.d
	install -m 755 crond ${DESTDIR}/sbin/
	install -m 755 crond.init ${DESTDIR}/etc/init.d/crond
	install -m 644 crontab ${DESTDIR}/etc/

uninstall:
	rm -f ${DESTDIR}/sbin/crond
	rm ${DESTDIR}/etc/init.d/crond
	rm ${DESTDIR}/etc/crontab

clean:
	rm crond
