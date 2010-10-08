includes = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
cc = gcc

all: binrunner javaclasses library

library: jlinuxfork.c jlinuxfork.h
	$(cc) -Wall -g -o libjlinuxfork.so -shared -Wl,-soname,libspawn.so $(includes) jlinuxfork.c -static -lc


binrunner:
	$(cc) -O3 -Wall -pedantic -o binrunner binrunner.c

javaclasses:
	javac *.java

install: binrunner
	sudo mv binrunner /usr/bin
	sudo cp libjlinuxfork.so /usr/lib/
