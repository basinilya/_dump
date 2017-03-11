package org.foo;

import static org.foo.Main.log;

import java.io.IOException;
import java.io.InputStream;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadPoolExecutor;

import org.apache.commons.net.ftp.FTPClient;
import org.apache.commons.net.ftp.FTPClientConfig;
import org.apache.commons.net.ftp.FTPFile;
import org.apache.commons.net.ftp.FTPReply;

public class MyFTPClient {
	FTPClient ftp;
	boolean error = false;

	public MyFTPClient() throws Exception {
		ftp = new FTPClient();
		FTPClientConfig config = new FTPClientConfig();
		// config.setXXX(YYY); // change required options
		// for example config.setServerTimeZoneId("Pacific/Pitcairn")
		ftp.configure(config);

		int reply;
		String server = "192.168.56.150";
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
	}

	public void close() throws Exception {
		log("close ftp client");
		try {
			ftp.logout();
		} finally {
			if (ftp.isConnected()) {
				try {
					ftp.disconnect();
				} catch (IOException ioe) {
					// do nothing
				}
			}
		}
	}

	public InputStream retrieve(String filename) throws Exception {
		InputStream is = ftp.retrieveFileStream(filename);
		return is;
	}

	public void completePendingCommand() throws Exception {
		if (!ftp.completePendingCommand()) {
			throw new Exception("something failed");
		}
	}
	
	public FTPFile[] listFiles() throws Exception {
		FTPFile[] files = ftp.listFiles();
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

}
