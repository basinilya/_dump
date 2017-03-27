package org.foo;

import static org.bar.bgexecutor.Log.*;

import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import org.apache.commons.net.ftp.FTPClient;
import org.apache.commons.net.ftp.FTPClientConfig;
import org.apache.commons.net.ftp.FTPFile;
import org.apache.commons.net.ftp.FTPReply;
import org.bar.bgexecutor.BgExecutor;
import org.bar.bgexecutor.BgExecutor.Worker;
import org.bar.bgexecutor.ftp.BgFTPFolder;

public class Main {
    
    public static final Set<String> deletedFiles = Collections
            .synchronizedSet(new HashSet<String>());
    
    public static void main(final String[] args) throws Exception {
        final BgExecutor ctx = new BgExecutor(5) {
            
            @Override
            protected boolean checkpoint(final Map<String, Worker> existingWorkersSnapshot,
                    final boolean firstTime) throws Exception {
                logProgress(existingWorkersSnapshot.values());
                if (firstTime) {
                    return super.checkpoint(existingWorkersSnapshot, firstTime);
                } else {
                    return !existingWorkersSnapshot.isEmpty();
                }
            }
            
        };
        new OurFTPFolder(ctx);
        ctx.run();
    }
    
    private static void logProgress(final Collection<Worker> workers) {
        final StringBuilder sb = new StringBuilder("---------------------\n");
        for (final Worker _worker : workers) {
            if (!(_worker instanceof RetrieveWorker)) {
                continue;
            }
            final RetrieveWorker worker = (RetrieveWorker) _worker;
            final FTPFile file = worker.getFile();
            final long progress = worker.getProgress();
            if (progress != -1) {
                sb.append(100 * progress / file.getSize()).append("% ").append(file.getName())
                        .append('\n');
            }
        }
        sb.append("---------------------");
        log(sb);
    }
}

class OurFTPFolder extends BgFTPFolder {
    
    private int nIter;
    
    private final String path = "/pub/manyfiles";
    
    private final String login = "anonymous";
    
    private final String server = "192.168.56.150";
    
    public OurFTPFolder(final BgExecutor executor) {
        super(executor);
    }
    
    @Override
    protected void executorStarving(final Map<String, Worker> existingWorkersSnapshot,
            final FTPClient ftp) throws Exception {
        final FTPFile[] files = ftp.listFiles();
        final BgExecutor executor = getExecutor();
        for (int i = 0; i < files.length; i++) {
            final FTPFile file = files[i];
            final String key = mkkey(file.getName());
            
            if (i > 7) {
                break;
            }
            if (Main.deletedFiles.contains(key)) {
                continue;
            }
            
            if (!existingWorkersSnapshot.containsKey(key)) {
                if (nIter != 0) {
                    log("NOT skipping " + file.getName());
                }
                final RetrieveWorker worker = new RetrieveWorker(this, file);
                executor.trySubmit(key, worker);
            } else {
                log("skipping " + file.getName());
            }
        }
        
        nIter++;
    }
    
    @Override
    protected void connect(final FTPClient ftp) throws Exception {
        final FTPClientConfig config = new FTPClientConfig();
        // config.setXXX(YYY); // change required options
        // for example config.setServerTimeZoneId("Pacific/Pitcairn")
        ftp.configure(config);
        
        int reply;
        ftp.connect(server);
        // log("Connected to " + server + ".");
        // log(ftp.getReplyString()); TODO: trim trailing newline
        
        // After connection attempt, you should check the reply code to
        // verify
        // success.
        reply = ftp.getReplyCode();
        
        if (!FTPReply.isPositiveCompletion(reply)) {
            ftp.disconnect();
            throw new Exception("FTP server refused connection.");
        }
        if (!ftp.login(login, "a@b.org")) {
            throw new Exception("login failed: " + ftp.getReplyString());
        }
        if (!ftp.changeWorkingDirectory(path)) {
            throw new Exception("failed to change directory: " + ftp.getReplyString());
        }
    }
    
    private String mkkey(final String filename) {
        return toString() + filename;
    }
    
    @Override
    public String toString() {
        return login + "@" + server + path + "/";
    }
}