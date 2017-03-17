/**
public class BgFTP {
	public BgFTP(int nThreads);
	public void run();
}
public abstract class BgFTPFolder {
	public BgFTPFolder(BgFTP bgFtp);
	// callback for when we need a new ftp connection 
	protected abstract void connect(FTPClient ftp);
	// In this callback we should list files on server.
	// It is called when we're about to starve and want more files to process
	protected abstract void submitMoreTasks(FTPClient ftp);
	// This adds a file to process. We should call it from submitMoreTasks()
	protected boolean trySubmit(String key, BgFTPFolder.Worker ftpWorker);

	public abstract class Worker {
		protected abstract void run(FTPClient ftp);
	}
	...
}

Usage example:
*/

DeliveryArchiveImportJob {
	void execute() {
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
- All FTP modifications after successful workflow start should be retried multiple times
- Do not rename files on server to lock them to not add additional overhead in optimistic scenario

Implementation requirements:

- The multithreaded FTP file downloader should be implemented as an utility class
- The class should download simultaneously from different servers and folders on one server

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
- If an old file still exists on server and existing task on that file not finished, it should not be scheduled again
- If an old file still exists on server and existing task on that file finished, it should be scheduled again, because previous attempt failed
- Job should exit, if all tasks finished (before next periodic check)
- While current tasks are processing, job should periodically check if new files appeared on server




TODO: see
> AJWF Download Processed Manuscripts From AMPP
> ImportFromBreeze
> DeliveryArchiveImportJob
> AbstractFileImportJob
