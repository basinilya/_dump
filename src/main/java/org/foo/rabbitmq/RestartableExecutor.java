package org.foo.rabbitmq;

import java.util.Collection;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

public class RestartableExecutor implements ExecutorService {
    
    private ExecutorService impl;
    
    public void restart(final ExecutorService newImpl) throws IllegalStateException {
        if (!impl.isTerminated()) {
            throw new IllegalStateException("previous pool not terminated");
        }
        impl = newImpl;
    }
    
    public RestartableExecutor(final ExecutorService impl) {
        this.impl = impl;
    }
    
    @Override
    public void execute(final Runnable command) {
        impl.execute(command);
    }
    
    @Override
    public void shutdown() {
        impl.shutdown();
    }
    
    @Override
    public List<Runnable> shutdownNow() {
        return impl.shutdownNow();
    }
    
    @Override
    public boolean isShutdown() {
        return impl.isShutdown();
    }
    
    @Override
    public boolean isTerminated() {
        return impl.isTerminated();
    }
    
    @Override
    public boolean awaitTermination(final long timeout, final TimeUnit unit)
            throws InterruptedException {
        return impl.awaitTermination(timeout, unit);
    }
    
    @Override
    public <T> Future<T> submit(final Callable<T> task) {
        return impl.submit(task);
    }
    
    @Override
    public <T> Future<T> submit(final Runnable task, final T result) {
        return impl.submit(task, result);
    }
    
    @Override
    public Future<?> submit(final Runnable task) {
        return impl.submit(task);
    }
    
    @Override
    public <T> List<Future<T>> invokeAll(final Collection<? extends Callable<T>> tasks)
            throws InterruptedException {
        return impl.invokeAll(tasks);
    }
    
    @Override
    public <T> List<Future<T>> invokeAll(final Collection<? extends Callable<T>> tasks,
            final long timeout, final TimeUnit unit) throws InterruptedException {
        return impl.invokeAll(tasks, timeout, unit);
    }
    
    @Override
    public <T> T invokeAny(final Collection<? extends Callable<T>> tasks)
            throws InterruptedException, ExecutionException {
        return impl.invokeAny(tasks);
    }
    
    @Override
    public <T> T invokeAny(final Collection<? extends Callable<T>> tasks, final long timeout,
            final TimeUnit unit) throws InterruptedException, ExecutionException, TimeoutException {
        return impl.invokeAny(tasks, timeout, unit);
    }
    
}
