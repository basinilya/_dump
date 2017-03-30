package org.foo;

import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import org.apache.commons.net.ftp.FTPClient;
import org.apache.commons.net.ftp.FTPClientConfig;
import org.apache.commons.net.ftp.FTPFile;
import org.apache.commons.net.ftp.FTPReply;

import com.spr.ajwf.commons.bgexecutor.BgExecutor;
import com.spr.ajwf.commons.bgexecutor.BgExecutor.Task;
import com.spr.ajwf.commons.bgexecutor.ftp.BgFTPFolder;
import com.spr.ajwf.commons.logger.Logger;

public final class Main {
    
    private Main() {}
    
    static final Logger LOGGER = new Logger(Main.class);
    
    public static final Set<String> DELETEDFILES = Collections
            .synchronizedSet(new HashSet<String>());
    
    public static void main(final String[] args) throws Exception {
        final BgExecutor ctx = new BgExecutor(5) {
            
            @Override
            protected boolean checkpoint(final Map<String, Task> existingTasksSnapshot,
                    final Set<BgContext> contextsSnapshot, final boolean doneItBefore)
                    throws Exception {
                logProgress(existingTasksSnapshot.values());
                if (!doneItBefore) {
                    return super.checkpoint(existingTasksSnapshot, contextsSnapshot, doneItBefore);
                } else {
                    return !existingTasksSnapshot.isEmpty();
                }
            }
            
        };
        new OurFTPFolder(ctx);
        ctx.run(true);
    }
    
    private static void logProgress(final Collection<Task> tasks) {
        final StringBuilder sb = new StringBuilder("---------------------\n");
        for (final Task task2 : tasks) {
            if (!(task2 instanceof RetrieveTask)) {
                continue;
            }
            final RetrieveTask task = (RetrieveTask) task2;
            final FTPFile file = task.getFile();
            final long progress = task.getProgress();
            if (progress != -1) {
                sb.append(100 * progress / file.getSize()).append("% ").append(file.getName())
                        .append('\n');
            }
        }
        sb.append("---------------------");
        LOGGER.info(sb);
    }
}

class OurFTPFolder extends BgFTPFolder {
    
    private int nIter;
    
    private final String path = "/pub/manyfiles";
    
    private final String login = "anonymous";
    
    private final String server = "192.168.56.150";
    
    OurFTPFolder(final BgExecutor executor) {
        super(executor);
    }
    
    @Override
    protected void executorStarving(final Map<String, Task> existingTasksSnapshot,
            final FTPClient ftp) throws Exception {
        final FTPFile[] files = ftp.listFiles();
        final BgExecutor executor = getExecutor();
        for (int i = 0; i < files.length; i++) {
            final FTPFile file = files[i];
            final String key = mkkey(file.getName());
            
            if (i > 12) {
                break;
            }
            if (Main.DELETEDFILES.contains(key)) {
                continue;
            }
            
            if (!existingTasksSnapshot.containsKey(key)) {
                if (nIter != 0) {
                    Main.LOGGER.info("NOT skipping " + file.getName());
                }
                final RetrieveTask task = new RetrieveTask(this, file);
                executor.trySubmit(key, task);
            } else {
                Main.LOGGER.info("skipping " + file.getName());
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