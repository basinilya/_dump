package org.foo;

import java.lang.reflect.Field;
import java.net.Authenticator;
import java.net.PasswordAuthentication;

import sun.misc.Unsafe;

/**
 * {@link Authenticator} disallows simultaneous password prompting from different threads, because
 * the callback runs inside a synchronized block. This implementation circumvents it by calling
 * {@link Unsafe#monitorExit(Object)}
 */
@SuppressWarnings("restriction")
public abstract class ConcurrentAuthenticatorUnsafe extends ConcurrentAuthenticator {
    
    private static final Unsafe UNSAFE;
    
    static {
        Unsafe uns = null;
        try {
            final Field singleoneInstanceField = Unsafe.class.getDeclaredField("theUnsafe");
            singleoneInstanceField.setAccessible(true);
            uns = (Unsafe) singleoneInstanceField.get(null);
        } catch (final Exception e) {
            e.printStackTrace();
        }
        UNSAFE = uns;
    }
    
    public ConcurrentAuthenticatorUnsafe() {
        super(null);
    }
    
    @Override
    protected PasswordAuthentication unlockAndGetPasswordAuthentication(final Thread callerThread,
            final ConcurrentAuthenticator selfCopy) {
        if (UNSAFE == null) {
            return super.unlockAndGetPasswordAuthentication(callerThread, selfCopy);
        } else {
            ConcurrentAuthenticatorUnsafe.this.notifyAll();
            UNSAFE.monitorExit(ConcurrentAuthenticatorUnsafe.this);
            try {
                return selfCopy.getPasswordAuthentication(callerThread);
            } catch (final Exception e) {
                throw new RuntimeException(e);
            } finally {
                UNSAFE.monitorEnter(ConcurrentAuthenticatorUnsafe.this);
            }
        }
    }
}
