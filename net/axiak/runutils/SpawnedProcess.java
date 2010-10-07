package net.axiak.runutils;

import java.io.*;
import java.security.AccessController;
import java.security.PrivilegedAction;

public class SpawnedProcess extends Process {
    private String name;
    private int exitCode;
    private int pid;
    private boolean hasExited;

    private OutputStream stdin;
    private InputStream stdout;
    private InputStream stderr;
    private FileDescriptor stdin_fd;
    public FileDescriptor stdout_fd;
    private FileDescriptor stderr_fd;

    private static boolean libLoaded;
    private static Throwable libLoadError;

    private native int execProcess(String [] cmdarray, String [] env, String chdir,
                                   FileDescriptor stdin_fd, FileDescriptor stdout_fd,
                                   FileDescriptor stderr_fd) throws IndexOutOfBoundsException, IOException;
    private native int waitForProcess(int pid);
    private native void killProcess(int pid);

    static {
        try {
            System.loadLibrary("spawnlib");
            libLoaded = true;
	    }
        catch (Throwable t) {
            libLoaded = false;
            libLoadError = t;
        }
    }

    private static class Gate {

        private boolean exited = false;
        private IOException savedException;

        synchronized void exit() { /* Opens the gate */
           exited = true;
           this.notify();
        }

        synchronized void waitForExit() { /* wait until the gate is open */
            boolean interrupted = false;
            while (!exited) {
                try {
                    this.wait();
                } catch (InterruptedException e) {
                    interrupted = true;
                }
            }
            if (interrupted) {
                Thread.currentThread().interrupt();
            }
        }

        void setException (IOException e) {
            savedException = e;
        }

        IOException getException() {
            return savedException;
        }
    }


    public SpawnedProcess(final String [] cmdarray, final String [] envp, final File chdir) throws IOException {
        stdin_fd = new FileDescriptor();
        stdout_fd = new FileDescriptor();
        stderr_fd = new FileDescriptor();

        final Gate gate = new Gate();

        AccessController.doPrivileged(
                new PrivilegedAction<Object>()  {
                    public Object run() {
                        try {
                            pid = execProcess(cmdarray, envp, chdir.getAbsolutePath(), stdin_fd, stdout_fd, stderr_fd);
                        } catch (IOException e) {
                            gate.setException(e);
                            gate.exit();
                            return null;
                        }
                        stdin = new BufferedOutputStream(new FileOutputStream(stdin_fd));
                        stdout = new BufferedInputStream(new FileInputStream(stdout_fd));
                        stderr = new FileInputStream(stderr_fd);

                        Thread t = new Thread("process reaper") {
                            public void run() {
                                gate.exit();
                                int res = waitForProcess(pid);
                                synchronized (SpawnedProcess.this) {
                                    hasExited = true;
                                    exitCode = res;
                                    SpawnedProcess.this.notifyAll();
                                }
                            }
                        };
                        t.setDaemon(true);
                        t.start();
                        return null;

                    }
                });
        if (gate.getException() != null) {
            throw gate.getException();
        }
    }

    public static boolean isLibraryLoaded() {
        return libLoaded;
    }

    public static Throwable getLibLoadError() {
        return libLoadError;
    }

    @Override
    public synchronized int exitValue() throws IllegalThreadStateException {
        if (!hasExited) {
            throw new IllegalThreadStateException("Process has not yet exited.");
        }
        return this.exitCode;
    }

    @Override
    public InputStream getInputStream () {
        return stdout;
    }

    @Override
    public InputStream getErrorStream() {
        return stderr;
    }

    @Override
    public OutputStream getOutputStream() {
        return stdin;
    }

    @Override
    public synchronized int waitFor() throws InterruptedException {
        while (!hasExited) {
            wait();
        }
        return exitCode;
    }
    
    @Override
    public void destroy () {
        synchronized (this) {
            if (!hasExited) {
                killProcess(pid);
            }
        }
        try {
            stdin.close();
        } catch (IOException e) {}
        try {
            stdout.close();
        } catch (IOException e) {}
        try {
            stderr.close();
        } catch (IOException e) {}
    }

    @Override
    public String toString() {
        if (hasExited) {
            return "[SpawnedProcess pid=" + pid + " exitcode=" + exitCode + "]";
        } else {
            return "[SpawnedProcess pid=" + pid + " exited=false]";
        }
    }
}
