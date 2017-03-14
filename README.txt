FTP:
- FTP connections should be reused to avoid reconnect delays
- When reusing, clients should not send same CWD verb to server
- Assume that server disconnects unused FTP connections, validate and re-connect
- Do not validate if connection was used recently
- Invalidate connection if error occured

Threads:
- Close connection when thread finishes
