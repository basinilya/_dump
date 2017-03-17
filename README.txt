/**
public class BgExecutor {
	// threadFactory e.g. to close IDfSession at the end
	public BgExecutor(int nThreads [,BgExecutorThreadFactory threadFactory]);
	public void run();
}
public abstract class BgContext<T> {
	public BgContext(BgExecutor bgExecutor);

	// In this callback we should list files
	// It is called when we're about to starve and want more files to process
	protected abstract void submitMoreTasks(T param);

	// This adds a file to process. We should call it from submitMoreTasks()
	@Override
	protected boolean trySubmit(String key, Callable worker);
}
public abstract class BgFTPFolder extends BgContext<FTPClient> {
	// callback for when we need a new ftp connection 
	protected abstract void connect(FTPClient ftp);

	// In this callback we should list files on server.
	// It is called when we're about to starve and want more files to process
	@Override
	protected abstract void submitMoreTasks(FTPClient ftp);

	// This adds a file to process. We should call it from submitMoreTasks()
	@Override
	protected boolean trySubmit(String key, BgFTPFolder.Worker ftpWorker);

	public abstract class Worker {
		protected abstract void run(FTPClient ftp);
	}
	...
}

Usage example:
*/

FTPExchangeJob {
	void doWork() {
		final String pattern = "*.zip";
		final int maxFiles = 42;
		BgFTP bgFTP = new BgFTP();
		BgFTPFolder folder1 = new OurFTPFolder("/folder1/out", pattern);
		BgFTPFolder folder2 = new OurFTPFolder("/folder2/out", pattern);
		bgFTP.run();
	}
}

class OurFTPFolder extends BgFTPFolder() {
	public OurFTPFolder(String path, String pattern) {
		this.path = path;
		this.pattern = pattern;
	}

	@Override
	protected void connect(FTPClient ftp) {
		ftp.connect("192.168.2.1");
		ftp.login("anonymous", "anonymous@aol.com");
		ftp.changeDir(this.path);
	}

	int total;

	@Override
	protected void submitMoreTasks(FTPClient ftp) {
		FTPFile[] files = ftp.list(pattern);
		
		for (FTPFile _f : files) {
			final FTPFile f = _f;
			if (maxFiles != -1 && total > maxFiles)
				break;
			String key = path + "/" + f.getName();
			ftpWorker = new BgFTPFolder.Worker(this) {
				public void run(FTPClient ftp) {
					ftp.retrieve(f.getName(), "/u01/flxdr/blah/in/" + f.getName());
					ftp.delete(f.getName());
				}
			};
			trySubmit(key, ftpWorker);
			total++;
		}
	}

Usage notes:
- Do not rename files on server to lock them to not add additional overhead in optimistic scenario
- If deleting a remote FTP file fails (for any reason), it is up to the user to prevent repeated download.
  I recommend using local file suffixes for incomplete download

Implementation requirements:

- The multithreaded executor should be implemented as an utility class
- The FTP downloader part should be optional
- The class should download simultaneously from different servers and folders on one server
- The class should ask the user for files to process, at least once (at the beginning)
- While current tasks are processing, job should periodically ask the user for new files to process
- Job should exit, if all tasks finished (before next periodic check)

- If a user tries to submit a task with a key that is already running, it should be rejected
  (This may happen when periodic refresh runs and the previous task haven't yet deleted the file on server)

- FTP connections should be reused to avoid reconnect delays
- When reusing, clients should not send same CWD verb to server
- Assume that server disconnects unused FTP connections, validate and re-connect
- Do not validate if connection was used recently
- Invalidate connection if error occurred

- Class users will write the code to login, list and download files and to start workflows
- The class should handle connection pooling and thread pooling internally.
- The class should not require users implement FTP connection factory. 
  Instead The utility class should provide an abstract initialize method with FTP client argument.
- Class users should not care about returning connections to the pool.
  Instead the connection should be passed as abstract method argument. 
- Class users should not care about invalidating connections in case of exception.
  Instead the class should handle exception and invalidate connection.  




TODO: see
> AJWF Download Processed Manuscripts From AMPP
> ImportFromBreeze
> DeliveryArchiveImportJob
> AbstractFileImportJob

local files job ajwf_ArchiveImport
FTP jobs:
ajwf_FTP_...
method FTPExchangeJob.AJWFFTPExchange

config:
/System/Springer/Config
ajwfftpconfiguration.xml