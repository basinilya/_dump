package org.foo;

import static org.bar.bgexecutor.Log.*;

import java.util.Collection;
import java.util.Map;

import org.apache.commons.net.ftp.FTPClient;
import org.apache.commons.net.ftp.FTPClientConfig;
import org.apache.commons.net.ftp.FTPFile;
import org.apache.commons.net.ftp.FTPReply;
import org.bar.bgexecutor.BgExecutor;
import org.bar.bgexecutor.BgExecutor.Worker;
import org.bar.bgexecutor.ftp.BgFTPFolder;

public class Main {
    
    public static void main(final String[] args) throws Exception {
        final BgExecutor ctx = new BgExecutor(5);
        new OurFTPFolder(ctx);
        ctx.run();
    }
}

class OurFTPFolder extends BgFTPFolder {
    
    private final String path = "/pub/manyfiles";
    
    public OurFTPFolder(final BgExecutor executor) {
        super(executor);
    }
    
    @Override
    protected void submitMoreTasks(final Map<String, Worker> existingTasksSnapshot,
            final FTPClient ftp) throws Exception {
        logProgress(existingTasksSnapshot.values());
        
        final FTPFile[] files = ftp.listFiles();
        for (int i = 0; i < files.length; i++) {
            // if (i > 2) break;
            final FTPFile file = files[i];
            final String key = this + file.getName();
            if (!existingTasksSnapshot.containsKey(key)) {
                if (!existingTasksSnapshot.isEmpty()) {
                    log("NOT skipping " + file.getName());
                }
                final RetrieveWorker worker = new RetrieveWorker(this, file);
                getExecutor().trySubmit(key, worker);
            } else {
                log("skipping " + file.getName());
            }
        }
        
    }
    
    @Override
    protected void connect(final FTPClient ftp) throws Exception {
        final FTPClientConfig config = new FTPClientConfig();
        // config.setXXX(YYY); // change required options
        // for example config.setServerTimeZoneId("Pacific/Pitcairn")
        ftp.configure(config);
        
        int reply;
        final String server = "192.168.56.150";
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
        if (!ftp.login("anonymous", "a@b.org")) {
            throw new Exception("login failed: " + ftp.getReplyString());
        }
        if (!ftp.changeWorkingDirectory(path)) {
            throw new Exception("failed to change directory: " + ftp.getReplyString());
        }
    }
    
    private void logProgress(final Collection<Worker> workers) {
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
    
    @Override
    public String toString() {
        return path + "/";
    }
}