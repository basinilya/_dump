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
	long npacket;

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
		bool remoteequal(const ClipConnection *cnn) const {
			return 0 == memcmp(&cnn->remote.clipaddr, &remote, sizeof(clipaddr));
		}
		bool localequal(const ClipConnection *cnn) const {
			return
				0 == memcmp(&ctx.localclipuuid.net, &header.dst.addr, sizeof(net_uuid_t))
				&& cnn->local.nchannel == header.dst.nchannel;
		}
		bool localandremoteequal(const ClipConnection *cnn) const {
			return localequal(cnn) && remoteequal(cnn);
		}
	} u;

	int flag = 0;

	_clipsrv_OpenClipboard(global_hwnd);

	HGLOBAL hglob = GetClipboardData(MY_CF);
	if (hglob) {
		SIZE_T sz = GlobalSize(hglob);
		if (sz > sizeofpacketheader) {
			p = pbeg = (char*)GlobalLock(hglob);
			pend = pbeg + sz;
			if (0 == memcmp(p, cliptun_data_header, sizeof(cliptun_data_header))) {
				p += sizeof(cliptun_data_header);

				u_long ul;
				memcpy(&ul, p, sizeof(u_long));
				p += sizeof(u_long);
				npacket = ntohl(ul);

				memcpy(&u.remote.addr, p, sizeof(net_uuid_t));
				p += sizeof(net_uuid_t);

				memcpy(buf, p, pend - p);
				pend = buf + (pend - p);
				p = buf;
				flag = 1;
				//printf("received %d\n", (int)sz);
			}
			GlobalUnlock(hglob);
		}
	}
	CloseClipboard();
	if (flag) {
		if (0 != memcmp(&u.remote.addr, &ctx.localclipuuid.net, sizeof(net_uuid_t)))
		{
			RPC_CSTR s;
			UuidToStringA(&u.remote._align, &s);
			log(INFO, "received packet: %s %ld", s, npacket);
			RpcStringFree(&s);
		}

		EnterCriticalSection(&ctx.lock);

		for (; pend != p;) {
			ClipConnection *cnn;
			#define FIND_cnn(cond) do { for (vector<ClipConnection*>::iterator it = ctx.connections.begin();; it++) { \
					if (it == ctx.connections.end()) { cnn = NULL; break; } cnn = *it; if (cond) break; \
				} } while(0)

			#define AAA(a, b) do { \
					if (pend - p < ((b) - (a))) { \
						log(WARN, "corrupted subpacket"); \
						goto break_loop; \
					} \
					memcpy(&u.c + (a), p, ((b) - (a))); \
					p += ((b) - (a)); \
				} while(0)

			AAA(0, sizeof(subpackheader_base));

			packtype_t packtype = (packtype_t)ntohl(u.header.net_packtype);
			switch(packtype) {
				case PACK_SYN:
					FIND_cnn(u.remoteequal(cnn) && 0 == strncmp(p, cnn->local.clipname, pend - p));
					if (cnn) {
						cnn->prev_recv_pos--;
						cnn->havedata();
					} else {
						for (vector<Cliplistener*>::iterator it = listeners.begin(); it != listeners.end(); it++) {
							Cliplistener *cliplistener = *it;
							if (0 == strncmp(p, cliplistener->clipname, pend - p)) {
								log(INFO, "accepted clip %ld -> %s", ntohl(u.remote.nchannel), cliplistener->clipname);
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
					}
					p += strlen(p)+1;
					break;
				case PACK_ACK:
					AAA(sizeof(subpackheader_base), sizeof(subpack_ack));

					for (vector<ClipConnection*>::iterator it = ctx.connections.begin(); it != ctx.connections.end(); it++) {
						cnn = *it;
						if (u.localequal(cnn))
						{
							if (cnn->state == STATE_SYN) {
								log(INFO, "connected clip %ld -> %s", ntohl(cnn->local.nchannel), cnn->remote.clipname);
								cnn->state = STATE_EST;
								cnn->remote.clipaddr = u.remote;
								cnn->tun->connected();
								break;
							} else if (u.remoteequal(cnn)) {
								rfifo_t *rfifo = &cnn->pump_send->buf;
								long prev_pos = ntohl(u.data.net_prev_pos);
								long count = ntohl(u.data.net_count);
								long ofs_beg = (u_long)rfifo->ofs_beg;
								if (ofs_beg - prev_pos >= 0) {
									cnn->resend_counter = 0;
									rfifo_confirmread(rfifo, prev_pos + count - ofs_beg);
									cnn->pump_recv->bufferavail();
								}
								break;
							}
						}
					}
					break;
				case PACK_DATA:
					AAA(sizeof(subpackheader_base), subpack_data_size(0));

					long count;
					count = ntohl(u.data.net_count);
					if ( (unsigned long)count > (size_t)(pend - p) ) {
						log(WARN, "corrupted subpacket");
						goto break_loop;
					}

					FIND_cnn(u.localandremoteequal(cnn));
					if (cnn)
					{
						rfifo_t *rfifo = &cnn->pump_recv->buf;
						long prev_pos = ntohl(u.data.net_prev_pos);
						long ofs_end = (u_long)rfifo->ofs_end;
						if (ofs_end - prev_pos >= 0) {
							if (cnn->prev_recv_pos - prev_pos > 0) {
								cnn->prev_recv_pos = prev_pos;
							}

							long datasz = prev_pos + count - ofs_end;
							long bufsz = rfifo_availwrite(rfifo);
							if (datasz > bufsz) datasz = bufsz;
							memcpy(rfifo_pfree(rfifo), p, datasz);
							rfifo_markwrite(rfifo, datasz);
							cnn->havedata();
							cnn->pump_send->havedata();
						}
					}
					p += count;
					break;
				case PACK_FIN:
					AAA(sizeof(subpackheader_base), sizeof(subpack_ack));

					FIND_cnn(u.localandremoteequal(cnn));
					if (cnn)
					{
						rfifo_t *rfifo = &cnn->pump_recv->buf;
						long prev_pos = ntohl(u.data.net_prev_pos);
						long ofs_end = (u_long)rfifo->ofs_end;
						if (ofs_end == prev_pos) {
							if (!cnn->pump_recv->eof) {
								log(INFO, "disconnected clip %ld", ntohl(u.remote.nchannel));
								cnn->pump_recv->eof = 1;
								cnn->pump_send->havedata();
							}
						}
					}
					break;
				default:
					abort();
			}
		}
		break_loop:
		LeaveCriticalSection(&ctx.lock);
	}
}

static int in_parsepacket = 0;

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

			if (!in_parsepacket) {
				in_parsepacket = 1;
				parsepacket();
				in_parsepacket = 0;
			}

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
	in_parsepacket = 1;
	nextWnd = SetClipboardViewer(global_hwnd); // May send WM_DRAWCLIPBOARD
	in_parsepacket = 0;
	if (!nextWnd && GetLastError() != 0) {
		pWin32Error(ERR, "SetClipboardViewer() failed");
		exit(1);
	}
	//printf("nextWnd = %p\n", (void*)nextWnd);

	atexit(exitproc);

	if (0 == SetConsoleCtrlHandler( CtrlHandler, TRUE )) {
		pWin32Error(ERR, "SetConsoleCtrlHandler() failed");
		exit(1);
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
	log(INFO, "listening on clip %s", clipname);
	return 0;
}
