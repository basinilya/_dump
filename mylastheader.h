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

#include "rfifo.h"

#ifdef __cplusplus
extern "C" {
#endif

void dbg_KillTimer(HWND hWnd, UINT_PTR uIDEvent, const char *timername);
HANDLE dbg_CreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName);
HGLOBAL dbg_GlobalAlloc(UINT uFlags, SIZE_T dwBytes);
void dbg_CancelIo(HANDLE hFile);
UINT_PTR dbg_SetTimer(HWND hWnd,UINT_PTR nIDEvent,UINT uElapse,TIMERPROC lpTimerFunc, const char *timername);
void dumpdata(const char *data, int sz, char const *fmt, ...);
void dbg_rfifo_markwrite(rfifo_t *rfifo, rfifo_long count);
void dbg_SetConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine, BOOL Add);
UINT dbg_EnumClipboardFormats(UINT format);
HWND dbg_SetClipboardViewer(HWND hWndNewViewer);
BOOL dbg_EmptyClipboard();
HANDLE dbg_SetClipboardData(UINT uFormat, HANDLE hMem);
HANDLE dbg_GetClipboardData(UINT uFormat);
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

#ifdef DEBUG_CLIPTUND

#define KillTimer(hWnd, uIDEvent) dbg_KillTimer(hWnd, uIDEvent, #uIDEvent)
#undef CreateEvent
#define CreateEvent dbg_CreateEvent
#define GlobalAlloc dbg_GlobalAlloc
#define CancelIo dbg_CancelIo
#define SetTimer(hWnd,nIDEvent,uElapse,lpTimerFunc) dbg_SetTimer(hWnd,nIDEvent,uElapse,lpTimerFunc, #lpTimerFunc)
//#define rfifo_markwrite dbg_rfifo_markwrite
#define EnumClipboardFormats dbg_EnumClipboardFormats
#define SetClipboardViewer dbg_SetClipboardViewer
#define SetConsoleCtrlHandler dbg_SetConsoleCtrlHandler
#define EmptyClipboard dbg_EmptyClipboard
#define SetClipboardData dbg_SetClipboardData
#define GetClipboardData dbg_GetClipboardData
#define OpenClipboard dbg_OpenClipboard
#define CloseClipboard dbg_CloseClipboard
#define CloseHandle(hObject) dbg_CloseHandle(__FILE__, __LINE__, hObject)
#define GetOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait) dbg_GetOverlappedResult(__FILE__, __LINE__, hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait)
//#define WriteFile(hFile,lpBuffer,nNumberOfBytesToWrite,lpNumberOfBytesWritten,lpOverlapped) dbg_WriteFile(__FILE__, __LINE__, hFile,lpBuffer,nNumberOfBytesToWrite,lpNumberOfBytesWritten,lpOverlapped)
//#define ReadFile(hFile,lpBuffer,nNumberOfBytesToRead,lpNumberOfBytesRead,lpOverlapped) dbg_ReadFile(__FILE__, __LINE__,hFile,lpBuffer,nNumberOfBytesToRead,lpNumberOfBytesRead,lpOverlapped)

#define AcceptEx(sListenSocket,sAcceptSocket,lpOutputBuffer,dwReceiveDataLength,dwLocalAddressLength,dwRemoteAddressLength,lpdwBytesReceived,lpOverlapped) dbg_AcceptEx(__FILE__, __LINE__,sListenSocket,sAcceptSocket,lpOutputBuffer,dwReceiveDataLength,dwLocalAddressLength,dwRemoteAddressLength,lpdwBytesReceived,lpOverlapped)
#define closesocket(s) dbg_closesocket(__FILE__, __LINE__, s)
#define shutdown(s, how) dbg_shutdown(__FILE__, __LINE__, s, how)
#define socket(af,type,protocol) dbg_socket(__FILE__, __LINE__, af,type,protocol)
#define listen(s,backlog) dbg_listen(__FILE__, __LINE__, s,backlog)
#define getpeername(s,name,namelen) dbg_getpeername(__FILE__, __LINE__, s,name,namelen)

#endif /* DEBUG_CLIPTUND */
