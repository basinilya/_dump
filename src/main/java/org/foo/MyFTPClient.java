package org.foo;

import static org.foo.Main.log;

import java.io.IOException;
import java.io.InputStream;

import org.apache.commons.net.ftp.FTPClient;
import org.apache.commons.net.ftp.FTPClientConfig;
import org.apache.commons.net.ftp.FTPFile;
import org.apache.commons.net.ftp.FTPReply;

public class MyFTPClient {
	private long lastUsed;

	private final FTPClient ftp;

	public boolean validate() throws Exception {
		final int reply = ftp.noop();
		lastUsed = System.nanoTime();
		return FTPReply.isPositiveCompletion(reply);
	}

	public MyFTPClient() throws Exception {
		ftp = new FTPClient();
		ftp.setServerSocketFactory(null);
		ftp.setSocketFactory(null);
		final FTPClientConfig config = new FTPClientConfig();
		// config.setXXX(YYY); // change required options
		// for example config.setServerTimeZoneId("Pacific/Pitcairn")
		ftp.configure(config);

		int reply;
		final String server = "192.168.56.150";
		ftp.connect(server);
		//log("Connected to " + server + ".");
		//log(ftp.getReplyString()); TODO: trim trailing newline

		// After connection attempt, you should check the reply code to
		// verify
		// success.
		reply = ftp.getReplyCode();

		if (!FTPReply.isPositiveCompletion(reply)) {
			ftp.disconnect();
			throw new Exception("FTP server refused connection.");
		}
		if (!ftp.login("anonymous", "a@b.org")) {
			throw new Exception("login failed: " + ftp.getReplyString());
		}
		if (!ftp.changeWorkingDirectory("/pub/manyfiles")) {
			throw new Exception("failed to change directory: " + ftp.getReplyString());
		}
		lastUsed = System.nanoTime();
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