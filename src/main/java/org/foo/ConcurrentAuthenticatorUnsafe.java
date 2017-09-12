package org.foo;

import java.lang.reflect.Field;
import java.net.PasswordAuthentication;

import sun.misc.Unsafe;

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
            final Object selfCopy) {
        if (UNSAFE == null) {
            return super.unlockAndGetPasswordAuthentication(callerThread, selfCopy);
        } else {
            UNSAFE.monitorExit(ConcurrentAuthenticatorUnsafe.this);
            try {
                return getPasswordAuthentication(callerThread, selfCopy);
            } catch (final Exception e) {
                throw new RuntimeException(e);
            } finally {
                UNSAFE.monitorEnter(ConcurrentAuthenticatorUnsafe.this);
            }
        }
    }
}
