package org.runutils;

import java.io.*;

public class SpawnProcess {
    native public static SpawnedProcess exec(String [] cmdarray, String [] env) throws IndexOutOfBoundsException;
    native private static void killProcess(int pid);
    native private static int waitForProcess(int pid);

    private static class SpawnedProcess extends Process {
        private String name;
        private int exitValue;
        private int pid;
        private boolean exited;

        SpawnedProcess (String name, int pid) {
            this.pid = pid;
            this.name = name;
            this.exited = false;
            this.exitValue = -1;
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
            return null;
        }

        @Override
        public InputStream getErrorStream() {
            return null;
        }

        @Override
        public OutputStream getOutputStream() {
            return null;
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
        System.loadLibrary("spawnlib");
    }

    public static Process exec(String [] cmdarray) {
        return exec(cmdarray, new String[0]);
    }

    public static Process exec(String command) {
        String[] cmdarray = {command};
        return exec(cmdarray, new String[0]);
    }

    public static void main(String args[]) {
        String[] breaker = new String[100000000];
        String[] cmd = {"./test", "3"};
        /*
        try {
            Runtime.getRuntime().exec(cmd);
        } catch (IOException e) {
            System.out.println("Error: " + e);
        }
        */
        SpawnedProcess result = exec(cmd, new String[0]);

        System.out.println("Finished code: " + result.waitFor());
        System.out.println(result.exitValue());
        /*
        try {
            Thread.currentThread().sleep(10000);
        }
        catch (InterruptedException ie) {
        }*/

    }

}
