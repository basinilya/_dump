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
 * {@link Authenticator} disallows simultaneous password prompting from different threads, because
 * the callback runs inside a synchronized block. This implementation circumvents it by calling
 * {@link Object#wait()} and losing the monitor ownership. The side effect is that the actual
 * password prompt has to run in a separate thread.
 */
public abstract class ConcurrentAuthenticator extends Authenticator implements Cloneable {
    
    protected abstract PasswordAuthentication getPasswordAuthentication(final Thread callerThread)
            throws Exception;
    
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
            final ConcurrentAuthenticator selfCopy) {
        try {
            synchronized (ConcurrentAuthenticator.this) {
                class Task implements Callable<PasswordAuthentication> {
                    
                    boolean done;
                    
                    @Override
                    public PasswordAuthentication call() throws Exception {
                        try {
                            return selfCopy.getPasswordAuthentication(callerThread);
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
            final ConcurrentAuthenticator selfCopy = (ConcurrentAuthenticator) clone();
            return unlockAndGetPasswordAuthentication(Thread.currentThread(), selfCopy);
        } catch (final CloneNotSupportedException e) {
            throw new RuntimeException("this can't be", e);
        }
    }
    
}
