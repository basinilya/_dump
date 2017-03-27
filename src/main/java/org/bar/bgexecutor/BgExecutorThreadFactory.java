package org.bar.bgexecutor;

import java.lang.ref.WeakReference;
import java.util.concurrent.ThreadFactory;

public class BgExecutorThreadFactory implements ThreadFactory {
    
    private final WeakReference<BgExecutor> ctxRef;
    
    public BgExecutorThreadFactory(final BgExecutor executor) {
        this.ctxRef = new WeakReference<BgExecutor>(executor);
    }
    
    @Override
    public Thread newThread(final Runnable r) {
        return new ThreadWithTlsDestruction(r, ctxRef);
    }
}

class ThreadWithTlsDestruction extends Thread {
    
    private final WeakReference<BgExecutor> ctxRef;
    
    public ThreadWithTlsDestruction(final Runnable r, final WeakReference<BgExecutor> ctxRef) {
        super(r);
        this.ctxRef = ctxRef;
    }
    
    @Override
    public void run() {
        try {
            super.run();
        } finally {
            final BgExecutor ctx = ctxRef.get();
            if (ctx != null) {
                ctx.setThreadHint("dying");
                ctx.destroyTls();
            }
        }
    }
}
