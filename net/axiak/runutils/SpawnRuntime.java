package net.axiak.runutils;

import java.io.File;
import java.io.IOException;

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
        return exec(cmdarray, new String[0]);
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
