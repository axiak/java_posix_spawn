import net.axiak.runutils.*;
import java.io.*;

class TestClass {
    @SuppressWarnings("unused")
    public static void main(String [] args) {
        //String[] memory_soaker = new String[10000];
        String[] cmd = {"ls", "-l"};
	System.out.println((char)(-1));

        try {
            //Process result = Runtime.getRuntime().exec(cmd);
            Process result = SpawnProcess.getInstance().exec(cmd);

	    //System.out.printf("" + result.getInputStream().available());
	    while (true) {
                System.out.print((char)result.getInputStream().read());
	    }
        } catch (IOException ignored) {
            System.out.println(ignored);
	}
    }
}
