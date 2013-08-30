#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#include "mylogging.h"
#include "myclipserver.h"
#include "myclipinternal.h"

#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#include <vector>
using namespace std;

#include "mylastheader.h"

static int counter=0;

static HWND nextWnd = NULL;

volatile HWND global_hwnd = NULL;

typedef struct cliplistener {
	char clipname[40+1];
	char host[40+1];
	short port;
} cliplistener;

static vector<cliplistener*> listeners;

static void unreg()
{
	HWND hwnd;
	hwnd = (HWND)InterlockedExchangePointer(&global_hwnd, NULL); /* ignore the "cast to greater size" warning */
	if (hwnd) {
		printf("Removing self from chain: %p <- %p\n", (void*)hwnd, (void*)nextWnd);
		if (!ChangeClipboardChain(hwnd, nextWnd) && GetLastError() != 0) {
			pWin32Error(ERR, "ChangeClipboardChain() failed");
		}
	}
}

DWORD clipsrv_nseq = 0;

static int enumformats() {
	UINT fmtid;
	while (!OpenClipboard(global_hwnd)) {
		//pWin32Error(ERR, "OpenClipboard() failed");
		Sleep(1);
	}

	for (fmtid = 0;;) {
		fmtid = EnumClipboardFormats(fmtid);
		if (fmtid == 0) {
			if (GetLastError() != ERROR_SUCCESS) {
				pWin32Error(ERR, "EnumClipboardFormats() failed");
				goto err;
			}
			break;
		}
		printf("%6d %d\n", counter, (int)fmtid);
		if (fmtid == MY_CF) {
			HGLOBAL hglob = GetClipboardData(fmtid);
			SIZE_T sz = GlobalSize(hglob);
			if (sz > sizeof(cliptun_data_header) + sizeof(net_uuid_t)) {
				const char *pbeg = (char*)GlobalLock(hglob);
				const char *pend = pbeg + sz;
				const char *p = pbeg;
				if (0 == memcmp(p, cliptun_data_header, sizeof(cliptun_data_header))) {
					p += sizeof(cliptun_data_header);

					net_uuid_t remoteaddr;
					memcpy(&remoteaddr, p, sizeof(net_uuid_t));
					p += sizeof(net_uuid_t);

					for(;;) {
						u_long netstate, netchannel;

						if (pend - p < sizeof(netchannel) + sizeof(netstate)) break;

						memcpy(&netchannel, p, sizeof(netchannel));
						p += sizeof(netchannel);

						memcpy(&netstate, p, sizeof(netstate));
						p += sizeof(netstate);

						cnnstate state = (cnnstate)ntohl(netstate);
						switch(state) {
							case STATE_SYN:
								for (vector<cliplistener*>::iterator it = listeners.begin(); it != listeners.end(); it++) {
									cliplistener *liplistener = *it;
									if (0 == strncmp(p, liplistener->clipname, pend - p)) {
										ClipConnection *cnn = new ClipConnection();
										cnn->state = STATE_EST;
										cnn->remote.clipaddr.addr = remoteaddr;
										cnn->remote.clipaddr.nchannel = netchannel;
										Tunnel *tun = new Tunnel(cnn);
										_clipsrv_reg_cnn(cnn);
										tun->deref();
										break;
									}
								}
								p += strlen(p)+1;
						}
					}
				}
				GlobalUnlock(hglob);
			}
		};
	}
err:
	if (!CloseClipboard()) {
		pWin32Error(ERR, "CloseClipboard() failed");
		return -1;
	}
}

static
LRESULT CALLBACK WindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
		case WM_CHANGECBCHAIN:
			counter++;
			{
				HWND wndRemove = (HWND)wParam;
				HWND wndNextNext = (HWND)lParam;
				printf("%6d WM_CHANGECBCHAIN %p %p\n", counter, (void*)wndRemove, (void*)wndNextNext);
				if (nextWnd == wndRemove) {
					printf("%6d saving next window %p\n", counter, (void*)wndNextNext);
					nextWnd = wndNextNext;
				} else if (nextWnd) {
					printf("%6d notifying next window %p\n", counter, (void*)nextWnd);
					return SendMessage(nextWnd,uMsg,wParam,lParam);
				} else {
					printf("%6d not notifying next window %p\n", counter, (void*)nextWnd);
				}
			}
			break;
		case WM_DRAWCLIPBOARD:
			counter++;
			// counter++; if (counter > 4) exit(0);
			printf("%6d WM_DRAWCLIPBOARD, clip seq = %d\n", counter, GetClipboardSequenceNumber());
			enumformats();
			if (nextWnd) {
				printf("%6d notifying next window %p\n", counter, (void*)nextWnd);
				return SendMessage(nextWnd,uMsg,wParam,lParam);
			} else {
				printf("%6d not notifying next window %p\n", counter, (void*)nextWnd);
			}
			break;
		case WM_USER:
			printf("WM_USER\n");
			unreg();
			break;
		default:
			return DefWindowProc( hwnd,uMsg,wParam,lParam);
	}
	return 0;
}

static void exitproc(void) {
	fprintf(stderr, "exitproc()\n");
	unreg();
}

static
BOOL WINAPI CtrlHandler( DWORD fdwCtrlType )
{
	fprintf(stderr, "CtrlHandler()\n");
	SendMessage(global_hwnd, WM_USER, 0, 0);

	switch ( fdwCtrlType ) {
		case CTRL_C_EVENT:
			fprintf(stderr, "\n\n Stopping due to Ctrl-C.\n" );
			break;

		case CTRL_CLOSE_EVENT:
			fprintf(stderr, "\n\n Stopping due to closing the window.\n" );
			break;
		
		case CTRL_BREAK_EVENT:
			fprintf(stderr, "\n\n Stopping due to Ctrl-Break.\n" );
			break;

		// Pass other signals to the next handler - ret = FALSE

		case CTRL_LOGOFF_EVENT:
			fprintf(stderr, "\n\n Stopping due to logoff.\n" );
			break;

		case CTRL_SHUTDOWN_EVENT:
			fprintf(stderr, "\n\n Stopping due to system shutdown.\n" );
			break;
		
		default:
			fprintf(stderr, "\n\n Stopping due to unknown event.\n" );
	}

	return FALSE;
}

HWND _createutilitywindow(WNDCLASS *wndclass)
{
	LPCTSTR lpClassName;
	HINSTANCE hinst = GetModuleHandle(NULL);
	HWND hwnd;

	wndclass->hInstance = hinst;
	lpClassName = (LPCTSTR)RegisterClass(wndclass);
	if (lpClassName == 0) {
		if (GetLastError() == ERROR_CLASS_ALREADY_EXISTS) {
			lpClassName = wndclass->lpszClassName;
		} else {
			pWin32Error(ERR, "RegisterClass() failed");
			return NULL;
		}
	}
	hwnd = CreateWindowEx(0, lpClassName, NULL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hinst, NULL);
	if (!hwnd) {
		pWin32Error(ERR, "CreateWindowEx() failed");
		return NULL;
	}
	return hwnd;
}

DWORD WINAPI clipmon_wnd_thread(void *param)
{
	MSG msg;

	createutilitywindowwithproc(&global_hwnd, WindowProc, _T("myclipmonitor"));
	if (global_hwnd == NULL) return 1;

	printf("hwnd = %p\n", (void*)global_hwnd);
	nextWnd = SetClipboardViewer(global_hwnd);
	if (!nextWnd && GetLastError() != 0) {
		pWin32Error(ERR, "SetClipboardViewer() failed");
		return -1;
	}
	printf("nextWnd = %p\n", (void*)nextWnd);

	atexit(exitproc);

	if (0 == SetConsoleCtrlHandler( CtrlHandler, TRUE )) {
		pWin32Error(ERR, "SetConsoleCtrlHandler() failed");
		return -1;
	}

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

int clipsrv_create_listener(const char clipname[40+1], const char host[40+1], short port)
{
	cliplistener *listener = (cliplistener*)malloc(sizeof(cliplistener));
	strcpy(listener->clipname, clipname);
	strcpy(listener->host, host);
	listener->port = port;
	listeners.push_back(listener);
	winet_log(INFO, "listening on clip %s\n", clipname);
	return 0;
}