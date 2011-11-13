import net.axiak.runtime.*;
import java.io.*;

class TestClass {
    @SuppressWarnings("unused")
    public static void main(String [] args) {
        String[] memory_soaker = new String[10000];
        String[] cmd = {"ls", "-l"};

        try {
            SpawnRuntime runtime = new SpawnRuntime(Runtime.getRuntime());
            if (File.separatorChar == '/' && !runtime.isLinuxSpawnLoaded()) {
                throw new RuntimeException("Cannot run because the spawn isn't loaded even though this is not Windows.");
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
