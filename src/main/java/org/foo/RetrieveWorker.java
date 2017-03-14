package org.foo;

import static org.foo.Main.log;

import java.io.InputStream;

import org.apache.commons.net.ftp.FTPFile;

public class RetrieveWorker implements Runnable {

	private final MyContext ctx;
	private final FTPFile file;

	public RetrieveWorker(MyContext ctx, FTPFile f) {
		this.ctx = ctx;
		this.file = f;
	}

	@Override
	public void run() {
		Thread.currentThread().setName("RETR " + file.getName());
		//log("worker start: " + file.getName());
		try {
			processCommand();
		} catch (Exception e) {
			ctx.invalidateFtp();
			throw new RuntimeException(e);
		} finally {
			ctx.getWorkersByFilename().remove(file.getName());
			log("worker end: " + file.getName());
		}
	}

	private void processCommand() throws Exception {
		progress = 0;
		MyFTPClient ftp = ctx.getFtp();
		
		InputStream is = ftp.retrieve(file.getName());

		try {
			byte[] buf = new byte[8192];
			int i;
			while((i = is.read(buf)) != -1) {
				//log("got " + i + " bytes");
				progress(i);
			}
		} finally {
			is.close();
		}
		ftp.completePendingCommand();
	}

	@Override
	public String toString() {
		return this.file.getName();
	}
	
	public FTPFile getFile() {
		return file;
	}

	private long progress = -1;

	public synchronized long getProgress() {
		return progress;
	}

	private synchronized void progress(long p) {
		progress += p;
	}
}