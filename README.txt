FTP:
- FTP connections should be reused to avoid reconnect delays
- When reusing, clients should not send same CWD verb to server
- Assume that server disconnects unused FTP connections, validate and re-connect
- Do not validate if connection was used recently
- Invalidate connection if error occured

Threads:
- Do not rename files on server to lock them to not add additional overhead in optimistic scenario
- While current tasks are processing, job should periodically check if new files appeared on server
- If an old file still exists on server and existing task on that file not finished, it should not be scheduled again
!!! double check current single-threaded implementation
- If an old file still exists on server and existing task on that file finished, it should be scheduled again, because previous attempt failed
- Job should exit, if all tasks finished (before next periodic check)
