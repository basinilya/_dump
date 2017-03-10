package org.foo;

public class MyThread extends Thread
{
	private MyFTPClient ftp;

	public MyThread(Runnable r) {
		super(r);
	}

	@Override
	public void run() {
		try {
			ftp = new MyFTPClient();
		} catch (Exception e1) {
			throw new RuntimeException(e1);
		}
		
		try {
			super.run();
		} finally {
			try {
				ftp.close();
			} catch (Exception e) {
			}
		}
	}

	public MyFTPClient getFtp() {
		return ftp;
	}
}
