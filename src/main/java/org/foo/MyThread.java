package org.foo;

public class MyThread extends Thread
{
	private final MyContext ctx;

	public MyThread(Runnable r, MyContext ctx) {
		super(r);
		this.ctx = ctx;
	}
	
	@Override
	public void run() {
		try {
			super.run();
		} finally {
			getCtx().invalidateFtp();
		}
	}

	public MyContext getCtx() {
		return ctx;
	}
}
