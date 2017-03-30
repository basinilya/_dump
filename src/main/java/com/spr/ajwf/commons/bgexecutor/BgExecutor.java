package com.spr.ajwf.commons.bgexecutor;

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
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import com.spr.ajwf.commons.logger.Logger;

/**
 * Wrapper for {@link ThreadPoolExecutor} that prevents accidental run of same task twice. Good for
 * watching local and remote folders for new files. Easily programmed to shutdown when idle.
 */
public class BgExecutor {
    
    /**
     * @param nThreads
     *            the number of threads in the pool
     */
    public BgExecutor(final int nThreads) {
        executor = (ThreadPoolExecutor) Executors.newFixedThreadPool(nThreads,
            new BgExecutorThreadFactory(this));
    }
    
    private boolean doneItBeforeField;
    
    /**
     * Queries for initial tasks and waits until they all complete. See
     * {@link #checkpoint(Map, boolean)} for how it works
     * 
     * @throws Exception
     */
    public void run(final boolean failOnExecutionException) throws Exception {
        try {
            for (;;) {
                synchronized (tasksByKey) {
                    if (!checkpoint(tasksByKey, contexts, doneItBeforeField)) {
                        break;
                    }
                }
                doneItBeforeField = true;
                
                try {
                    awaitStarvation(4, failOnExecutionException);
                    // No exception. We starved
                } catch (final TimeoutException e) {
                    //
                }
            }
        } finally {
            executor.shutdown();
            executor.awaitTermination(Long.MAX_VALUE, TimeUnit.SECONDS);
            destroyTls();
        }
    }
    
    /**
     * Called by {@link #run()} when it starts and also called periodically allowing to watch
     * progress and add more tasks. For each context the default implementation schedules a call to
     * {@link BgContext#executorStarving(Map) } in background thread. It schedules nothing and
     * returns false if all tasks completed. Do not perform lengthy operations in it, because
     * {@link BgExecutor} instance is locked and new tasks won't be able to start
     * 
     * @param existingTasksSnapshot
     *            map of existing tasks by their key. Empty map means no tasks added yet or all
     *            tasks completed
     * @param contextsSnapshot
     *            set of contexts
     * @param doneItBefore
     *            false when it's called for the first time
     * @return true if should continue to run
     * @throws Exception
     */
    protected boolean checkpoint(final Map<String, Task> existingTasksSnapshot,
            final Set<BgContext> contextsSnapshot, final boolean doneItBefore) throws Exception {
        if (doneItBefore && existingTasksSnapshot.size() == 0) {
            // all tasks completed
            return false;
        }
        for (final BgContext ctx : contextsSnapshot) {
            ctx.submitStarving();
        }
        return true;
    }
    
    /**
     * Wait for all tasks to complete. Similar to the existing
     * {@link ExecutorService#awaitTermination(long, TimeUnit)}, but doesn't require shutdown of the
     * executor. It is implemented by having a map of all submitted futures and calling
     * {@link Future#get(long, TimeUnit)} for each of them. Alternative would be overriding
     * {@link ThreadPoolExecutor#afterExecute(Runnable, Throwable)} and unlocking a custom lock in
     * it
     * 
     * @param timeoutSeconds
     *            the maximum time to wait
     * @throws TimeoutException
     *             if the wait timed out
     * @throws InterruptedException
     *             {@link Future#get()} called internally theoretically can throw this
     */
    private void awaitStarvation(final long timeoutSeconds, final boolean failOnExecutionException)
            throws TimeoutException, ExecutionException, InterruptedException {
        final long begNanos = System.nanoTime();
        final long endNanos = begNanos + (timeoutSeconds * 1000000000L);
        
        for (;;) {
            final Map<String, BgExecutor.Task> tasksSnapshot = mksnapshotTasks();
            if (tasksSnapshot.isEmpty()) {
                break;
            }
            
            for (final Task task : tasksSnapshot.values()) {
                try {
                    long timeoutNanos = endNanos - System.nanoTime();
                    if (timeoutNanos <= 0) {
                        timeoutNanos = 0;
                    }
                    // InterruptedException, ExecutionException, TimeoutException;
                    task.fut.get(timeoutNanos, TimeUnit.NANOSECONDS);
                } catch (final ExecutionException e) {
                    if (failOnExecutionException) {
                        throw e;
                    }
                    LOGGER.info("swallow exception dispatched to main thread", e.getCause());
                }
            }
        }
    }
    
    /**
     * Tries to submit a task to our pool. Users should call this method from
     * {@link BgContext#executorStarving(Map)}
     * 
     * @param key
     *            unique string to prevent simultaneous execution of same thing
     * @param task
     *            the task
     * @return false if the key provided is already queued
     * @throws Exception
     */
    public boolean trySubmit(final String key, final Task task) throws Exception {
        synchronized (tasksByKey) {
            final Task prev = tasksByKey.put(key, task);
            if (prev != null) {
                tasksByKey.put(key, prev);
                return false;
            }
            task.key = key;
            task.fut = executor.submit(task);
        }
        return true;
    }
    
    /**
     * A context in which a task is executed, e.g. a folder. Can be used to allow a group tasks
     * share one resource
     */
    public abstract class BgContext {
        
        public BgContext() {
            synchronized (tasksByKey) {
                contexts.add(this);
            }
        }
        
        /**
         * A callback for when we want more tasks from this context. This method itself is run in a
         * background task and it is safe to perform lengthy operations in it
         * 
         * @param existingTasksSnapshot
         *            contains all tasks (including other contexts) BEFORE this method was called.
         *            Useful to not accidentally submit same task that completed while this method
         *            was executing
         * @throws Exception
         */
        protected abstract void executorStarving(Map<String, Task> existingTasksSnapshot)
                throws Exception;
        
        /**
         * A callback for when a thread in a pool terminates or {@link #run()} finishes. Override it
         * if you need to destroy some thread-local variable. Default implementation does nothing
         */
        protected void destroyTls() {}
        
        /**
         * Used as part of the key for the executorStarving task. Make sure it's unique
         */
        @Override
        public String toString() {
            return super.toString();
        }
        
        public BgExecutor getExecutor() {
            return BgExecutor.this;
        }
        
        public boolean submitStarving() throws Exception {
            final String taskName = "executorStarving()";
            return trySubmit(toString() + " " + taskName, new Task() {
                
                @Override
                protected void call2() throws Exception {
                    executorStarving(mksnapshotTasks());
                }
                
                @Override
                public String toString() {
                    return taskName;
                }
            });
        }
    }
    
    /**
     * Base class for our tasks.
     */
    public abstract class Task implements Callable<Void> {
        
        /** A string that uniquely identifies this task */
        private String key;
        
        /** A future returned from {@link ExecutorService#submit(Callable)} */
        private Future<Void> fut;
        
        /**
         * This implementation ensures that the task is removed from our map of futures when it
         * completes so the map does not grow infinitely
         */
        @Override
        public final Void call() throws Exception {
            setThreadHint(toString());
            LOGGER.trace("task start: " + key);
            try {
                call2();
            } catch (final Exception e) {
                LOGGER.info("Exception in background thread. Dispatching it to main thread:", e);
                throw e;
            } finally {
                synchronized (tasksByKey) {
                    tasksByKey.remove(key);
                }
                LOGGER.trace("task end: " + key);
                setThreadHint("idle");
            }
            
            return null;
        }
        
        /**
         * A callback that subclasses should implement instead of {@link #call()}
         * 
         * @throws Exception
         */
        protected abstract void call2() throws Exception;
        
        public String getKey() {
            return key;
        }
        
        /**
         * Used as an argument to {@link BgExecutor#setThreadHint(String)}. Make sure it's short
         */
        @Override
        public String toString() {
            return super.toString();
        }
    }
    
    /**
     * Sets a name for background thread indicating its state
     * 
     * @param hint
     *            used as part of the thread name
     */
    void setThreadHint(final String hint) {
        Thread.currentThread().setName("Bg" + Thread.currentThread().getId() + "(" + hint + ")");
    }
    
    /** Map of tasks used in various places and also our lock object */
    private final Map<String, Task> tasksByKey = new LinkedHashMap<String, Task>();
    
    /** Set of contexts */
    private final Set<BgContext> contexts = new HashSet<BgContext>();
    
    private final ThreadPoolExecutor executor;
    
    /**
     * Creates a copy of {@link #tasksByKey}
     * 
     * @return
     */
    protected final Map<String, Task> mksnapshotTasks() {
        synchronized (tasksByKey) {
            return new HashMap<String, BgExecutor.Task>(tasksByKey);
        }
    }
    
    /**
     * Called by background thread when it finishes. For each context this method calls
     * {@link BgContext#destroyTls()}
     */
    void destroyTls() {
        HashSet<BgContext> contextsSnapshot = null;
        synchronized (tasksByKey) {
            contextsSnapshot = new HashSet<BgContext>(contexts);
        }
        for (final BgContext ctx : contextsSnapshot) {
            ctx.destroyTls();
        }
        
    }
    
    /**
     * Calls {@link ExecutorService#shutdown()}, if {@link #run()} failed to do that. If we don't do
     * that, the active executor will never be collected by gc.
     */
    @Override
    protected void finalize() throws Throwable {
        LOGGER.trace(this + " finalize()");
        try {
            executor.shutdown();
        } catch (final Exception e) {
        }
        super.finalize();
    }
    
    public ThreadPoolExecutor getPoolExecutor() {
        return executor;
    }
    
    private static final Logger LOGGER = new Logger(BgExecutor.class);
}
