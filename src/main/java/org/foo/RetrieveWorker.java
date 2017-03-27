package org.foo;

import java.io.InputStream;

import org.apache.commons.net.ftp.FTPClient;
import org.apache.commons.net.ftp.FTPFile;
import org.bar.bgexecutor.ftp.BgFTPFolder;
import org.bar.bgexecutor.ftp.BgFTPFolder.FtpWorker;

public class RetrieveWorker extends FtpWorker {
    
    private final FTPFile file;
    
    public RetrieveWorker(final BgFTPFolder folder, final FTPFile f) {
        folder.super(folder.getExecutor());
        this.file = f;
    }
    
    @Override
    public void call(final FTPClient ftp) throws Exception {
        progress = 0;
        
        final InputStream is = ftp.retrieveFileStream(file.getName());
        
        try {
            final byte[] buf = new byte[8192];
            int i;
            while ((i = is.read(buf)) != -1) {
                // log("got " + i + " bytes");
                progress(i);
            }
        } finally {
            is.close();
        }
        
        if (!ftp.completePendingCommand()) {
            throw new Exception("something failed");
        }
    }
    
    @Override
    public String toString() {
        return this.file.getName();
    }
    
    public FTPFile getFile() {
        return file;
    }
    
    private long progress = -1;
    
    public synchronized long getProgress() {
        return progress;
    }
    
    private synchronized void progress(final long p) {
        progress += p;
    }
}