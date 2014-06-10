CFLAGS=-O2 -Wall

all: dcron

install: dcron
	mkdir -p ${PREFIX}/bin
	install -m 755 dcron ${PREFIX}/bin/
	install -m 755 init.d/dcron /etc/init.d/

uninstall:
	rm -f ${PREFIX}/dcron
	rm /etc/init.d/dcron

clean:
	rm dcron
