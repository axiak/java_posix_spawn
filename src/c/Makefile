includes = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
CC ?= cc
INSTALL ?= install

all: binrunner library

library: jlinuxfork.c jlinuxfork.h
	$(CC) -fPIC -Wno-long-long -pedantic -O3 -Wall -g -o libjlinuxfork.so -shared -Wl,-soname,libspawn.so $(includes) jlinuxfork.c -lc

clean:
	rm -fv *.so binrunner

binrunner:
	$(CC) -O3 -Wall -pedantic -o binrunner binrunner.c
	
install: binrunner libjlinuxfork.so
	$(INSTALL) -m 755 binrunner $(DESTDIR)/usr/bin
	$(INSTALL) -m 644 libjlinuxfork.so $(DESTDIR)/usr/lib/
	
