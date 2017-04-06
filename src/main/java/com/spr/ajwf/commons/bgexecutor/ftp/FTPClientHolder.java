package com.spr.ajwf.commons.bgexecutor.ftp;

import java.io.IOException;

import org.apache.commons.net.ftp.FTPClient;
import org.apache.commons.net.ftp.FTPReply;

import com.spr.ajwf.commons.logger.Logger;

class FTPClientHolder {
    
    private long lastUsed;
    
    private final FTPClient ftp = new FTPClient();
    
    public boolean validate() throws Exception {
        final int reply = ftp.noop();
        lastUsed = System.nanoTime();
        return FTPReply.isPositiveCompletion(reply);
    }
    
    public void close() throws Exception {
        LOGGER.info("close ftp client");
        try {
            ftp.logout();
        } finally {
            if (ftp.isConnected()) {
                try {
                    ftp.disconnect();
                } catch (final IOException ioe) {
                    // do nothing
                }
            }
        }
    }
    
    public long getLastUsed() {
        return lastUsed;
    }
    
    public void use() {
        this.lastUsed = System.nanoTime();
    }
    
    private static final Logger LOGGER = new Logger(FTPClientHolder.class);
    
    public FTPClient getFtp() {
        return ftp;
    }
}
