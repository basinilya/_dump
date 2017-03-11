package org.foo;

public class MyThread extends Thread
{
	private final MyContext ctx;

	private MyFTPClient ftp;

	public MyThread(Runnable r, MyContext ctx) {
		super(r);
		this.ctx = ctx;
	}
	
	@Override
	public void run() {
		try {
			super.run();
		} finally {
			invalidateFtp();
		}
	}

	public MyFTPClient getFtp() throws Exception {
		if (ftp == null) {
			ftp = new MyFTPClient();
		}
		return ftp;
	}

	public void invalidateFtp() {
		if (ftp != null) {
			try {
				ftp.close();
			} catch (Exception e) {
			}
			ftp = null;
		}
	}

	public MyContext getCtx() {
		return ctx;
	}
}
