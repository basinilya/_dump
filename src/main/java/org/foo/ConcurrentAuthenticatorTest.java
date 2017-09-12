package org.foo;

import java.io.InputStream;
import java.net.Authenticator;
import java.net.PasswordAuthentication;
import java.net.URL;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

public class ConcurrentAuthenticatorTest {
    
    public static void main(final String[] args) throws Exception {
        final ExecutorService executor = Executors.newCachedThreadPool();
        ((ThreadPoolExecutor) executor).setKeepAliveTime(3, TimeUnit.SECONDS);
        
        final ConcurrentAuthenticatorUnsafe inst = new ConcurrentAuthenticatorUnsafe() {
            
            @Override
            protected PasswordAuthentication getPasswordAuthentication(final Thread callerThread)
                    throws Exception {
                final URL u = getRequestingURL();
                System.out.println("requesting auth for \"" + u + "\"...");
                Thread.sleep(2000);
                System.out.println("got auth for \"" + u + "\"");
                final String[] parts = u.getPath().split("/");
                final String user = parts[parts.length - 2];
                final String password = parts[parts.length - 1];
                return new PasswordAuthentication(user, password.toCharArray());
            }
        };
        Authenticator.setDefault(inst);
        try {
            final Future<Void> fut1 = executor.submit(mkTest("user1", "password1"));
            final Callable<Void> test2 = mkTest("user2", "password2");
            // final Future<Void> fut2 = executor.submit(test2);
            test2.call();
            fut1.get();
            // fut2.get();
        } finally {
            executor.shutdown();
        }
        // executor.awaitTermination(1, TimeUnit.MINUTES);
    }
    
    private static Callable<Void> mkTest(final String user, final String password) throws Exception {
        return new Callable<Void>() {
            
            @Override
            public Void call() throws Exception {
                final URL u =
                        new URL("http://httpbin.org/basic-auth/" + user + "/" + password + "");
                // downloadContent(u);
                requestAuth(u);
                return null;
            }
        };
    }
    
    static void requestAuth(final URL u) {
        Authenticator.requestPasswordAuthentication(null, null, 0, null, null, null, u, null);
    }
    
    static void downloadContent(final URL u) throws Exception {
        try (InputStream in = u.openStream()) {
            int nb;
            final byte[] buf = new byte[1024];
            while ((nb = in.read(buf)) != -1) {
                System.out.write(buf, 0, nb);
            }
        }
    }
}
