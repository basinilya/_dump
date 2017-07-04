package org.foo;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.nio.channels.Channels;
import java.nio.channels.FileChannel;
import java.nio.file.StandardOpenOption;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.TimeUnit;

import org.apache.commons.io.output.NullOutputStream;
import org.apache.commons.net.io.CopyStreamException;
import org.apache.commons.net.io.Util;

public class InterruptibleStreamTest {
    
    private static InputStream newInterruptibleInputFileStream(final File file) throws IOException {
        return Channels.newInputStream(FileChannel.open(file.toPath(), StandardOpenOption.READ));
    }
    
    public static void main(final String[] args) throws Exception {
        
        // FileChannel.open(Paths.get(""), StandardOpenOption.CREATE,
        // StandardOpenOption.TRUNCATE_EXISTING, StandardOpenOption.WRITE);
        final Thread mainThread = Thread.currentThread();
        new Timer(true).schedule(new TimerTask() {
            
            @Override
            public void run() {
                System.out.println("interrupting main thread...");
                mainThread.interrupt();
            }
        }, 4000);
        
        final InputStream in =
                newInterruptibleInputFileStream(new File(
                        "V:\\Distr\\NotForBackup\\CentOS-7-x86_64-DVD-1511.iso"));
        try (InputStream in2 = in) {
            try {
                Util.copyStream(in, new NullOutputStream());
            } catch (final CopyStreamException e) {
                throw e.getIOException();
            }
        } catch (final Exception e) {
            System.out.println("Caught in try-with-resources");
            System.out.println(Thread.interrupted());
            System.out.println(Thread.interrupted());
            throw e;
        }
    }
    
    static class SlowFileInputStream extends FileInputStream {
        
        public SlowFileInputStream(final String name) throws FileNotFoundException {
            super(name);
        }
        
        @Override
        public int read() throws IOException {
            delayNanos(TimeUnit.MILLISECONDS.toNanos(50));
            return super.read();
        }
        
        @Override
        public int read(final byte[] b) throws IOException {
            delayNanos(TimeUnit.MILLISECONDS.toNanos(50));
            return super.read(b);
        }
        
        @Override
        public int read(final byte[] b, final int off, final int len) throws IOException {
            delayNanos(TimeUnit.MILLISECONDS.toNanos(50));
            return super.read(b, off, len);
        }
    }
    
    static void delayNanos(final long nanos) {
        final long end = System.nanoTime() + nanos;
        do {
            Thread.yield();
        } while (System.nanoTime() - end < 0);
    }
}
