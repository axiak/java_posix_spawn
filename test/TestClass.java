import net.axiak.runtime.*;
import java.io.*;

class TestClass {
    @SuppressWarnings("unused")
    public static void main(String [] args) {
        String[] memory_soaker = new String[1000000];
        String[] cmd = {"ls", "-l"};

        for (int i = 0; i < memory_soaker.length; i++) {
            memory_soaker[i] = String.valueOf(i);
        }

        System.out.println("");
        System.out.println("");
        System.out.println("");

        try {
            SpawnRuntime runtime = new SpawnRuntime(Runtime.getRuntime());
            if (runtime.isLinuxSpawnLoaded()) {
                System.out.println(" >> java_posix_spawn libraries are found and being tested.");
            } else {
                System.out.println(" >> java_posix_spawn is not built and/or there was an error. Running with fallback.");
            }

            Process result = runtime.exec(cmd);
            int lastChar = 0;
            do {
                lastChar = result.getInputStream().read();
                if (lastChar != -1)
                    System.out.print((char)lastChar);
            } while(lastChar != -1);
        } catch (IOException ignored) {
            System.out.println(ignored);
        }
    }
}
