package com.crunchtime.utils.runtime;

import java.io.File;
import java.io.IOException;
import java.util.StringTokenizer;

public class SpawnRuntime {
    private Runtime runtime;
    private Boolean linuxSpawnLoaded;
    private static SpawnRuntime instance;

    public SpawnRuntime(Runtime runtime) {
        this.runtime = runtime;
        linuxSpawnLoaded = SpawnedProcess.isLibraryLoaded();
    }

    public Process exec(String [] cmdarray, String [] envp, File chdir) throws IOException {
        if (linuxSpawnLoaded) {
            return new SpawnedProcess(cmdarray, envp, chdir);
        } else {
	        return runtime.exec(cmdarray, envp, chdir);
        }
    }

    public Process exec(String [] cmdarray, String [] envp) throws IOException {
        return exec(cmdarray, envp, new File("."));
    }

    public Process exec(String [] cmdarray) throws IOException {
        return exec(cmdarray, new String[0]);
    }

    public Process exec(String command) throws IOException {
        String[] cmdarray = {command};
        return exec(command, new String[0], new File("."));
    }

    public Process exec(String command, String[] envp, File dir) throws IOException {
        if (command.length() == 0) {
            throw new IllegalArgumentException("Empty command");
        }
        StringTokenizer st = new StringTokenizer(command);
        String[] cmdarray = new String[st.countTokens()];
        for (int i = 0; st.hasMoreTokens(); i++) {
            cmdarray[i] = st.nextToken();
        }
        return exec(cmdarray, envp, dir);
    }

    public Process exec(String command, String[] envp) throws IOException {
        return exec(command, envp, new File("."));
    }

    public static SpawnRuntime getInstance() {
        if (instance == null) {
            instance = new SpawnRuntime(Runtime.getRuntime());
        }
        return instance;
    }

    public Boolean isLinuxSpawnLoaded() {
        return linuxSpawnLoaded;
    }
}
