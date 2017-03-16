/**
Usage example:
public abstract class BgFTP {
	protected abstract void connect(FTPClient ftp);
	protected abstract void submitMoreTasks(FTPClient ftp);
	protected boolean trySubmit(String key, BgFTP.Worker ftpWorker);
	public void run();

	public abstract class Worker {
		protected abstract void run(FTPClient ftp);
	}
	...
}
*/

DeliveryArchiveImportJob {
	void execute() {
		final String pattern = "*.zip";
		final int maxFiles = 42;
		BgFTP bgFTP = new BgFTP(this) {
			@Override
			protected void connect(FTPClient ftp) {
				ftp.login("anonymous", "anonymous@aol.com");
				ftp.changeDir("/blah/out");
			}
	
			int total;

			@Override
			protected void submitMoreTasks(FTPClient ftp) {
				FTPFile[] files = ftp.list(pattern);
				
				for (FTPFile _f : files) {
					final FTPFile f = _f;
					if (maxFiles != -1 && total > maxFiles)
						break;
					String key = f.getName();
					ftpWorker = new BgFTP.Worker(this) {
						public void run(FTPClient ftp) {
							ftp.retrieve(this.f.getName(), "/u01/flxdr/blah/in/");
						}
					};
					trySubmit(key, ftpWorker);
					total++;
				}
			}
		bgFTP.run();
	}
    
}

- The multithreaded FTP file downloader should be implemented as an utility class

- FTP connections should be reused to avoid reconnect delays
- When reusing, clients should not send same CWD verb to server
- Assume that server disconnects unused FTP connections, validate and re-connect
- Do not validate if connection was used recently
- Invalidate connection if error occured

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

- All FTP modifications after successful workflow start should be retried multiple times
- Do not rename files on server to lock them to not add additional overhead in optimistic scenario
- While current tasks are processing, job should periodically check if new files appeared on server



TODO: see
> AJWF Download Processed Manuscripts From AMPP
> ImportFromBreeze
> DeliveryArchiveImportJob
> AbstractFileImportJob
