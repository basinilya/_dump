package org.foo;

import static org.foo.Main.log;

import java.io.InputStream;

import org.apache.commons.net.ftp.FTPFile;

public class RetrieveWorker implements Runnable {

	private final FTPFile file;

	public RetrieveWorker(FTPFile f) {
		this.file = f;
	}

	@Override
	public void run() {
		Thread.currentThread().setName("RETR " + file.getName());
		//log("worker start: " + file.getName());
		try {
			processCommand();
		} catch (Exception e) {
			throw new RuntimeException(e);
		} finally {
			getCtx().getWorkersByFilename().remove(file.getName());
			log("worker end: " + file.getName());
		}
	}

	private void processCommand() throws Exception {
		MyFTPClient ftp = getFtp();
		
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

	private long progress;

	public synchronized long getProgress() {
		return progress;
	}

	private synchronized void progress(long p) {
		progress += p;
	}

	private MyContext getCtx() {
		return ((MyThread) Thread.currentThread()).getCtx();
	}

	private MyFTPClient getFtp() throws Exception {
		return ((MyThread) Thread.currentThread()).getFtp();
	}
}