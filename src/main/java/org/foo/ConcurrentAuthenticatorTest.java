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

public abstract class ConcurrentAuthenticatorTest {
    
    public static void main(final String[] args) throws Exception {
        final ExecutorService executor = Executors.newCachedThreadPool();
        ((ThreadPoolExecutor) executor).setKeepAliveTime(3, TimeUnit.SECONDS);
        
        class Impl extends ConcurrentAuthenticatorUnsafe {
            
            @Override
            protected PasswordAuthentication getPasswordAuthentication(final Thread callerThread)
                    throws Exception {
                final URL u = getRequestingURL();
                System.out.println("requesting auth for \"" + u + "\"...");
                Thread.sleep(200000);
                System.out.println("got auth for \"" + u + "\"");
                final String[] parts = u.getPath().split("/");
                final String user = parts[parts.length - 2];
                final String password = parts[parts.length - 1];
                return new PasswordAuthentication(user, password.toCharArray());
            }
        }
        
        Authenticator.setDefault(new Impl());
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
}
