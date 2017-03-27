package org.bar.bgftp;

import java.lang.ref.WeakReference;

class ThreadWithConnDestruction extends Thread {
    
    private final WeakReference<BgExecutor> ctxRef;
    
    public ThreadWithConnDestruction(final Runnable r, final BgExecutor ctx) {
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
                ctx.threadDying();
            }
        }
    }
}
