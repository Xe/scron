CC=gcc
CFLAGS=-Wall
PREFIX=/usr
BINDIR=${PREFIX}/bin

all: dcron

install: dcron
	mkdir -p ${DESTDIR}${BINDIR}
	install -m 755 dcron ${DESTDIR}${BINDIR}

uninstall:
	rm -f ${DESTDIR}${BINDIR}/dcron

clean:
	rm dcron
