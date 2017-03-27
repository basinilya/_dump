package org.bar.bgexecutor;

import java.lang.ref.WeakReference;

class ThreadWithTlsDestruction extends Thread {
    
    private final WeakReference<BgExecutor> ctxRef;
    
    public ThreadWithTlsDestruction(final Runnable r, final BgExecutor ctx) {
        super(r);
        this.ctxRef = new WeakReference<BgExecutor>(ctx);
    }
    
    @Override
    public void run() {
        try {
            super.run();
        } finally {
            final BgExecutor ctx = ctxRef.get();
            if (ctx != null) {
                ctx.setThreadHint("dying");
                ctx.threadDying();
            }
        }
    }
}
