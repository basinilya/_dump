package org.foo;

import java.net.Authenticator;
import java.net.PasswordAuthentication;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.ThreadFactory;

/**
 * {@link Authenticator} disallows simultaneous password prompting from different threads by running
 * the callback inside a synchronized block. This implementation circumvents it by losing the
 * monitor ownership by calling {@link Object#wait()}.
 */
public abstract class ConcurrentAuthenticator extends Authenticator implements Cloneable {
    
    protected abstract PasswordAuthentication getPasswordAuthentication(final Thread callerThread,
            final Object selfCopy) throws Exception;
    
    private final ExecutorService executor;
    
    public ConcurrentAuthenticator() {
        this(Executors.newCachedThreadPool(new ThreadFactory() {
            
            final ThreadFactory parent = Executors.defaultThreadFactory();
            
            @Override
            public Thread newThread(final Runnable r) {
                final Thread res = parent.newThread(r);
                res.setDaemon(true);
                return res;
            }
        }));
    }
    
    public ConcurrentAuthenticator(final ExecutorService executor) {
        this.executor = executor;
    }
    
    protected PasswordAuthentication unlockAndGetPasswordAuthentication(final Thread callerThread,
            final Object selfCopy) {
        try {
            synchronized (ConcurrentAuthenticator.this) {
                class Task implements Callable<PasswordAuthentication> {
                    
                    boolean done;
                    
                    @Override
                    public PasswordAuthentication call() throws Exception {
                        try {
                            return getPasswordAuthentication(callerThread, selfCopy);
                        } finally {
                            synchronized (ConcurrentAuthenticator.this) {
                                done = true;
                                ConcurrentAuthenticator.this.notifyAll();
                            }
                        }
                    }
                }
                final Task task = new Task();
                final Future<PasswordAuthentication> fut = executor.submit(task);
                while (!task.done) {
                    ConcurrentAuthenticator.this.wait();
                }
                return fut.get();
            }
        } catch (final InterruptedException e) {
            callerThread.interrupt();
            throw new RuntimeException(e);
        } catch (final ExecutionException e) {
            throw new RuntimeException(e);
        }
    }
    
    @Override
    protected final PasswordAuthentication getPasswordAuthentication() {
        try {
            final Object selfCopy = clone();
            synchronized (ConcurrentAuthenticator.this) {
                return unlockAndGetPasswordAuthentication(Thread.currentThread(), selfCopy);
            }
        } catch (final CloneNotSupportedException e) {
            throw new RuntimeException("this can't be", e);
        }
    }
    
    @SuppressWarnings("unchecked")
    protected static <T> T cast(final T self, final Object selfCopy) {
        return (T) selfCopy;
    }
}
