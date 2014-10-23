Synopsis
========

This code attempts to implement some of the ideas mentioned at http://bugs.sun.com/view_bug.do?bug_id=5049299.

Basically, the goal is to avoid a fork() call whenever we need to spawn a process so that a large project
does not have to unnecessarily duplicate its virtual memory usage.

Install
=======

If you read the Theory of Operation, you'll learn the trick. The upshot is that this library requires
three discrete components to work correctly:

1. The JAR (``jlinuxfork.jar``)
2. The dynamic-linked library (``libjlinuxfork-{arch}.so``)
3. The binary redirect program (``binrunner``)

While the JAR file is standard Java and can be dropped in a dependency management via pom or other systems, the other two components make the usage a of this library a little bit trickier.

Simple Install
--------------

The simplest install is to just install the components to the operating system.
This requires no configuration at run time and can be built thus::

    $ git clone https://axiak@github.com/axiak/java_posix_spawn.git
    $ cd java_posix_spawn
    $ ant
    $ cd target
    $ sudo cp libjlinuxfork*so /usr/lib/
    $ sudo cp binrunner /usr/bin
    $ cp jlinuxfork.jar YOUR_PROJECT_LIB


Running the Test
-----------------

To run the test to see that it works, do the following::

    $ git clone clone https://axiak@github.com/axiak/java_posix_spawn.git
    $ cd java_posix_spawn
    $ ant
    $ cd test
    $ make

This should then print out the output of running "ls -l /" without any of the duplication of virtual address space.

The question now is, is it behaving better than the standard java library? To be sure, follow these steps:

1. Start a VM that does not have much memory (e.g. 512MB)
2. Edit test/``Makefile`` to adjust ``Xms`` and ``Xmx`` to make the heap large enough.
3. In the ``test`` directory, run ``make`` to see this library, and run ``make runfallback`` to see the fallback action.


Custom Paths
-----------------

If you don't want to place the libraries in system-level directories, then there
are two java settings you can define when running your java application:

- ``java.system.path`` - Set to the directory containing the ``.so`` file.
- ``posixspawn.binrunner`` - Set to the complete path of your ``binrunner`` file.

For example, to run the ``TestClass`` in the repository, the bundled ``Makefile``, runs the following::

    java -classpath ../target/jlinuxfork.jar:. -Djava.library.path=../target -Dposixspawn.binrunner=../target/binrunner TestClass

Description of Operation
==============================

Problem
---------
In linux, a ``fork`` causes linux to duplicate the allocation of virtual memory
in the virtual memory address space. This works well for lots of small applications spawning each other, but in the java world large applications can spawn small applications in rapid succession and leave the operating system unable to manage the virtual address space.

Solution
----------

The workaround is to use ``vfork`` rather than ``fork`` before spawning another process. With fork, this is how java spawns a process:

1. Create 3 pipes (for stdin, stdout, and stderr of the child process)
2. Fork the process
3. Redirect stdin, stdout, and stderr to the appropriate pipes
4. Run ``execve`` or some equivalent to run a different application (and ``brk``)

In order for ``vfork`` to work in a thread-safe manner, one cannot reference memory
space in the parent process after the ``vfork`` call. (The only legal calls are ``_exit`` and ``execve``. It is not legal to redirect file descriptors after a call to vfork, since we haven't created the new memory address space to redirect yet.) So all we can do in ``vfork`` is run an external program.

The trick to get around this limitation of ``vfork`` is to create an intermediary program, called ``binrunner``. binrunner is a really simple C program that expects the following::

    Usage: binrunner stdin# stdout# stderr# chdir program [argv0 ... ]

So essentially you tell it what file descriptors you want redirected, what directory you want to change into, what program you want to run, and you complete running arguments at the end to pass to the next program. Binrunner will then perform the pipe redirection, call chdir() and then call execvp.

So this is how this library will spawn a new process:

1. Create 3 pipes
2. Call vfork
3. Run binrunner with the pipe information, the new directory, and the program information
4. binrunner redirects the pipes
5. binrunner calls ``chdir()``
6. binrunner calls ``execvp`` to run the application (and ``brk``)



vfork rather than posix_spawn
-----------------------------

The library is actually slightly misnamed -- rather than using ``posix_spawn`` blindly, it actually calls ``vfork`` manually. This is due to that fact that posix_spawn does not produce a useful error in the case where the ``execve``
fails. By calling ``vfork`` manually, the application can safely write an error
to ``stderr`` rather than keeping completely silent.

Documentation
=============

Beyond this README, I put up a shell of the API at http://axiak.github.com/java_posix_spawn/
The API is supposed to mirror that of Java's Runtime, so it shouldn't be anything new. The only
added method is ``SpawnRuntime.isLinuxSpawnLoaded()``, which indicates if the java library was
successfully able to load the dynamic library. On Windows you would expect that method to always
return false.

Dependencies
=============

- Java >= 1.4
- A C compiler
- make
- ant (>=1.7?)

Supported Platforms
=====================

The API is written such that it will fall back to the standard java Runtime API if it cannot load the dynamic libraries. This means that windows can just run the java code without any support for compilation (since its Runtime exec doesn't suffer from the but, it's safe).

As for non-windows systems, this library was tested on linux 32- and 64-bit. No testing has been done on other posix-compliant systems, but the code strictly adheres to posix standards.

License
=======

The library is released in the Modified BSD License. See LICENSE for more detail.

Known Bugs
==========

None at the moment. Please file an issue if you find any.

