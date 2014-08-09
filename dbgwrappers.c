#ifdef DEBUG_MYPROG
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#include <mswsock.h>
#include <windows.h>
#include "mylogging.h"

#undef DEBUG_MYPROG
#include "mylastheader.h"

DWORD whenclipopened;

static volatile LONG currentclipowner = 0;

void dbg_KillTimer(HWND hWnd, UINT_PTR uIDEvent, const char *timername)
{
	BOOL b;
	log(DBG, "KillTimer(%s)", timername);
	b = KillTimer(hWnd, uIDEvent);
	if (!b) {
		pWin32Error(ERR, "KillTimer() failed");
		abort();
	}
}

HANDLE dbg_CreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName)
{
	HANDLE ev = CreateEvent(lpEventAttributes, bManualReset, bInitialState, lpName);
	if (!ev) {
		pWin32Error(ERR, "CreateEvent() failed");
		abort();
	}
	return ev;
}

HGLOBAL dbg_GlobalAlloc(UINT uFlags, SIZE_T dwBytes)
{
	HGLOBAL hglob = GlobalAlloc(uFlags, dwBytes);
	if (hglob == NULL) {
		pWin32Error(ERR, "GlobalAlloc() failed");
		abort();
	}
	return hglob;
}

UINT_PTR dbg_SetTimer(HWND hWnd,UINT_PTR nIDEvent,UINT uElapse,TIMERPROC lpTimerFunc, const char *timername)
{
	log(DBG, "SetTimer(%u, %s)", uElapse, timername);
	return SetTimer(hWnd,nIDEvent,uElapse,lpTimerFunc);
}

#define DUMPBYTES 128
void dumpdata(const char *data, int sz, char const *fmt, ...)
{
	char msg[1024];
	va_list args;
	int i;
	char dst[DUMPBYTES*2+1];
	return;
	dst[0] = '\0';
	for (i = 0; i < sz && i < DUMPBYTES; i++) {
		sprintf(&dst[i*2], "%02X", (unsigned char)data[i]);
	}

	va_start(args, fmt);
	vsprintf(msg, fmt, args);
	va_end(args);
	log(DBG, "%s: %s", msg, dst);
}

void dbg_SetConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine, BOOL Add)
{
	if (0 == SetConsoleCtrlHandler(HandlerRoutine, Add)) {
		pWin32Error(ERR, "SetConsoleCtrlHandler() failed");
		abort();
	}
}

UINT dbg_EnumClipboardFormats(UINT format)
{
	UINT fmtid = EnumClipboardFormats(format);
	if (fmtid == 0 && GetLastError() != ERROR_SUCCESS) {
		pWin32Error(ERR, "EnumClipboardFormats() failed");
		abort();
	}
	return fmtid;
}

HWND dbg_SetClipboardViewer(HWND hWndNewViewer)
{
	HWND hwndres;
	SetLastError(ERROR_SUCCESS);
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
		//log(DBG, "opened clipboard %p", (void*)hwnd);
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
	//log(DBG, "closed clipboard after %d mks", i);
}

void dbg_OpenProcessToken(HANDLE ProcessHandle, DWORD DesiredAccess, PHANDLE TokenHandle)
{
	if (!OpenProcessToken(ProcessHandle, DesiredAccess, TokenHandle)) {
		pWin32Error(ERR, "OpenProcessToken() failed");
		abort();
	}
}
void dbg_LookupPrivilegeValue(LPCTSTR lpSystemName, LPCTSTR lpName, PLUID   lpLuid)
{
	if (!LookupPrivilegeValue(lpSystemName, lpName, lpLuid)) {
		pWin32Error(ERR, "LookupPrivilegeValue() failed");
		abort();
	}
}
void dbg_AdjustTokenPrivileges(HANDLE TokenHandle, BOOL DisableAllPrivileges, PTOKEN_PRIVILEGES NewState, DWORD BufferLength, PTOKEN_PRIVILEGES PreviousState, PDWORD ReturnLength)
{
	if (!AdjustTokenPrivileges(TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength)) {
		pWin32Error(ERR, "AdjustTokenPrivileges() failed");
		abort();
	}
}
void dbg_SetFilePointerEx(HANDLE hFile, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod)
{
	if (!SetFilePointerEx(hFile, liDistanceToMove, lpNewFilePointer, dwMoveMethod)) {
		pWin32Error(ERR, "SetFilePointerEx() failed");
		abort();
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

void dbg_CancelIo(HANDLE hFile)
{
	log(DBG, "CancelIo(hFile=%p(%lld))", (void*)hFile, (long long)hFile);
	CancelIo(hFile);
}

void dbg_WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	if (!WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped)) {
		pWin32Error(ERR, "WriteFile() failed");
		abort();
	}
}
void dbg_ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
	if (!ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped)) {
		pWin32Error(ERR, "ReadFile() failed");
		abort();
	}
}

BOOL dbg_WriteFile123(const char *file, int line, HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
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
BOOL dbg_ReadFile123(const char *file, int line, HANDLE hFile,LPVOID lpBuffer,DWORD nNumberOfBytesToRead,LPDWORD lpNumberOfBytesRead,LPOVERLAPPED lpOverlapped) {
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

#endif /* DEBUG_MYPROG */
