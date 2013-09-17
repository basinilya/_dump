#ifdef _DEBUG
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#include <mswsock.h>
#include <windows.h>
#include "mylogging.h"

#undef _DEBUG
#include "mylastheader.h"

void dbg_CloseHandle(const char *file, int line, HANDLE hObject) {
	if (!CloseHandle(hObject)) {
		pWin32Error(ERR, "CloseHandle() failed at %s:%d", file, line);
		abort();
	}
}
void dbg_GetOverlappedResult(const char *file, int line, HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait) {
	BOOL b = GetOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait);
	DWORD dw = GetLastError();
	log(INFO, "GetOverlappedResult(hFile=%p(%lld), nb=%d, ev=%p) == %d; err = %d",
		(void*)hFile, (long long)hFile, *lpNumberOfBytesTransferred, (void*)lpOverlapped->hEvent, b, dw);
	SetLastError(dw);
	if (!b) {
		pWin32Error(ERR, "GetOverlappedResult() failed at %s:%d", file, line);
		abort();
	}
}
void dbg_WriteFile(const char *file, int line, HANDLE hFile,LPCVOID lpBuffer,DWORD nNumberOfBytesToWrite,LPDWORD lpNumberOfBytesWritten,LPOVERLAPPED lpOverlapped) {
	BOOL b = WriteFile(hFile,lpBuffer,nNumberOfBytesToWrite,lpNumberOfBytesWritten,lpOverlapped);
	DWORD dw = GetLastError();
	log(INFO, "WriteFile(hFile=%p(%lld), bufsz=%d, nb=%d, ev=%p) == %d; err = %d",
		(void*)hFile, (long long)hFile, nNumberOfBytesToWrite, *lpNumberOfBytesWritten, (void*)lpOverlapped->hEvent, b, dw);
	if (!b && dw != ERROR_IO_PENDING) {
		pWin32Error(ERR, "WriteFile() failed at %s:%d", file, line);
		abort();
	}
	SetLastError(dw);
	//return b;
}
void dbg_ReadFile(const char *file, int line, HANDLE hFile,LPVOID lpBuffer,DWORD nNumberOfBytesToRead,LPDWORD lpNumberOfBytesRead,LPOVERLAPPED lpOverlapped) {
	BOOL b = ReadFile(hFile,lpBuffer,nNumberOfBytesToRead,lpNumberOfBytesRead,lpOverlapped);
	DWORD dw = GetLastError();
	log(INFO, "ReadFile(hFile=%p(%lld), bufsz=%d, nb=%d, ev=%p) == %d; err = %d",
		(void*)hFile, (long long)hFile, nNumberOfBytesToRead, *lpNumberOfBytesRead, (void*)lpOverlapped->hEvent, b, dw);
	if (!b && dw != ERROR_IO_PENDING) {
		pWin32Error(ERR, "ReadFile() failed at %s:%d", file, line);
		abort();
	}
	SetLastError(dw);
	//return b;
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
		pWinsockError(ERR, "socket() failed");
		abort();
	}
	return s;
}
void dbg_listen(const char *file, int line, SOCKET s, int backlog) {
	int rc = listen(s,backlog);
	if (0 != rc) {
		pWinsockError(ERR, "listen() failed");
		abort();
	}
}
#endif /* _DEBUG */
