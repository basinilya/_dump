package org.foo;

import java.io.InputStream;
import java.net.Authenticator;
import java.net.PasswordAuthentication;
import java.net.URL;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

public abstract class ConcurrentAuthenticator extends Authenticator implements Cloneable {
    
    private final ExecutorService executor;
    
    public ConcurrentAuthenticator(final ExecutorService executor) {
        this.executor = executor;
    }
    
    protected abstract PasswordAuthentication getPasswordAuthentication(final Thread callerThread,
            final ConcurrentAuthenticator requestParams) throws Exception;
    
    @Override
    protected final PasswordAuthentication getPasswordAuthentication() {
        final Thread callerThread = Thread.currentThread();
        try {
            final ConcurrentAuthenticator requestParams =
                    (ConcurrentAuthenticator) ConcurrentAuthenticator.this.clone();
            synchronized (ConcurrentAuthenticator.this) {
                class Task implements Callable<PasswordAuthentication> {
                    
                    boolean done;
                    
                    @Override
                    public PasswordAuthentication call() throws Exception {
                        try {
                            return getPasswordAuthentication(callerThread, requestParams);
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
        } catch (final CloneNotSupportedException | ExecutionException e) {
            throw new RuntimeException(e);
        }
    }
    
    public static void main(final String[] args) throws Exception {
        final ExecutorService executor = Executors.newCachedThreadPool();
        ((ThreadPoolExecutor) executor).setKeepAliveTime(3, TimeUnit.SECONDS);
        final ConcurrentAuthenticator instance = new ConcurrentAuthenticator(executor) {
            
            @Override
            protected PasswordAuthentication getPasswordAuthentication(final Thread callerThread,
                    final ConcurrentAuthenticator requestParams) throws Exception {
                final URL u = requestParams.getRequestingURL();
                System.out.println("requesting auth for \"" + u + "\"...");
                Thread.sleep(6000);
                System.out.println("got auth for \"" + u + "\"");
                final String[] parts = u.getPath().split("/");
                final String user = parts[parts.length - 2];
                final String password = parts[parts.length - 1];
                return new PasswordAuthentication(user, password.toCharArray());
            }
        };
        Authenticator.setDefault(instance);
        try {
            final Future<Void> fut1 = test(executor, "user1", "password1");
            final Future<Void> fut2 = test(executor, "user2", "password2");
            fut1.get();
            fut2.get();
        } finally {
            executor.shutdown();
        }
        // executor.awaitTermination(1, TimeUnit.MINUTES);
    }
    
    private static Future<Void> test(final ExecutorService executor, final String user,
            final String password) throws Exception {
        return executor.submit(new Callable<Void>() {
            
            @Override
            public Void call() throws Exception {
                final URL u =
                        new URL("http://httpbin.org/basic-auth/" + user + "/" + password + "");
                try (InputStream in = u.openStream()) {
                    int nb;
                    final byte[] buf = new byte[1024];
                    while ((nb = in.read(buf)) != -1) {
                        System.out.write(buf, 0, nb);
                    }
                }
                return null;
            }
        });
    }
    
    /*
     * public URL getRequestingURL1() { return super.getRequestingURL(); } public RequestorType
     * getRequestorType1() { return super.getRequestorType(); } public String getRequestingHost1() {
     * return super.getRequestingHost(); } public final InetAddress getRequestingSite1() { return
     * super.getRequestingSite(); } public final int getRequestingPort1() { return
     * super.getRequestingPort(); } public final String getRequestingProtocol1() { return
     * super.getRequestingProtocol(); } public final String getRequestingPrompt1() { return
     * super.getRequestingPrompt(); } public final String getRequestingScheme1() { return
     * super.getRequestingScheme(); }
     */
}
