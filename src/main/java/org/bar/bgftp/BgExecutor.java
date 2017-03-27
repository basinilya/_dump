package org.bar.bgftp;

import static org.foo.Main.*;

import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/*
 * Мысль такая:
 * Если я собираюсь делать листинг нескольких папок в бэкграунде (каждая папка в своем треде), то имеет смысл для каждой папки иметь свой снепшот.
 * Тогда я точно смогу сказать, что снепшот был сделан непосредственно перед листингом и не содержит лишних файлов.
 * Если иметь общий снепшот, то не понятно, когда его очищать 
 */

public abstract class BgExecutor {
    
    public void run() throws Exception {
        for (;;) {
            synchronized (workersByFilename) {
                for (final BgContext ctx : contexts) {
                    trySubmit("submitMoreTasks " + ctx, new Worker() {
                        
                        @Override
                        protected void call2() throws Exception {
                            ctx.submitMoreTasks();
                        }
                    });
                }
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
        threadDying();
    }
    
    private void awaitStarvation(final long timeoutSeconds) throws TimeoutException,
            InterruptedException {
        final long begNanos = System.nanoTime();
        final long endNanos = begNanos + (timeoutSeconds * 1000000000L);
        
        final Map<String, BgExecutor.Worker> workersSnapshot = mksnapshotWorkers();
        
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
    
    boolean trySubmit(final String key, final Worker ftpWorker) throws Exception {
        synchronized (workersByFilename) {
            final Worker prev = workersByFilename.put(key, ftpWorker);
            if (prev != null) {
                workersByFilename.put(key, prev);
                return false;
            }
            ftpWorker.key = key;
            ftpWorker.fut = executor.submit(ftpWorker);
        }
        return true;
    }
    
    public abstract class BgContext {
        
        protected abstract void submitMoreTasks() throws Exception;
        
        protected void threadDying() {}
    }
    
    public abstract class Worker implements Callable<Void> {
        
        String key;
        
        Future<Void> fut;
        
        @Override
        public final Void call() throws Exception {
            Thread.currentThread().setName("Bg " + key);
            // log("worker start: " + file.getName());
            try {
                call2();
            } finally {
                synchronized (workersByFilename) {
                    workersByFilename.remove(key);
                }
                log("worker end: " + key);
                Thread.currentThread().setName("Bg idle " + Thread.currentThread().getId());
            }
            
            return null;
        }
        
        protected abstract void call2() throws Exception;
        
        @Override
        public String toString() {
            return "" + key;
        }
        
    }
    
    private final Map<String, Worker> workersByFilename = new LinkedHashMap<String, Worker>();
    
    // private final HashMap<String, Worker> workersSnapshot = new HashMap<String, Worker>();
    
    protected final Map<String, Worker> mksnapshotWorkers() {
        synchronized (workersByFilename) {
            return new HashMap<String, BgExecutor.Worker>(workersByFilename);
        }
    }
    
    private final Set<BgContext> contexts = new HashSet<BgContext>();
    
    private final ExecutorService executor = Executors.newFixedThreadPool(5, new ThreadFactory() {
        
        @Override
        public Thread newThread(final Runnable r) {
            return new ThreadWithTlsDestruction(r, BgExecutor.this);
        }
    });
    
    @Override
    protected void finalize() throws Throwable {
        log(this + " finalize()");
        try {
            executor.shutdown();
        } catch (final Exception e) {
        }
        super.finalize();
    }
    
    void threadDying() {
        HashSet<BgContext> contextsSnapshot;
        synchronized (workersByFilename) {
            contextsSnapshot = new HashSet<BgContext>(contexts);
        }
        for (final BgContext ctx : contextsSnapshot) {
            ctx.threadDying();
        }
        
    }
}
