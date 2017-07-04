import java.io.File;
import java.nio.file.CopyOption;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;

public class Test {
    
    public static void main(final String[] args) throws Exception {
        final File src = new File("src.txt");
        final File dst = new File("dst.txt");
        src.createNewFile();
        dst.createNewFile();
        // System.out.println(src.renameTo(dst));
        CopyOption[] opts = null;
        opts = new CopyOption[] { StandardCopyOption.REPLACE_EXISTING };
        opts = new CopyOption[] {};
        Files.move(src.toPath(), dst.toPath(), opts);
    }
}
