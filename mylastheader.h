#include "mylogging.h"

/* local */
#undef INFO
#undef WARN
#undef ERR
#define INFO WINET_LOG_MESSAGE
#define WARN WINET_LOG_WARNING
#define ERR WINET_LOG_ERROR

#ifdef _DEBUG
static void dbg_CloseHandle(const char *file, int line, HANDLE hObject) {
	if (!CloseHandle(hObject)) {
		pWin32Error(ERR, "CloseHandle() failed at %s:%d", file, line);
		abort();
	}
}
static void dbg_GetOverlappedResult(const char *file, int line, HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait) {
	BOOL b = GetOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait);
	DWORD dw = GetLastError();
	winet_log(INFO, "GetOverlappedResult(hFile=%p(%lld), nb=%d, ev=%p) == %d; err = %d\n",
		(void*)hFile, (long long)hFile, *lpNumberOfBytesTransferred, (void*)lpOverlapped->hEvent, b, dw);
	SetLastError(dw);
	if (!b) {
		pWin32Error(ERR, "GetOverlappedResult() failed at %s:%d", file, line);
		abort();
	}
}
static void dbg_WriteFile(const char *file, int line, HANDLE hFile,LPCVOID lpBuffer,DWORD nNumberOfBytesToWrite,LPDWORD lpNumberOfBytesWritten,LPOVERLAPPED lpOverlapped) {
	BOOL b = WriteFile(hFile,lpBuffer,nNumberOfBytesToWrite,lpNumberOfBytesWritten,lpOverlapped);
	DWORD dw = GetLastError();
	winet_log(INFO, "WriteFile(hFile=%p(%lld), bufsz=%d, nb=%d, ev=%p) == %d; err = %d\n",
		(void*)hFile, (long long)hFile, nNumberOfBytesToWrite, *lpNumberOfBytesWritten, (void*)lpOverlapped->hEvent, b, dw);
	if (!b && dw != ERROR_IO_PENDING) {
		pWin32Error(ERR, "WriteFile() failed at %s:%d", file, line);
		abort();
	}
	SetLastError(dw);
	//return b;
}
static void dbg_ReadFile(const char *file, int line, HANDLE hFile,LPVOID lpBuffer,DWORD nNumberOfBytesToRead,LPDWORD lpNumberOfBytesRead,LPOVERLAPPED lpOverlapped) {
	BOOL b = ReadFile(hFile,lpBuffer,nNumberOfBytesToRead,lpNumberOfBytesRead,lpOverlapped);
	DWORD dw = GetLastError();
	winet_log(INFO, "ReadFile(hFile=%p(%lld), bufsz=%d, nb=%d, ev=%p) == %d; err = %d\n",
		(void*)hFile, (long long)hFile, nNumberOfBytesToRead, *lpNumberOfBytesRead, (void*)lpOverlapped->hEvent, b, dw);
	if (!b && dw != ERROR_IO_PENDING) {
		pWin32Error(ERR, "ReadFile() failed at %s:%d", file, line);
		abort();
	}
	SetLastError(dw);
	//return b;
}

static void dbg_closesocket(const char *file, int line, SOCKET s) {
	if (closesocket(s) == SOCKET_ERROR) {
		pWinsockError(ERR, "closesocket() failed at %s:%d", file, line);
		abort();
	}
}
static void dbg_shutdown(const char *file, int line, SOCKET s, int how) {
	int rc = shutdown(s, how);
	if (rc != 0) {
		pWinsockError(ERR, "shutdown(%d, SD_SEND) failed at %s:%d", (int)s, file, line);
		abort();
	}
}

#define CloseHandle(hObject) dbg_CloseHandle(__FILE__, __LINE__, hObject)
#define GetOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait) dbg_GetOverlappedResult(__FILE__, __LINE__, hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait)
#define WriteFile(hFile,lpBuffer,nNumberOfBytesToWrite,lpNumberOfBytesWritten,lpOverlapped) dbg_WriteFile(__FILE__, __LINE__, hFile,lpBuffer,nNumberOfBytesToWrite,lpNumberOfBytesWritten,lpOverlapped)
#define ReadFile(hFile,lpBuffer,nNumberOfBytesToRead,lpNumberOfBytesRead,lpOverlapped) dbg_ReadFile(__FILE__, __LINE__,hFile,lpBuffer,nNumberOfBytesToRead,lpNumberOfBytesRead,lpOverlapped)

#define closesocket(s) dbg_closesocket(__FILE__, __LINE__, s)
#define shutdown(s, how) dbg_shutdown(__FILE__, __LINE__, s, how)

#endif /* _DEBUG */
