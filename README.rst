Synopsis
========

This code attempts to implement some of the ideas mentioned at http://bugs.sun.com/view_bug.do?bug_id=5049299.

Basically, the goal is to avoid a fork() call whenever we need to spawn a process so that a large project
does not have to unnecessarily duplicate its virtual memory usage.

Install / Usage
===============

This is very preliminary, so I'm just going to outline the steps you'd need to get it up and running.::

    $ git clone https://axiak@github.com/axiak/java_posix_spawn.git
    $ cd java_posix_spawn/org/runutils
    $ javac SpawnProcess.java
    $ make
    $ make install # <-- This moves the helper binrunner to /usr/bin/
    $ cd ../
    $ javac TestClass.java
    $ java -Xms400m -Xmx2000m TestClass # <-- This should be tuned


This should then print out the output of running "ls -l /" without any of the duplication of ram.
