package org.foo;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import org.apache.commons.net.ftp.FTPFile;

public class Main {



	public static void main(final String[] args) throws Exception {
		final MyContext ctx = new MyContext();

		final ExecutorService executor = Executors.newFixedThreadPool(5,
				new ThreadFactory() {
					@Override
					public Thread newThread(final Runnable r) {
						return new MyThread(r, ctx);
					}
				});

		for(;;) {
			try {
				doIt(ctx, executor);
			} catch (final Exception e) {
				log("list files failed", e);
				ctx.invalidateFtp();
			}
			//break;
		}
		//log("Finished all threads");
	}

	private static void doIt(final MyContext ctx, final ExecutorService executor) throws Exception {
		final Map<String, RetrieveWorker> workersByFilename = ctx.getWorkersByFilename();
		HashMap<String, RetrieveWorker> workersBeforeListfiles;
		synchronized(workersByFilename) {
			workersBeforeListfiles = new HashMap<String, RetrieveWorker>(workersByFilename);
		}

		final FTPFile[] files = ctx.getFtp().listFiles();

		int addedThisTime = 0;

		synchronized(workersByFilename) {
			for (int i = 0; i < files.length; i++) {
				//if (i > 2) break;
				final FTPFile file = files[i];
				final String filename = file.getName();
				if (!workersBeforeListfiles.containsKey(filename)) {
					if (!workersBeforeListfiles.isEmpty()) {
						log("NOT skipping " + filename);
					}
					final RetrieveWorker worker = new RetrieveWorker(ctx, file);
					workersByFilename.put(filename, worker);
					// TODO: executor.submit(worker) // .get(4, TimeUnit.SECONDS);
					executor.execute(worker);
					addedThisTime++;
				} else {
					log("skipping " + filename);
				}
			}
		}

		//if (addedThisTime
		// TODO: executor.submit(worker) // .get(4, TimeUnit.SECONDS);
		executor.shutdown();
		while(!executor.awaitTermination(4, TimeUnit.SECONDS))
		//for (int i = 0; i < 4; i++)
		{
			//Thread.sleep(4000);
			final StringBuilder sb = new StringBuilder("---------------------\n");
			synchronized(workersByFilename) {
				for (final RetrieveWorker worker : workersByFilename.values()) {
					final FTPFile file = worker.getFile();
					final long progress = worker.getProgress();
					if (progress != -1) {
						sb.append(100 * progress / file.getSize()).append("% ").append(file.getName()).append('\n');
					}
				}
			}
			sb.append("---------------------");
			log(sb);
		}
	}

	public static void log(final Object o) {
		final Thread t = Thread.currentThread();
		System.out.println(t.getId() + " " + t.getName() + " " + o);
	}

	public static void log(final Object o, final Throwable e) {
		final StringWriter sw = new StringWriter();
		final PrintWriter out = new PrintWriter(sw);
		e.printStackTrace(out);
		log(o + " " + sw.toString());
	}
}
