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

struct Cliplistener {
	u_long net_channel;
	char clipname[40+1];
	ConnectionFactory *connfact;
	~Cliplistener() {
		delete connfact;
	}
};

static vector<Cliplistener*> listeners;

static void unreg()
{
	HWND hwnd;
	hwnd = (HWND)InterlockedExchangePointer((void**)&global_hwnd, NULL); /* ignore the "cast to greater size" warning */
	if (hwnd) {
		printf("Removing self from chain: %p <- %p\n", (void*)hwnd, (void*)nextWnd);
		if (!ChangeClipboardChain(hwnd, nextWnd) && GetLastError() != 0) {
			pWin32Error(ERR, "ChangeClipboardChain() failed");
		}
	}
}

static void parsepacket() {
	char buf[MAXPACKETSIZE];
	const char *pbeg, *pend, *p;


	union {
		clipaddr remote;
		struct {
			net_uuid_t addr;
			union {
				char c[1];
				subpackheader header;
				subpack_syn syn;
				subpack_data data;
			};
		};
	} u;

	int flag = 0;

	while (!OpenClipboard(global_hwnd)) {
		Sleep(1);
	}

	HGLOBAL hglob = GetClipboardData(MY_CF);
	if (hglob) {
		SIZE_T sz = GlobalSize(hglob);
		if (sz > sizeof(cliptun_data_header) + sizeof(net_uuid_t)) {
			p = pbeg = (char*)GlobalLock(hglob);
			pend = pbeg + sz;
			if (0 == memcmp(p, cliptun_data_header, sizeof(cliptun_data_header))) {
				p += sizeof(cliptun_data_header);

				memcpy(&u.remote.addr, p, sizeof(net_uuid_t));
				p += sizeof(net_uuid_t);

				memcpy(buf, p, pend - p);
				pend = buf + (pend - p);
				p = buf;
				flag = 1;
				printf("received %d\n", (int)sz);
			}
			GlobalUnlock(hglob);
		}
	}
	if (!CloseClipboard()) {
		pWin32Error(WARN, "CloseClipboard() failed");
	}
	if (flag) for (;;) {

		if (pend - p < sizeof(subpackheader_base)) break;

		memcpy(&u.header, p, sizeof(subpackheader_base));
		p += sizeof(subpackheader_base);

		packtype_t packtype = (packtype_t)ntohl(u.header.net_packtype);
		switch(packtype) {
			case PACK_SYN:
				EnterCriticalSection(&ctx.lock);
				for (vector<ClipConnection*>::iterator it = ctx.connections.begin(); it != ctx.connections.end(); it++) {
					ClipConnection *cnn = *it;
					if (0 == memcmp(&cnn->remote.clipaddr, &u.remote, sizeof(clipaddr)) && 0 == strncmp(p, cnn->local.clipname, pend - p)) {
						cnn->prev_recv_pos--;
						_clipsrv_havenewdata();
						LeaveCriticalSection(&ctx.lock);
						goto after_clipname;
					}
				}
				LeaveCriticalSection(&ctx.lock);
				for (vector<Cliplistener*>::iterator it = listeners.begin(); it != listeners.end(); it++) {
					Cliplistener *cliplistener = *it;
					if (0 == strncmp(p, cliplistener->clipname, pend - p)) {
						ClipConnection *cnn = new ClipConnection(NULL);
						Tunnel *tun = cnn->tun;
						strcpy(cnn->local.clipname, cliplistener->clipname);
						cnn->local.nchannel = cliplistener->net_channel;
						cnn->state = STATE_NEW_SRV;
						cnn->remote.clipaddr = u.remote;
						_clipsrv_reg_cnn(cnn);
						cliplistener->connfact->connect(tun);
						tun->Release();
						break;
					}
				}
				after_clipname:
				p += strlen(p)+1;
				break;
			case PACK_ACK:
				memcpy(u.c + sizeof(subpackheader_base), p, sizeof(subpack_ack) - sizeof(subpackheader_base));
				p += sizeof(subpack_ack) - sizeof(subpackheader_base);
				EnterCriticalSection(&ctx.lock);
				for (vector<ClipConnection*>::iterator it = ctx.connections.begin(); it != ctx.connections.end(); it++) {
					ClipConnection *cnn = *it;
					if (
						1
						//&& 0 == memcmp(&cnn->remote.clipaddr, &u.remote, sizeof(clipaddr))
						&& 0 == memcmp(&ctx.localclipuuid.net, &u.header.dst.addr, sizeof(net_uuid_t))
						&& cnn->local.nchannel == u.header.dst.nchannel
						)
					{
						if (cnn->state == STATE_SYN) {
							cnn->state = STATE_EST;
							cnn->remote.clipaddr = u.remote;
							cnn->tun->connected();
							break;
						} else if (0 == memcmp(&cnn->remote.clipaddr, &u.remote, sizeof(clipaddr))) {
							rfifo_t *rfifo = &cnn->pump_send->buf;
							long prev_pos = ntohl(u.data.net_prev_pos);
							long count = ntohl(u.data.net_count);
							long ofs_beg = (u_long)rfifo->ofs_beg;
							if (ofs_beg - prev_pos >= 0) {
								rfifo_confirmread(rfifo, prev_pos + count - ofs_beg);
								cnn->pump_recv->bufferavail();
							}
							break;
						}
					}
				}
				LeaveCriticalSection(&ctx.lock);
				break;
			case PACK_DATA:
				memcpy(u.c + sizeof(subpackheader_base), p, subpack_data_size(0) - sizeof(subpackheader_base));
				p += subpack_data_size(0) - sizeof(subpackheader_base);
				long count;
				count = ntohl(u.data.net_count);
				EnterCriticalSection(&ctx.lock);
				for (vector<ClipConnection*>::iterator it = ctx.connections.begin(); it != ctx.connections.end(); it++) {
					ClipConnection *cnn = *it;
					if (
						0 == memcmp(&cnn->remote.clipaddr, &u.remote, sizeof(clipaddr))
						&& 0 == memcmp(&ctx.localclipuuid.net, &u.header.dst.addr, sizeof(net_uuid_t))
						&& cnn->local.nchannel == u.header.dst.nchannel
						)
					{
						rfifo_t *rfifo = &cnn->pump_recv->buf;
						long prev_pos = ntohl(u.data.net_prev_pos);
						long ofs_end = (u_long)rfifo->ofs_end;
						if (ofs_end - prev_pos >= 0) {
							long n = (u_long)cnn->prev_recv_pos;
							n -= prev_pos;
							if (n > 0) {
								cnn->prev_recv_pos -= n;
							}

							long datasz = prev_pos + count - ofs_end;
							long bufsz = rfifo_availwrite(rfifo);
							if (datasz > bufsz) datasz = bufsz;
							memcpy(rfifo_pfree(rfifo), p, datasz);
							rfifo_markwrite(rfifo, datasz);
							_clipsrv_havenewdata();
							cnn->pump_send->havedata();
						}
						break;
					}
				}
				LeaveCriticalSection(&ctx.lock);
				p += count;
				break;
			case PACK_FIN:
				memcpy(u.c + sizeof(subpackheader_base), p, sizeof(subpackheader) - sizeof(subpackheader_base));
				p += sizeof(subpackheader) - sizeof(subpackheader_base);
				EnterCriticalSection(&ctx.lock);
				for (vector<ClipConnection*>::iterator it = ctx.connections.begin(); it != ctx.connections.end(); it++) {
					ClipConnection *cnn = *it;
					if (
						0 == memcmp(&cnn->remote.clipaddr, &u.remote, sizeof(clipaddr))
						&& 0 == memcmp(&ctx.localclipuuid.net, &u.header.dst.addr, sizeof(net_uuid_t))
						&& cnn->local.nchannel == u.header.dst.nchannel
						)
					{
						cnn->pump_recv->eof = 1;
						cnn->pump_send->havedata();
						break;
					}
				}
				break;
			default:
				abort();
		}
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
			//printf("%6d WM_DRAWCLIPBOARD, clip seq = %d\n", counter, GetClipboardSequenceNumber());
			parsepacket();
			if (nextWnd) {
				//printf("%6d notifying next window %p\n", counter, (void*)nextWnd);
				return SendMessage(nextWnd,uMsg,wParam,lParam);
			} else {
				//printf("%6d not notifying next window %p\n", counter, (void*)nextWnd);
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

	//printf("hwnd = %p\n", (void*)global_hwnd);
	nextWnd = SetClipboardViewer(global_hwnd);
	if (!nextWnd && GetLastError() != 0) {
		pWin32Error(ERR, "SetClipboardViewer() failed");
		return -1;
	}
	//printf("nextWnd = %p\n", (void*)nextWnd);

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

int clipsrv_create_listener(ConnectionFactory *connfact, const char clipname[40+1])
{
	Cliplistener *listener = new Cliplistener();
	listener->net_channel = htonl(InterlockedIncrement(&ctx.nchannel));
	listener->connfact = connfact;
	strcpy(listener->clipname, clipname);
	listeners.push_back(listener);
	winet_log(INFO, "listening on clip %s\n", clipname);
	return 0;
}
