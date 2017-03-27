package org.bar.bgexecutor;

import static org.bar.bgexecutor.Log.*;

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
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

/**
 * Wrapper for {@link ThreadPoolExecutor} that
 */
public class BgExecutor {
    
    public BgExecutor(final int nThreads) {
        executor = Executors.newFixedThreadPool(nThreads, new ThreadFactory() {
            
            @Override
            public Thread newThread(final Runnable r) {
                return new ThreadWithTlsDestruction(r, BgExecutor.this);
            }
        });
    }
    
    public void run() throws Exception {
        boolean firstTime = true;
        for (;;) {
            synchronized (workersByFilename) {
                if (!checkpoint(workersByFilename, firstTime)) {
                    break;
                }
            }
            firstTime = false;
            
            try {
                awaitStarvation(4);
                // No exception. We starved
            } catch (final TimeoutException e) {
                //
            }
        }
        executor.shutdown();
        executor.awaitTermination(Long.MAX_VALUE, TimeUnit.SECONDS);
        destroyTls();
    }
    
    protected boolean checkpoint(final Map<String, Worker> existingWorkersSnapshot,
            final boolean firstTime) throws Exception {
        if (!firstTime && existingWorkersSnapshot.size() == 0) {
            // all workers completed
            return false;
        }
        final String workerName = "executorStarving()";
        for (final BgContext ctx : contexts) {
            trySubmit(ctx.toString() + " " + workerName, new Worker() {
                
                @Override
                protected void call2() throws Exception {
                    ctx.executorStarving(mksnapshotWorkers());
                }
                
                @Override
                public String toString() {
                    return workerName;
                }
            });
        }
        return true;
    }
    
    private void awaitStarvation(final long timeoutSeconds) throws TimeoutException,
            InterruptedException {
        final long begNanos = System.nanoTime();
        final long endNanos = begNanos + (timeoutSeconds * 1000000000L);
        
        for (;;) {
            final Map<String, BgExecutor.Worker> workersSnapshot = mksnapshotWorkers();
            if (workersSnapshot.isEmpty()) {
                break;
            }
            
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
    }
    
    public boolean trySubmit(final String key, final Worker worker) throws Exception {
        synchronized (workersByFilename) {
            final Worker prev = workersByFilename.put(key, worker);
            if (prev != null) {
                workersByFilename.put(key, prev);
                return false;
            }
            worker.key = key;
            worker.fut = executor.submit(worker);
        }
        return true;
    }
    
    public abstract class BgContext {
        
        public BgContext() {
            synchronized (workersByFilename) {
                contexts.add(this);
            }
        }
        
        protected abstract void executorStarving(Map<String, Worker> existingWorkersSnapshot)
                throws Exception;
        
        protected void destroyTls() {}
    }
    
    void setThreadHint(final String hint) {
        Thread.currentThread().setName("Bg" + Thread.currentThread().getId() + "(" + hint + ")");
    }
    
    public abstract class Worker implements Callable<Void> {
        
        private String key;
        
        private Future<Void> fut;
        
        @Override
        public final Void call() throws Exception {
            setThreadHint(toString());
            // log("worker start: " + file.getName());
            try {
                call2();
            } finally {
                synchronized (workersByFilename) {
                    workersByFilename.remove(key);
                }
                log("worker end: " + key);
                setThreadHint("idle");
            }
            
            return null;
        }
        
        protected abstract void call2() throws Exception;
        
        public String getKey() {
            return key;
        }
        
    }
    
    private final Map<String, Worker> workersByFilename = new LinkedHashMap<String, Worker>();
    
    protected final Map<String, Worker> mksnapshotWorkers() {
        synchronized (workersByFilename) {
            return new HashMap<String, BgExecutor.Worker>(workersByFilename);
        }
    }
    
    private final Set<BgContext> contexts = new HashSet<BgContext>();
    
    private final ExecutorService executor;
    
    @Override
    protected void finalize() throws Throwable {
        log(this + " finalize()");
        try {
            executor.shutdown();
        } catch (final Exception e) {
        }
        super.finalize();
    }
    
    void destroyTls() {
        HashSet<BgContext> contextsSnapshot;
        synchronized (workersByFilename) {
            contextsSnapshot = new HashSet<BgContext>(contexts);
        }
        for (final BgContext ctx : contextsSnapshot) {
            ctx.destroyTls();
        }
        
    }
}
