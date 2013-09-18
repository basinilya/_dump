#ifdef _DEBUG
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#include <mswsock.h>
#include <windows.h>
#include "mylogging.h"

#undef _DEBUG
#include "mylastheader.h"

void dbg_CloseClipboard() {
	if (!CloseClipboard()) {
		pWin32Error(WARN, "CloseClipboard() failed");
	}
}

void dbg_CloseHandle(const char *file, int line, HANDLE hObject) {
	if (!CloseHandle(hObject)) {
		pWin32Error(ERR, "CloseHandle() failed at %s:%d", file, line);
		abort();
	}
}
BOOL dbg_GetOverlappedResult(const char *file, int line, HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait) {
	BOOL b = GetOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait);
	DWORD dw = GetLastError();
	log(DBG, "GetOverlappedResult(hFile=%p(%lld), nb=%d, ev=%p) == %d; err = %d",
		(void*)hFile, (long long)hFile, *lpNumberOfBytesTransferred, (void*)lpOverlapped->hEvent, b, dw);
	if (!b) {
		SetLastError(dw);
		pWin32Error(DBG, "GetOverlappedResult() failed");
	}
	SetLastError(dw);
	return b;
}
BOOL dbg_WriteFile(const char *file, int line, HANDLE hFile,LPCVOID lpBuffer,DWORD nNumberOfBytesToWrite,LPDWORD lpNumberOfBytesWritten,LPOVERLAPPED lpOverlapped) {
	HANDLE copyev;
	BOOL b;
	DWORD dw;
	b = DuplicateHandle(GetCurrentProcess(), lpOverlapped->hEvent, GetCurrentProcess(), &copyev, DUPLICATE_SAME_ACCESS, FALSE, 0);
	b = WriteFile(hFile,lpBuffer,nNumberOfBytesToWrite,lpNumberOfBytesWritten,lpOverlapped);
	dw = GetLastError();
	log(DBG, "WriteFile(hFile=%p(%lld), bufsz=%d, nb=%d, ev=%p) == %d; err = %d",
		(void*)hFile, (long long)hFile, nNumberOfBytesToWrite, *lpNumberOfBytesWritten, (void*)lpOverlapped->hEvent, b, dw);
	if (!b && dw != ERROR_IO_PENDING) {
		SetLastError(dw);
		pWin32Error(DBG, "WriteFile() failed");
		if (WAIT_OBJECT_0 == WaitForSingleObject(copyev, 0)) {
			pWin32Error(ERR, "The event was unexpectedly set at: %s:%d", file, line);
			abort();
		}
	}
	CloseHandle(copyev);
	SetLastError(dw);
	return b;
}
BOOL dbg_ReadFile(const char *file, int line, HANDLE hFile,LPVOID lpBuffer,DWORD nNumberOfBytesToRead,LPDWORD lpNumberOfBytesRead,LPOVERLAPPED lpOverlapped) {
	HANDLE copyev;
	BOOL b;
	DWORD dw;
	b = DuplicateHandle(GetCurrentProcess(), lpOverlapped->hEvent, GetCurrentProcess(), &copyev, DUPLICATE_SAME_ACCESS, FALSE, 0);
	b = ReadFile(hFile,lpBuffer,nNumberOfBytesToRead,lpNumberOfBytesRead,lpOverlapped);
	dw = GetLastError();
	log(DBG, "ReadFile(hFile=%p(%lld), bufsz=%d, nb=%d, ev=%p) == %d; err = %d",
		(void*)hFile, (long long)hFile, nNumberOfBytesToRead, *lpNumberOfBytesRead, (void*)lpOverlapped->hEvent, b, dw);
	if (!b && dw != ERROR_IO_PENDING) {
		SetLastError(dw);
		pWin32Error(ERR, "ReadFile() failed");
		if (WAIT_OBJECT_0 == WaitForSingleObject(copyev, 0)) {
			pWin32Error(ERR, "The event was unexpectedly set at: %s:%d", file, line);
			abort();
		}
	}
	CloseHandle(copyev);
	SetLastError(dw);
	return b;
}

void dbg_AcceptEx(const char *file, int line, SOCKET sListenSocket,SOCKET sAcceptSocket,PVOID lpOutputBuffer,DWORD dwReceiveDataLength,DWORD dwLocalAddressLength,DWORD dwRemoteAddressLength,LPDWORD lpdwBytesReceived,LPOVERLAPPED lpOverlapped) {
	BOOL b = AcceptEx(sListenSocket,sAcceptSocket,lpOutputBuffer,dwReceiveDataLength,dwLocalAddressLength,dwRemoteAddressLength,lpdwBytesReceived,lpOverlapped);
	DWORD dw = WSAGetLastError();
	if (!b && dw != ERROR_IO_PENDING) {
		pWinsockError(ERR, "AcceptEx() failed");
		abort();
	}
	WSASetLastError(dw);
}

void dbg_closesocket(const char *file, int line, SOCKET s) {
	if (closesocket(s) == SOCKET_ERROR) {
		pWinsockError(ERR, "closesocket() failed at %s:%d", file, line);
		abort();
	}
}
void dbg_shutdown(const char *file, int line, SOCKET s, int how) {
	int rc = shutdown(s, how);
	if (rc != 0) {
		pWinsockError(ERR, "shutdown(%d, SD_SEND) failed at %s:%d", (int)s, file, line);
		abort();
	}
}
SOCKET dbg_socket(const char *file, int line, int af,int type,int protocol) {
	SOCKET s = socket(af,type,protocol);
	if (s == INVALID_SOCKET) {
		pWinsockError(ERR, "socket() failed at %s:%d", (int)s, file, line);
		abort();
	}
	return s;
}
void dbg_listen(const char *file, int line, SOCKET s, int backlog) {
	int rc = listen(s,backlog);
	if (0 != rc) {
		pWinsockError(ERR, "listen() failed at %s:%d", (int)s, file, line);
		abort();
	}
}
void dbg_getpeername(const char *file, int line, SOCKET s,struct sockaddr * name,int * namelen) {
	if (0 != getpeername(s,name,namelen)) {
		pWinsockError(ERR, "getpeername() failed at %s:%d", (int)s, file, line);
		abort();
	}
}
#endif /* _DEBUG */
