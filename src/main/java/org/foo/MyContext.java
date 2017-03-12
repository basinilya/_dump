package org.foo;

import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;

public class MyContext {
	public MyContext() throws Exception {}

	private static final ThreadLocal<MyFTPClient> ftpTls = new ThreadLocal<MyFTPClient>();

	private final Map<String, RetrieveWorker> workersByFilename = Collections.synchronizedMap(new LinkedHashMap<String, RetrieveWorker>());

	private final ExecutorService executor = Executors.newFixedThreadPool(1,
			new ThreadFactory() {
				@Override
				public Thread newThread(Runnable r) {
					try {
						return new MyThread(r, MyContext.this);
					} catch (Exception e) {
						throw new RuntimeException(e);
					}
				}
			});

	public Map<String, RetrieveWorker> getWorkersByFilename() {
		return workersByFilename;
	}

	public ExecutorService getExecutorService() {
		return executor;
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
		long secondsPassed = (System.nanoTime() - ftp.getLastBorrowed()) / 1000000000L;
		if (secondsPassed < 4) {
			return true;
		}
		return ftp.isValid();
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
		ftp.setLastBorrowed(System.nanoTime());
		return ftp;
	}

}
