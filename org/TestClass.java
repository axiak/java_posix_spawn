import runutils.SpawnProcess;
import java.io.*;

class TestClass {
    public static void main(String [] args) {
        String[] memory_soaker = new String[100000000];

        String[] cmd = {"/bin/ls", "-l", "/"};


        try {
            Process result = Runtime.getRuntime().exec(cmd);
            //Process result = SpawnProcess.exec(cmd);

            while (true) {
                System.out.print((char)result.getInputStream().read());
            }
        } catch (IOException ignored) {
            System.out.println(ignored);
        }
    }
}
