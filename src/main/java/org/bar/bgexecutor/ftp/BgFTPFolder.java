package org.bar.bgexecutor.ftp;

import java.util.Map;

import org.apache.commons.net.ftp.FTPClient;
import org.bar.bgexecutor.BgExecutor;
import org.bar.bgexecutor.BgExecutor.BgContext;
import org.bar.bgexecutor.BgExecutor.Worker;

public abstract class BgFTPFolder extends BgContext {
    
    public BgFTPFolder(final BgExecutor executor) {
        executor.super();
        this.executor = executor;
    }
    
    protected abstract void connect(FTPClient ftp) throws Exception;
    
    protected abstract void executorStarving(Map<String, Worker> existingWorkersSnapshot, FTPClient ftp)
            throws Exception;
    
    private final ThreadLocal<FTPClientHolder> ftpTls = new ThreadLocal<FTPClientHolder>();
    
    @Override
    protected final void executorStarving(final Map<String, Worker> existingWorkersSnapshot)
            throws Exception {
        try {
            final FTPClientHolder ftpHolder = getFtp();
            executorStarving(existingWorkersSnapshot, ftpHolder.ftp);
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
        protected final void call2() throws Exception {
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
    protected void destroyTls() {
        invalidateFtp();
    }
    
    private final BgExecutor executor;
    
    public BgExecutor getExecutor() {
        return executor;
    }
}
