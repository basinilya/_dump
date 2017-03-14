package org.foo;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import org.apache.commons.net.ftp.FTPFile;

public class Main {


	public static void main(String[] args) throws Exception {

		final MyContext ctx = new MyContext();

		ExecutorService executor = Executors.newFixedThreadPool(1,
				new ThreadFactory() {
					@Override
					public Thread newThread(Runnable r) {
						return new MyThread(r, ctx);
					}
				});

		for(;;) {
			try {
				doIt(ctx, executor);
			} catch (Exception e) {
				log("list files failed", e);
				ctx.invalidateFtp();
			}
			//break;
		}
		//log("Finished all threads");
	}

	private static void doIt(MyContext ctx, ExecutorService executor) throws Exception {
		Map<String, RetrieveWorker> workersByFilename = ctx.getWorkersByFilename();
		HashMap<String, RetrieveWorker> workersBeforeListfiles = new HashMap<String, RetrieveWorker>();
		workersBeforeListfiles.clear();
		synchronized(workersByFilename) {
			workersBeforeListfiles.putAll(workersByFilename);
		}

		FTPFile[] files = ctx.getFtp().listFiles();

		synchronized(workersByFilename) {
			for (int i = 0; i < files.length; i++) {
				//if (i > 2) break;
				FTPFile file = files[i];
				String filename = file.getName();
				if (!workersBeforeListfiles.containsKey(filename)) {
					if (!workersBeforeListfiles.isEmpty()) {
						log("NOT skipping " + filename);
					}
					RetrieveWorker worker = new RetrieveWorker(ctx, file);
					workersByFilename.put(filename, worker);
					executor.execute(worker);
				} else {
					log("skipping " + filename);
				}
			}
		}

		executor.shutdown();
		while(!executor.awaitTermination(4, TimeUnit.SECONDS))
		//for (int i = 0; i < 4; i++)
		{
			//Thread.sleep(4000);
			StringBuilder sb = new StringBuilder("---------------------\n");
			synchronized(workersByFilename) {
				for (RetrieveWorker worker : workersByFilename.values()) {
					FTPFile file = worker.getFile();
					long progress = worker.getProgress();
					if (progress != 0 && progress != file.getSize()) {
						sb.append(100 * progress / file.getSize()).append("% ").append(file.getName()).append('\n');
					}
				}
			}
			sb.append("---------------------");
			log(sb);
		}
	}

	public static void log(Object o) {
		Thread t = Thread.currentThread();
		System.out.println(t.getId() + " " + t.getName() + " " + o);
	}

	public static void log(Object o, Throwable e) {
		StringWriter sw = new StringWriter();
		PrintWriter out = new PrintWriter(sw);
		e.printStackTrace(out);
		log(o + " " + sw.toString());
	}
}
