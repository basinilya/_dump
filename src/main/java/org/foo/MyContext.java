package org.foo;

import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;

public class MyContext {
	public MyContext() throws Exception {}

	private final ThreadLocal<MyFTPClient> ftpTls = new ThreadLocal<MyFTPClient>();

	private final Map<String, RetrieveWorker> workersByFilename = Collections.synchronizedMap(new LinkedHashMap<String, RetrieveWorker>());


	public Map<String, RetrieveWorker> getWorkersByFilename() {
		return workersByFilename;
	}

	public void invalidateFtp() {
		MyFTPClient ftp = ftpTls.get();
		if (ftp != null) {
			try {
				ftp.close();
			} catch (Exception e) {
			}
			ftpTls.set(null);
		}
	}

	private boolean validateFtp(MyFTPClient ftp) throws Exception {
		long secondsPassed = (System.nanoTime() - ftp.getLastUsed()) / 1000000000L;
		if (secondsPassed < 30) {
			return true;
		}
		return ftp.validate();
	}

	public MyFTPClient getFtp() throws Exception {
		MyFTPClient ftp = ftpTls.get();
		if (ftp == null) {
			ftp = new MyFTPClient();
			ftpTls.set(ftp);
		} else {
			try {
				if (!validateFtp(ftp)) {
					throw new Exception("validation failed");
				}
			} catch (Exception e) {
				invalidateFtp();
				return getFtp();
			}
		}
		ftp.setLastUsed(System.nanoTime());
		return ftp;
	}

}