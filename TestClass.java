import net.axiak.runutils.*;
import java.io.*;

class TestClass {
    @SuppressWarnings("unused")
    public static void main(String [] args) {
        //String[] memory_soaker = new String[10000];
        String[] cmd = {"ls", "-l"};

        try {
            //Runtime runtime = Runtime.getRuntime();
            SpawnRuntime runtime = SpawnRuntime.getInstance();
            /*
            if (File.separatorChar == '/' && !runtime.isLinuxSpawnLoaded()) {
                throw new RuntimeException("Boo!");
            }
            */

            Process result = runtime.exec(cmd);
        System.out.println(result.getInputStream());
	    while (true) {
                System.out.print((char)result.getInputStream().read());
	    }
        } catch (IOException ignored) {
            System.out.println(ignored);
	}
    }
}
