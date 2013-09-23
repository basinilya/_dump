#ifdef DEBUG_CLIPTUND
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#include <mswsock.h>
#include <windows.h>
#include "mylogging.h"
#include "rfifo.h"

#undef DEBUG_CLIPTUND
#include "mylastheader.h"

DWORD whenclipopened;

volatile LONG currentclipowner = 0;

void dbg_KillTimer(HWND hWnd, UINT_PTR uIDEvent) {
	BOOL b;
	b = KillTimer(hWnd, uIDEvent);
	if (!b) {
		pWin32Error(ERR, "KillTimer() failed");
		abort();
	}
}

void dumpdata(const char *data, int sz, char const *fmt, ...)
{
	char msg[1024];
	va_list args;
	int i;
	char dst[10*2+1];
	for (i = 0; i < sz && i < 10; i++) {
		sprintf(&dst[i*2], "%02X", (unsigned char)data[i]);
	}

	va_start(args, fmt);
	vsprintf(msg, fmt, args);
	va_end(args);
	log(INFO, "%s: %s", msg, dst);
}

void dbg_rfifo_markwrite(rfifo_t *rfifo, rfifo_long count)
{
	{
		HANDLE h;
		DWORD nb;
		void *p;
		char filename[100];
		sprintf(filename, "aaa-%p.dat", rfifo);
		h = CreateFile(filename, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		p = rfifo_pfree(rfifo);
		dumpdata(p, (int)count, "writing %ld at %lx to %p", (long)count, (long)rfifo->ofs_end, rfifo);
		WriteFile(h, p, count, &nb, NULL);
		CloseHandle(h);
	}
	rfifo_markwrite123(rfifo, count);
}

void dbg_SetConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine, BOOL Add)
{
	if (0 == SetConsoleCtrlHandler(HandlerRoutine, Add)) {
		pWin32Error(ERR, "SetConsoleCtrlHandler() failed");
		abort();
	}
}

HWND dbg_SetClipboardViewer(HWND hWndNewViewer)
{
	HWND hwndres;
	hwndres = SetClipboardViewer(hWndNewViewer);
	if (!hwndres && GetLastError() != 0) {
		pWin32Error(ERR, "SetClipboardViewer() failed");
		abort();
	}
	return hwndres;
}

BOOL dbg_OpenClipboard(HWND hwnd) {
	BOOL b;
	DWORD dw;
	if (currentclipowner == GetCurrentThreadId()) {
		log(ERR, "Clipboard already open");
		abort();
	}
	b = OpenClipboard(hwnd);
	dw = GetLastError();
	if (b) {
		currentclipowner = GetCurrentThreadId();
		whenclipopened = GetTickCount();
		//log(INFO, "opened clipboard %p", (void*)hwnd);
	}
	SetLastError(dw);
	return b;
}

BOOL dbg_EmptyClipboard()
{
	BOOL b;
	DWORD dw;
	log(DBG, "         EmptyClipboard() begin");
	b = EmptyClipboard();
	dw = GetLastError();
	log(DBG, "         EmptyClipboard() end");
	if (!b) {
		SetLastError(dw);
		pWin32Error(WARN, "EmptyClipboard() failed");
	}
	SetLastError(dw);
	return b;
}

HANDLE dbg_SetClipboardData(UINT uFormat, HANDLE hMem) {
	HANDLE h;
	DWORD dw;
	log(DBG, "         SetClipboardData(%u, %p) begin", uFormat, hMem);
	SetLastError(ERROR_SUCCESS);
	h = SetClipboardData(uFormat, hMem);
	dw = GetLastError();
	log(DBG, "         SetClipboardData(%u, %p) returned %p", uFormat, hMem, (void*)h);
	if (!h && (hMem || dw != ERROR_SUCCESS)) {
		SetLastError(dw);
		pWin32Error(WARN, "SetClipboardData() failed");
	}
	SetLastError(dw);
	return h;
}

HANDLE dbg_GetClipboardData(UINT uFormat) {
	HANDLE h;
	DWORD dw;
	log(DBG, "         GetClipboardData(%u) begin", uFormat);
	h = GetClipboardData(uFormat);
	dw = GetLastError();
	log(DBG, "         GetClipboardData(%u) returned %p", uFormat, (void*)h);
	SetLastError(dw);
	return h;
}

void dbg_CloseClipboard() {
	DWORD whenclipclosed;
	DWORD curthread = GetCurrentThreadId();
	log(DBG, "         CloseClipboard() begin");
	if (curthread != InterlockedCompareExchange(&currentclipowner, 0, curthread)) {
		log(WARN, "Clipboard wasn't opened by this thread");
	}
	whenclipclosed = GetTickCount();
	if (!CloseClipboard()) {
		pWin32Error(WARN, "CloseClipboard() failed");
	}
	log(DBG, "         CloseClipboard() end");
	//log(INFO, "closed clipboard after %d mks", i);
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
	char buf1[20];
	if (lpOverlapped) {
		b = DuplicateHandle(GetCurrentProcess(), lpOverlapped->hEvent, GetCurrentProcess(), &copyev, DUPLICATE_SAME_ACCESS, FALSE, 0);
	}
	b = WriteFile(hFile,lpBuffer,nNumberOfBytesToWrite,lpNumberOfBytesWritten,lpOverlapped);
	dw = GetLastError();
	if (lpOverlapped) {
		sprintf(buf1, "%p", (void*)lpOverlapped->hEvent);
	} else {
		strcpy(buf1, "N/A");
	}
	log(DBG, "WriteFile(hFile=%p(%lld), bufsz=%d, nb=%d, ev=%s) == %d; err = %d",
		(void*)hFile, (long long)hFile, nNumberOfBytesToWrite, *lpNumberOfBytesWritten, buf1, b, dw);
	if (!b && dw != ERROR_IO_PENDING) {
		SetLastError(dw);
		pWin32Error(DBG, "WriteFile() failed");
		if (lpOverlapped) {
			if (WAIT_OBJECT_0 == WaitForSingleObject(copyev, 0)) {
				pWin32Error(ERR, "The event was unexpectedly set at: %s:%d", file, line);
				abort();
			}
		}
	}
	if (lpOverlapped) {
		CloseHandle(copyev);
	}
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
		const char *sHow;
		char buf[20];
		switch (how) {
			case SD_SEND:
				sHow = "SD_SEND";
				break;
			case SD_RECEIVE:
				sHow = "SD_RECEIVE";
				break;
			case SD_BOTH:
				sHow = "SD_BOTH";
				break;
			default:
				sHow = buf;
				sprintf(buf, "%d", how);
		}
		pWinsockError(ERR, "shutdown(%d, %s) failed at %s:%d", (int)s, sHow, file, line);
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
#endif /* DEBUG_CLIPTUND */
