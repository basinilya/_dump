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
		printf("CloseHandle() failed at %s:%d\n", file, line);
		abort();
	}
}
static void dbg_closesocket(const char *file, int line, SOCKET s) {
	if (closesocket(s) == SOCKET_ERROR) {
		pWinsockError(WARN, "closesocket() failed at %s:%d", file, line);
	}
}
static void dbg_GetOverlappedResult(const char *file, int line, HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait) {
	BOOL b = GetOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait);
	DWORD dw = GetLastError();
	winet_log(INFO, "GetOverlappedResult(hFile=%p(%lld), nb=%d, ev=%p) == %d; err = %d\n",
		(void*)hFile, (long long)hFile, lpNumberOfBytesTransferred, (void*)lpOverlapped->hEvent, b, dw);
	SetLastError(dw);
	if (!b) {
		pWin32Error(ERR, "GetOverlappedResult() failed");
		abort();
	}
}

#define CloseHandle(hObject) dbg_CloseHandle(__FILE__, __LINE__, hObject)
#define closesocket(s) dbg_closesocket(__FILE__, __LINE__, s)
#define GetOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait) dbg_GetOverlappedResult(__FILE__, __LINE__, hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait)
#endif /* _DEBUG */
