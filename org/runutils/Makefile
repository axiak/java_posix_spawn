includes = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
cc = gcc

all : spawnlib.c spawnlib.h binrunner.c
	$(cc) -O3 -Wall -pedantic -o binrunner binrunner.c
	$(cc) -g -o libspawnlib.so -shared -Wl,-soname,libspawn.so $(includes) spawnlib.c -static -lc

install: binrunner
	sudo mv binrunner /usr/bin
