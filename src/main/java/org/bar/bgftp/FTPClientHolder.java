package org.bar.bgftp;

import static org.bar.bgftp.Log.log;

import java.io.IOException;
import java.io.InputStream;

import org.apache.commons.net.ftp.FTPClient;
import org.apache.commons.net.ftp.FTPFile;
import org.apache.commons.net.ftp.FTPReply;

class FTPClientHolder {
	private long lastUsed;

	final FTPClient ftp = new FTPClient();

	public boolean validate() throws Exception {
		final int reply = ftp.noop();
		lastUsed = System.nanoTime();
		return FTPReply.isPositiveCompletion(reply);
	}

	public void close() throws Exception {
		log("close ftp client");
		try {
			ftp.logout();
		} finally {
			if (ftp.isConnected()) {
				try {
					ftp.disconnect();
				} catch (final IOException ioe) {
					// do nothing
				}
			}
		}
	}

	public InputStream retrieve(final String filename) throws Exception {
		final InputStream is = ftp.retrieveFileStream(filename);
		lastUsed = System.nanoTime();
		return is;
	}

	public void completePendingCommand() throws Exception {
		if (!ftp.completePendingCommand()) {
			throw new Exception("something failed");
		}
		lastUsed = System.nanoTime();
	}
	
	public FTPFile[] listFiles() throws Exception {
		final FTPFile[] files = ftp.listFiles();
		lastUsed = System.nanoTime();
		return files;
		/*
		String[] rslt = new String[files.length];
		for (int i = 0 ; i < files.length; i++) {
			FTPFile f = files[i];
			rslt[i] = f.getName();
			f.getSize();
		}
		return rslt;
		*/
	}

	public long getLastUsed() {
		return lastUsed;
	}

	public void setLastUsed(final long lastUsed) {
		this.lastUsed = lastUsed;
	}

}
