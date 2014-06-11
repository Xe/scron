CFLAGS=-std=c99 -Wall -Wpedantic -Wextra

all: dcron

install: dcron
	mkdir -p ${DESTDIR}/sbin
	mkdir -p ${DESTDIR}/etc/init.d
	install -m 755 dcron ${DESTDIR}/sbin/
	install -m 755 init.d/dcron ${DESTDIR}/etc/init.d/
	install -m 644 dcron.conf ${DESTDIR}/etc/

uninstall:
	rm -f ${DESTDIR}/sbin/dcron
	rm ${DESTDIR}/etc/init.d/dcron
	rm ${DESTDIR}/etc/dcron.conf

clean:
	rm dcron
