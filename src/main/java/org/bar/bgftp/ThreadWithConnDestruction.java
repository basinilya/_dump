package org.bar.bgftp;

import java.lang.ref.WeakReference;

class ThreadWithConnDestruction extends Thread
{
	private final WeakReference<BgFTP> ctxRef;

	public ThreadWithConnDestruction(final Runnable r, final BgFTP ctx) {
		super(r);
		this.ctxRef = new WeakReference<BgFTP>(ctx);
	}
	
	@Override
	public void run() {
		try {
			super.run();
		} finally {
			final BgFTP ctx = ctxRef.get();
			if (ctx != null) {
				ctx.invalidateFtp();
			}
		}
	}
}
