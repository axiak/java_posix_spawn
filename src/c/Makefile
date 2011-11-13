includes = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
cc = cc

all: binrunner library

library: jlinuxfork.c jlinuxfork.h
	$(cc) -fPIC -Wno-long-long -pedantic -O3 -Wall -g -o libjlinuxfork.so -shared -Wl,-soname,libspawn.so $(includes) jlinuxfork.c -lc

clean:
	rm -fv *.so binrunner

binrunner:
	$(cc) -O3 -Wall -pedantic -o binrunner binrunner.c
	chmod -v +x binrunner

install: binrunner libjlinuxfork.so
	sudo mv binrunner /usr/bin
	sudo cp libjlinuxfork.so /usr/lib/
