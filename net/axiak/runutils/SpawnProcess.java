package net.axiak.runutils;

import java.io.*;

public class SpawnProcess {
    native private static SpawnedProcess exec_process(String [] cmdarray, String [] env, String chdir) throws IndexOutOfBoundsException;
    native private static void killProcess(int pid);
    native private static int waitForProcess(int pid);
    private static boolean libLoaded = false;
    private static SpawnProcess instance = null;

    private static class SpawnedProcess extends Process {
        private String name;
        private int exitValue;
        private int pid;
        private boolean exited;

        private OutputStream stdin;
        private InputStream stdout;
        private InputStream stderr;

        SpawnedProcess (String name, int pid, FileDescriptor [] fds) {
            this.pid = pid;
            this.name = name;
            this.exited = false;
            this.exitValue = -1;
            stdin = new FileOutputStream(fds[0]);
            stdout = new FileInputStream(fds[1]);
            stderr = new FileInputStream(fds[2]);
        }

        @Override
        public int exitValue() throws IllegalThreadStateException {
            if (!exited) {
                throw new IllegalThreadStateException("Process has not yet exited.");
            }
            return this.exitValue;
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
        public int waitFor() {
            int retval = waitForProcess(pid);
            if (retval != -500) {
                exitValue = retval;
            }
            this.exited = true;
            return this.exitValue;
        }

        @Override
        public void destroy () {
            killProcess(pid);
        }

        @Override
        public String toString() {
            if (exited) {
                return "[SpawnedProcess pid=" + pid + " exitcode=" + exitValue + "]";
            } else {
                return "[SpawnedProcess pid=" + pid + " exited=false]";
            }
        }
    }

    static {
        try {
            System.loadLibrary("spawnlib");
            libLoaded = true;
        }
        catch (Exception t) {
        }
    }

    public Process exec(String [] cmdarray, String [] envp, String chdir) {
        if (libLoaded) {
            return exec_process(cmdarray, envp, chdir);
        } else {
            return null;
        }
    }

    public Process exec(String [] cmdarray, String [] envp) {
        return exec(cmdarray, envp, ".");
    }

    public Process exec(String [] cmdarray) {
        return exec(cmdarray, new String[0]);
    }

    public Process exec(String command) {
        String[] cmdarray = {command};
        return exec(cmdarray, new String[0]);
    }

    public static SpawnProcess getInstance() {
        if (instance == null) {
            instance = new SpawnProcess();
        }
        return instance;
    }
}
