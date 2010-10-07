includes = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
cc = gcc

all : spawnlib.c spawnlib.h binrunner javaclasses
	$(cc) -Wall -g -o libspawnlib.so -shared -Wl,-soname,libspawn.so $(includes) spawnlib.c -static -lc

binrunner:
	$(cc) -O3 -Wall -pedantic -o binrunner binrunner.c

javaclasses:
	javac *.java

install: binrunner
	sudo mv binrunner /usr/bin
	sudo cp libspawnlib.so /usr/lib/
