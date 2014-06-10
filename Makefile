CFLAGS=-O2 -Wall

all: dcron

install: dcron
	mkdir -p ${DESTDIR}/bin
	install -m 755 dcron ${DESTDIR}/bin/
	install -m 755 init.d/dcron ${DESTDIR}/etc/init.d/
	install -m 644 dcron.conf ${DESTDIR}/etc/

uninstall:
	rm -f ${DESTDIR}/bin/dcron
	rm ${DESTDIR}/etc/init.d/dcron
	rm ${DESTDIR}/etc/dcron.conf

clean:
	rm dcron
