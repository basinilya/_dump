package com.spr.ajwf.commons.bgexecutor.ftp;

import java.util.Map;

import org.apache.commons.net.ftp.FTPClient;

import com.spr.ajwf.commons.bgexecutor.BgExecutor;
import com.spr.ajwf.commons.bgexecutor.BgExecutor.BgContext;
import com.spr.ajwf.commons.bgexecutor.BgExecutor.Task;

public abstract class BgFTPFolder extends BgContext {
    
    public BgFTPFolder(final BgExecutor executor) {
        executor.super();
        this.executor = executor;
    }
    
    protected abstract void connect(FTPClient ftp) throws Exception;
    
    protected abstract void executorStarving(Map<String, Task> existingTasksSnapshot, FTPClient ftp)
            throws Exception;
    
    private final ThreadLocal<FTPClientHolder> ftpTls = new ThreadLocal<FTPClientHolder>();
    
    @Override
    protected final void executorStarving(final Map<String, Task> existingTasksSnapshot)
            throws Exception {
        try {
            final FTPClientHolder ftpHolder = getFtp();
            executorStarving(existingTasksSnapshot, ftpHolder.getFtp());
            ftpHolder.use();
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
                connect(ftp.getFtp());
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
        ftp.use();
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
    
    public abstract class FtpTask extends Task {
        
        public FtpTask(final BgExecutor executor) {
            executor.super();
        }
        
        protected abstract void call(FTPClient ftp) throws Exception;
        
        @Override
        protected final void call2() throws Exception {
            try {
                final FTPClientHolder ftpHolder = getFtp();
                call(ftpHolder.getFtp());
                ftpHolder.use();
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
    
    @Override
    public BgExecutor getExecutor() {
        return executor;
    }
}
