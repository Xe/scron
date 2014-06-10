CFLAGS=-O2 -Wall

all: dcron

install: dcron
	mkdir -p ${DESTDIR}/bin
	install -m 755 dcron ${DESTDIR}/bin/
	install -m 755 init.d/dcron /etc/init.d/

uninstall:
	rm -f ${DESTDIR}/bin/dcron
	rm /etc/init.d/dcron

clean:
	rm dcron
