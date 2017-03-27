package org.bar.bgftp;

import static org.foo.Main.*;

import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import org.apache.commons.net.ftp.FTPClient;

/*
 * Мысль такая:
 * Если я собираюсь делать листинг нескольких папок в бэкграунде (каждая папка в своем треде), то имеет смысл для каждой папки иметь свой снепшот.
 * Тогда я точно смогу сказать, что снепшот был сделан непосредственно перед листингом и не содержит лишних файлов.
 * Если иметь общий снепшот, то не понятно, когда его очищать 
 */

public abstract class BgFTP {
    
    public void run() throws Exception {
        for (;;) {
            mksnapshotWorkers();
            try {
                final FTPClientHolder ftpHolder = getFtp();
                submitMoreTasks(ftpHolder.ftp);
                ftpHolder.setLastUsed(System.nanoTime());
            } catch (final Exception e) {
                invalidateFtp();
                throw e;
            }
            
            try {
                awaitStarvation(4);
                // No exception. We starved
                break;
            } catch (final TimeoutException e) {
                continue;
            }
        }
        executor.shutdown();
        executor.awaitTermination(Long.MAX_VALUE, TimeUnit.SECONDS);
    }
    
    private void awaitStarvation(final long timeoutSeconds) throws TimeoutException,
            InterruptedException {
        final long begNanos = System.nanoTime();
        final long endNanos = begNanos + (timeoutSeconds * 1000000000L);
        
        mksnapshotWorkers();
        
        for (final Worker worker : workersSnapshot.values()) {
            try {
                long timeoutNanos = endNanos - System.nanoTime();
                if (timeoutNanos <= 0) {
                    timeoutNanos = 0;
                }
                // InterruptedException, ExecutionException, TimeoutException;
                worker.fut.get(timeoutNanos, TimeUnit.NANOSECONDS);
            } catch (final ExecutionException e) {
                log("executor dispatched exception to main thread", e);
            }
        }
    }
    
    public boolean trySubmit(final String key, final Worker ftpWorker) throws Exception {
        synchronized (workersByFilename) {
            if (workersSnapshot.containsKey(key)) {
                return false;
            }
            workersByFilename.put(key, ftpWorker);
            ftpWorker.key = key;
            ftpWorker.fut = executor.submit(ftpWorker);
        }
        return true;
    }
    
    public abstract class Worker implements Callable<Void> {
        
        String key;
        
        Future<Void> fut;
        
        @Override
        public final Void call() throws Exception {
            // Thread.currentThread().setName("RETR " + file.getName());
            // log("worker start: " + file.getName());
            try {
                final FTPClientHolder ftpHolder = getFtp();
                call(ftpHolder.ftp);
                ftpHolder.setLastUsed(System.nanoTime());
            } catch (final Exception e) {
                invalidateFtp();
                throw e;
            } finally {
                synchronized (workersByFilename) {
                    workersByFilename.remove(key);
                }
                log("worker end: " + key);
            }
            
            return null;
        }
        
        protected abstract void call(FTPClient ftp);
        
        @Override
        public String toString() {
            return "" + key;
        }
        
    }
    
    private final Map<String, Worker> workersByFilename = new LinkedHashMap<String, Worker>();
    
    private final HashMap<String, Worker> workersSnapshot = new HashMap<String, Worker>();
    
    private void mksnapshotWorkers() {
        workersSnapshot.clear();
        synchronized (workersByFilename) {
            workersSnapshot.putAll(workersByFilename);
        }
    }
    
    private final ExecutorService executor = Executors.newFixedThreadPool(5, new ThreadFactory() {
        
        @Override
        public Thread newThread(final Runnable r) {
            return new ThreadWithConnDestruction(r, BgFTP.this);
        }
    });
    
    @Override
    protected void finalize() throws Throwable {
        try {
            executor.shutdown();
        } catch (final Exception e) {
        }
        super.finalize();
    }
}
