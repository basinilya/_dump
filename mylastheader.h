/* local */
#undef DBG
#undef INFO
#undef WARN
#undef ERR
#define DBG WINET_LOG_DEBUG
#define INFO WINET_LOG_MESSAGE
#define WARN WINET_LOG_WARNING
#define ERR WINET_LOG_ERROR

#define pSysError     cliptund_pSysError
#define pWinsockError cliptund_pWinsockError
#define pWin32Error   cliptund_pWin32Error
#define log           cliptund_log

#define COUNTOF(a) (sizeof(a) / sizeof(a[0]))

#ifdef __cplusplus
extern "C" {
#endif

BOOL dbg_OpenClipboard(HWND hwnd);
void dbg_CloseClipboard();
void dbg_CloseHandle(const char *file, int line, HANDLE hObject);
BOOL dbg_GetOverlappedResult(const char *file, int line, HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait);
BOOL dbg_WriteFile(const char *file, int line, HANDLE hFile,LPCVOID lpBuffer,DWORD nNumberOfBytesToWrite,LPDWORD lpNumberOfBytesWritten,LPOVERLAPPED lpOverlapped);
BOOL dbg_ReadFile(const char *file, int line, HANDLE hFile,LPVOID lpBuffer,DWORD nNumberOfBytesToRead,LPDWORD lpNumberOfBytesRead,LPOVERLAPPED lpOverlapped);
void dbg_AcceptEx(const char *file, int line, SOCKET sListenSocket,SOCKET sAcceptSocket,PVOID lpOutputBuffer,DWORD dwReceiveDataLength,DWORD dwLocalAddressLength,DWORD dwRemoteAddressLength,LPDWORD lpdwBytesReceived,LPOVERLAPPED lpOverlapped);
void dbg_closesocket(const char *file, int line, SOCKET s);
void dbg_shutdown(const char *file, int line, SOCKET s, int how);
SOCKET dbg_socket(const char *file, int line, int af,int type,int protocol);
void dbg_listen(const char *file, int line, SOCKET s, int backlog);
void dbg_getpeername(const char *file, int line, SOCKET s,struct sockaddr * name,int * namelen);
#ifdef __cplusplus
}
#endif

#ifdef _DEBUG

#define OpenClipboard dbg_OpenClipboard
#define CloseClipboard dbg_CloseClipboard
#define CloseHandle(hObject) dbg_CloseHandle(__FILE__, __LINE__, hObject)
#define GetOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait) dbg_GetOverlappedResult(__FILE__, __LINE__, hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait)
#define WriteFile(hFile,lpBuffer,nNumberOfBytesToWrite,lpNumberOfBytesWritten,lpOverlapped) dbg_WriteFile(__FILE__, __LINE__, hFile,lpBuffer,nNumberOfBytesToWrite,lpNumberOfBytesWritten,lpOverlapped)
#define ReadFile(hFile,lpBuffer,nNumberOfBytesToRead,lpNumberOfBytesRead,lpOverlapped) dbg_ReadFile(__FILE__, __LINE__,hFile,lpBuffer,nNumberOfBytesToRead,lpNumberOfBytesRead,lpOverlapped)

#define AcceptEx(sListenSocket,sAcceptSocket,lpOutputBuffer,dwReceiveDataLength,dwLocalAddressLength,dwRemoteAddressLength,lpdwBytesReceived,lpOverlapped) dbg_AcceptEx(__FILE__, __LINE__,sListenSocket,sAcceptSocket,lpOutputBuffer,dwReceiveDataLength,dwLocalAddressLength,dwRemoteAddressLength,lpdwBytesReceived,lpOverlapped)
#define closesocket(s) dbg_closesocket(__FILE__, __LINE__, s)
#define shutdown(s, how) dbg_shutdown(__FILE__, __LINE__, s, how)
#define socket(af,type,protocol) dbg_socket(__FILE__, __LINE__, af,type,protocol)
#define listen(s,backlog) dbg_listen(__FILE__, __LINE__, s,backlog)
#define getpeername(s,name,namelen) dbg_getpeername(__FILE__, __LINE__, s,name,namelen)

#endif /* _DEBUG */
