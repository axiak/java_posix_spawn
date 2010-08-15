includes = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
cc = gcc

all : spawnlib.c spawnlib.h
	$(cc) -g -o libspawnlib.so -shared -Wl,-soname,libspawn.so $(includes) spawnlib.c -static -lc
