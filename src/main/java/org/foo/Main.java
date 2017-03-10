package org.foo;

import java.util.ArrayList;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import org.apache.commons.net.ftp.FTPFile;

public class Main {

	public static void main(String[] args) throws Exception {
		ExecutorService executor = Executors.newFixedThreadPool(5,
				new ThreadFactory() {
					@Override
					public Thread newThread(Runnable r) {
						try {
							return new MyThread(r);
						} catch (Exception e) {
							throw new RuntimeException(e);
						}
					}
				});

		MyFTPClient ftp = new MyFTPClient();
		FTPFile[] files = ftp.listFiles();

		RetrieveWorker[] allworkers = new RetrieveWorker[files.length];
		for (int i = 0; i < files.length; i++) {
			FTPFile file = files[i];
			RetrieveWorker worker = new RetrieveWorker(file);
			allworkers[i] = worker;
			executor.execute(worker);
		}
		executor.shutdown();
		while(!executor.awaitTermination(4, TimeUnit.SECONDS)) {
			StringBuilder sb = new StringBuilder("---------------------\n");
			for (int i = 0; i < files.length; i++) {
				FTPFile file = allworkers[i].getFile();
				long progress = allworkers[i].getProgress();
				if (progress != 0 && progress != file.getSize()) {
					sb.append(100 * progress / file.getSize()).append("% ").append(file.getName()).append('\n');
				}
			}
			sb.append("---------------------");
			log(sb);
		}
		log("Finished all threads");
	}

	public static void log(Object o) {
		Thread t = Thread.currentThread();
		System.out.println(t.getId() + " " + t.getName() + " " + o);
	}
}
