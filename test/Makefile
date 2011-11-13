target_dir = ../target
classpath = $(target_dir)/jlinuxfork.jar
javac = $(JAVA_HOME)/bin/javac
java = $(JAVA_HOME)/bin/java

runtest: compile
	$(java) -classpath $(classpath):. -Djava.library.path=$(target_dir) -Dposixspawn.binrunner=$(target_dir)/binrunner TestClass


compile: TestClass.class
	$(javac) -classpath $(classpath):. TestClass.java


