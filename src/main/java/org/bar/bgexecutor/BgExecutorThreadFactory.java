package org.bar.bgexecutor;

import java.lang.ref.WeakReference;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.ThreadPoolExecutor;

class BgExecutorThreadFactory implements ThreadFactory {
    
    /**
     * {@link ThreadPoolExecutor} holds a strong reference to its thread factory. To allow gc
     * collect {@link BgExecutor} instance we use a weak reference here
     */
    private final WeakReference<BgExecutor> exRef;
    
    public BgExecutorThreadFactory(final BgExecutor executor) {
        this.exRef = new WeakReference<BgExecutor>(executor);
    }
    
    @Override
    public Thread newThread(final Runnable r) {
        return new ThreadWithTlsDestruction(r, exRef);
    }
}

class ThreadWithTlsDestruction extends Thread {
    
    /**
     * Thread will not finish unless {@link BgExecutor} instance is collected and shutdown is
     * called. To allow gc collect {@link BgExecutor} instance we use a weak reference here
     */
    private final WeakReference<BgExecutor> exRef;
    
    public ThreadWithTlsDestruction(final Runnable r, final WeakReference<BgExecutor> exRef) {
        super(r);
        this.exRef = exRef;
    }
    
    @Override
    public void run() {
        try {
            super.run();
        } finally {
            final BgExecutor executor = exRef.get();
            if (executor != null) {
                executor.setThreadHint("dying");
                executor.destroyTls();
            }
        }
    }
}
