target_dir = ../target
classpath = $(target_dir)/jlinuxfork.jar
javac = $(JAVA_HOME)/bin/javac
java = $(JAVA_HOME)/bin/java

# TUNE THESE
XMS = 100m
XMX = 200m


runtest: compile
	$(java) -classpath $(classpath):. -Xms$(XMS) -Xmx$(XMX) -Djava.library.path=$(target_dir) -Dposixspawn.binrunner=$(target_dir)/binrunner TestClass

runfallback: compile
	$(java) -classpath $(classpath):. -Xms$(XMS) -Xmx$(XMX) TestClass


compile: TestClass.class
	$(javac) -classpath $(classpath):. TestClass.java


