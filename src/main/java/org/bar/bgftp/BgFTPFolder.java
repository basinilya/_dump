package org.bar.bgftp;

import org.apache.commons.net.ftp.FTPClient;
import org.bar.bgftp.BgExecutor.BgContext;
import org.bar.bgftp.BgExecutor.Worker;

public abstract class BgFTPFolder extends BgContext<FTPClient> {
    
    public BgFTPFolder(final BgExecutor executor) {
        executor.super();
    }
    
    protected abstract void connect(FTPClient ftp) throws Exception;
    
    protected abstract void submitMoreTasks(FTPClient ftp) throws Exception;
    
    private final ThreadLocal<FTPClientHolder> ftpTls = new ThreadLocal<FTPClientHolder>();
    
    void submitMoreTasks() throws Exception {
        try {
            final FTPClientHolder ftpHolder = getFtp();
            submitMoreTasks(ftpHolder.ftp);
            ftpHolder.setLastUsed(System.nanoTime());
        } catch (final Exception e) {
            invalidateFtp();
            throw e;
        }
    }
    
    private FTPClientHolder getFtp() throws Exception {
        FTPClientHolder ftp = ftpTls.get();
        if (ftp == null) {
            ftp = new FTPClientHolder();
            ftpTls.set(ftp);
            try {
                connect(ftp.ftp);
            } catch (final Exception e) {
                invalidateFtp();
                throw e;
            }
        } else {
            try {
                if (!validateFtp(ftp)) {
                    throw new Exception("validation failed");
                }
            } catch (final Exception e) {
                invalidateFtp();
                return getFtp();
            }
        }
        ftp.setLastUsed(System.nanoTime());
        return ftp;
    }
    
    private boolean validateFtp(final FTPClientHolder ftp) throws Exception {
        final long secondsPassed = (System.nanoTime() - ftp.getLastUsed()) / 1000000000L;
        if (secondsPassed < 30) {
            return true;
        }
        return ftp.validate();
    }
    
    void invalidateFtp() {
        final FTPClientHolder ftp = ftpTls.get();
        if (ftp != null) {
            try {
                ftp.close();
            } catch (final Exception e) {
            }
            ftpTls.set(null);
        }
    }
    
    public abstract class FtpWorker extends Worker {
        
        public FtpWorker(final BgExecutor executor) {
            executor.super();
        }
        
        protected abstract void call(FTPClient ftp) throws Exception;
        
        @Override
        protected void call2() throws Exception {
            try {
                final FTPClientHolder ftpHolder = getFtp();
                call(ftpHolder.ftp);
                ftpHolder.setLastUsed(System.nanoTime());
            } catch (final Exception e) {
                invalidateFtp();
                throw e;
            }
        }
    }
    
    @Override
    protected void threadDying() {
        invalidateFtp();
    }
}
