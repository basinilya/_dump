/*
 *  Based on WinInetd by Davide Libenzi ( Inetd-like daemon for Windows )
 *  Copyright 2013  Ilja Basin
 *  Copyright (C) 2003  Davide Libenzi
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Davide Libenzi <davidel@xmailserver.org>
 *
 */

#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "cliptund.h"



#define WINET_LOG_MESSAGE 1
#define WINET_LOG_WARNING 2
#define WINET_LOG_ERROR 3

/* local */
#undef INFO
#undef WARN
#undef ERR
#define INFO WINET_LOG_MESSAGE
#define WARN WINET_LOG_WARNING
#define ERR WINET_LOG_ERROR

#define CFGFILENAME "cliptund.conf"
#define ACCEPT_TIMEOUT 4
#define LSN_BKLOG 128


static _TCHAR *winet_a2t(char const *str, _TCHAR *buf, int size);
static void winet_evtlog(char const *logmsg, long type);
static int winet_log(int level, char const *fmt, ...);
static int winet_load_cfg(char const *cfgfile);
static void winet_cleanup(void);
static _TCHAR *winet_inet_ntoa(struct in_addr addr, _TCHAR *buf, int size);

static int sk_timeout = -1;
static int linger_timeo = 60;
static int stopsvc;


static _TCHAR *winet_a2t(char const *str, _TCHAR *buf, int size) {

#ifdef _UNICODE
	MultiByteToWideChar(CP_ACP, 0, str, strlen(str), buf, size);
#else
	strncpy(buf, str, size);
#endif
	return buf;
}

static int _winet_log(int level, char const *emsg)
{
	printf("%s", emsg);

	if (level == WINET_LOG_ERROR)
		winet_evtlog(emsg, EVENTLOG_ERROR_TYPE);

	return 0;
}

static char *cleanstr(char *s)
{
	while(*s) {
		switch((int)*s){
			case 13:
			case 10:
			*s=' ';
			break;
		}
		s++;
	}
	return s;
}


static void __pWin32Error(int level, char mode, DWORD eNum, const char* fmt, va_list args)
{
	char emsg[1024];
	char *pend = emsg + sizeof(emsg);
	size_t count = sizeof(emsg);
	unsigned u;

	do {
		u = (unsigned)_snprintf(pend - count, count, "[%s] ", WINET_APPNAME);
		if (u >= count) break;
		count -= u;

		u = (unsigned)_vsnprintf(pend - count, count, fmt, args);
		if (u >= count) break;
		count -= u;

		u = (unsigned)_snprintf(pend - count, count, ": ");
		if (u >= count) break;
		count -= u;

		if (mode == 'w') {
			u = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
																NULL, eNum,
																MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
																pend - count, count, NULL );
		} else if (mode == 's') {
			u = (unsigned)_snprintf(pend - count, count, "%s", strerror(eNum));
		}
		if (u == 0) {
			u = (unsigned)_snprintf(pend - count, count, "0x%08x (%d)", eNum, eNum);
		}
	} while(0);

	emsg[sizeof(emsg)-1] = '\0';
	pend = cleanstr(emsg);

	if (pend < emsg + sizeof(emsg)-1) {
		pend++;
		*pend = '\0';
	}
	pend[-1] = '\n';
	_winet_log(level, emsg);
}

void pSysError(int lvl, char const *fmt, ...)
{
	va_list args;
	int myerrno = errno;

	va_start(args, fmt);
	__pWin32Error(lvl, 's', myerrno, fmt, args);
	va_end(args);
}

void pWin32Error(int lvl, char const *fmt, ...)
{
	va_list args;
	DWORD eNum = GetLastError();

	va_start(args, fmt);
	__pWin32Error(lvl, 'w', eNum, fmt, args);
	va_end(args);
}

void pWinsockError(int lvl, char const *fmt, ...)
{
	va_list args;
	DWORD eNum = WSAGetLastError();

	va_start(args, fmt);
	__pWin32Error(lvl, 'w', eNum, fmt, args);
	va_end(args);
}

#ifdef _DEBUG
static void dbg_CloseHandle(const char *file, int line, HANDLE hObject) {
	if (!CloseHandle(hObject)) {
		pWin32Error(WARN, "CloseHandle() failed at %s:%d", file, line);
	}
}
static void dbg_closesocket(const char *file, int line, SOCKET s) {
	if (closesocket(s) == SOCKET_ERROR) {
		pWinsockError(WARN, "closesocket() failed at %s:%d", file, line);
	}
}
#define CloseHandle(hObject) dbg_CloseHandle(__FILE__, __LINE__, hObject)
#define closesocket(s) dbg_closesocket(__FILE__, __LINE__, s)
#endif /* _DEBUG */


static void winet_evtlog(char const *logmsg, long type) {
	DWORD err;
	HANDLE hesrc;
	LPTSTR tmsg;
	_TCHAR lmsg[128];
	LPTSTR strs[2];
	_TCHAR wmsg[1024];

	winet_a2t(logmsg, wmsg, COUNTOF(wmsg));
	tmsg = wmsg;

	err = GetLastError();
	hesrc = RegisterEventSource(NULL, _TEXT(WINET_APPNAME));

	_stprintf(lmsg, _TEXT("%s error: 0x%08x"), _TEXT(WINET_APPNAME), err);
	strs[0] = lmsg;
	strs[1] = tmsg;

	if (hesrc != NULL) {
		ReportEvent(hesrc, (WORD) type, 0, 0, NULL, 2, 0, strs, NULL);

		DeregisterEventSource(hesrc);
	}
}


static int winet_log(int level, char const *fmt, ...) {
	va_list args;
	char emsg[1024];

	va_start(args, fmt);
	_vsnprintf(emsg, sizeof(emsg) - 1, fmt, args);
	va_end(args);

	return _winet_log(level, emsg);
}

#undef STRINGIZE2
#undef STRINGIZE
#define STRINGIZE2(x) #x
#define STRINGIZE(x) STRINGIZE2(x)

#define ADDRESSLENGTH (sizeof(struct sockaddr_in)+16)

typedef struct data_accept {
	SOCKET lsock;
	SOCKET asock;
	OVERLAPPED overlap;
	char buf[ADDRESSLENGTH*2];
} data_accept;

typedef struct data_recv {
	SOCKET sock;
} data_recv;

static int recv_handler_func(DWORD i_event, WSAEVENT ev, void *param)
{
	WSANETWORKEVENTS netevs;
	data_recv *data = (data_recv *)param;

	if (0 != WSAEnumNetworkEvents(data->sock, ev, &netevs)) {
		pWinsockError(ERR, "WSAEnumNetworkEvents() failed");
		return -1;
	}
	if (netevs.lNetworkEvents & FD_READ) {
		int i;
		char buf[100];
		i = netevs.iErrorCode[FD_READ_BIT];
		if (i != 0) {
			winet_log(ERR, "[%s] FD_READ failed with error %d\n", WINET_APPNAME, i);
			return -1;
		}
		i = recv(data->sock, buf, 100, 0);
		i = 0;
	}

	return 0;
}

static int accept_handler_func(DWORD i_event, WSAEVENT ev, void *param)
{
	data_accept *data = (data_accept *)param;
	DWORD nb;
	struct sockaddr_in localSockaddr, remoteSockaddr;
	struct sockaddr_in *pLocalSockaddr, *pRemoteSockaddr;
	int lenL, lenR;

	if (!GetOverlappedResult((HANDLE)data->lsock, &data->overlap, &nb, FALSE)) {
		pWin32Error(ERR, "GetOverlappedResult() failed");
		return -1;
	}

	GetAcceptExSockaddrs(data->buf, 0, ADDRESSLENGTH, ADDRESSLENGTH, (LPSOCKADDR*)&pLocalSockaddr, &lenL, (LPSOCKADDR*)&pRemoteSockaddr, &lenR);
	localSockaddr = *pLocalSockaddr;
	remoteSockaddr = *pRemoteSockaddr;

	{
		int i;
		int adrlen;
		struct sockaddr_in saddr;
		data_recv *newdata;
		WSAEVENT ev;
		SOCKET asock;
		wsaevent_handler recv_handler = { recv_handler_func };

		recv_handler.param = newdata = (data_recv *)malloc(sizeof(data_recv));
		if (!data) {
			pSysError(ERR, "malloc() failed");
			return -1;
		}

		adrlen = sizeof(saddr);
		asock = newdata->sock = accept(data->lsock, (struct sockaddr *) &saddr, &adrlen);
		if (asock == INVALID_SOCKET) {
			pWinsockError(ERR, "accept() failed");
			goto cleanup1;
		}
		ev = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (ev == WSA_INVALID_EVENT) {
			pWinsockError(ERR, "CreateEvent() failed");
			goto cleanup2;
		}
		if (0 != WSAEventSelect(asock, ev, FD_READ | FD_WRITE | FD_CLOSE)) {
			pWinsockError(ERR, "WSAEventSelect() failed");
			goto cleanup3;
		}
		hEvents_add(ev, &recv_handler);
		return 0;
	cleanup3:
		CloseHandle(ev);
	cleanup2:
		closesocket(asock);
	cleanup1:
		free(newdata);
		return -1;
	}
	return 0;
}

static DWORD WINAPI foo(
    LPVOID lpThreadParameter
    )
{
	Sleep(2000);
	//CloseHandle((HANDLE)lpThreadParameter);
	closesocket((SOCKET)lpThreadParameter);
	return 0;
}


static int winet_create_listener(short port)
{
	int rc = -1;
	struct sockaddr_in saddr;
	data_accept *newdata;
	DWORD nb;
	HANDLE ev;
	SOCKET lsock, asock;
	wsaevent_handler accept_handler = { accept_handler_func };

	accept_handler.param = newdata = (data_accept *)malloc(sizeof(data_accept));
	if (!newdata) {
		pSysError(ERR, "malloc() failed");
		return -1;
	}

	memset(&newdata->overlap, 0, sizeof(OVERLAPPED));
	newdata->overlap.hEvent = ev = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (ev == WSA_INVALID_EVENT) {
		pWin32Error(ERR, "CreateEvent() failed");
		goto cleanup1;
	}

	if ((newdata->lsock = lsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		pWinsockError(ERR, "socket() failed");
		goto cleanup2;
	}

	if ((newdata->asock = asock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		pWinsockError(ERR, "socket() failed");
		goto cleanup3;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_addr.S_un.S_addr = INADDR_ANY;
	saddr.sin_port = htons((short int) port);
	saddr.sin_family = AF_INET;

	if (0 != bind(lsock, (const struct sockaddr *) &saddr, sizeof(saddr))) {
		pWinsockError(WARN, "failed to bind port %d", port);
		rc = -2; /* not fatal */
		goto cleanup4;
	}

	if (0 != listen(lsock, LSN_BKLOG)) {
		pWinsockError(ERR, "listen() failed");
		goto cleanup4;
	}

	if (!AcceptEx(lsock, asock, newdata->buf, 0, ADDRESSLENGTH, ADDRESSLENGTH, &nb, &newdata->overlap) && WSAGetLastError() != ERROR_IO_PENDING) {
		pWinsockError(ERR, "AcceptEx() failed");
		goto cleanup4;
	}
	hEvents_add(ev, &accept_handler);
	winet_log(INFO, "[%s] listening on port: %d\n", WINET_APPNAME, port);
	//CreateThread(NULL, 0, foo, lsock, 0, &nb);
	return 0;
cleanup4:
	closesocket(asock);
cleanup3:
	closesocket(lsock);
cleanup2:
	CloseHandle(ev);
cleanup1:
	free(newdata);
	return rc;
}


static int winet_load_cfg(char const *cfgfilename) {
	#undef WORDSZ
	#define WORDSZ 40
	#undef SP
	#define SP " \t\f\v\r"
	#undef SCANWRD
	#define SCANWRD "%" STRINGIZE(WORDSZ) "[^" SP "\n" "]" /* does not skip leading white spaces */
	#define SKIPSP "%*[" SP "]" /* does not skip '\n' */
	typedef char word_t[WORDSZ+1];
	int rc;
	FILE *conf_file;
	short port;
	word_t s_listen, s_port, s_theport, s_forward, s_clip, s_clipname;
	int npmaps;

	if (!(conf_file = fopen(cfgfilename, "rt"))) {
		winet_log(WINET_LOG_ERROR, "[%s] unable to open config file: file='%s'\n",
			  WINET_APPNAME, cfgfilename);
		return -1;
	}

	npmaps = 0;
	while (!feof(conf_file)) {
		rc = fscanf(conf_file, SCANWRD SKIPSP SCANWRD SKIPSP SCANWRD SKIPSP SCANWRD SKIPSP SCANWRD SKIPSP SCANWRD
			, s_listen, s_port, s_theport, s_forward, s_clip, s_clipname);
		fscanf(conf_file, "%*[^\n]"); /* read till EOL */
		fscanf(conf_file, "\n"); /* read till EOL */
		if (rc == 6 && sscanf(s_theport, "%hd", &port) == 1
			&& 0 == strcmp(s_listen, "listen")
			&& 0 == strcmp(s_port, "port")
			&& 0 == strcmp(s_forward, "forward")
			&& 0 == strcmp(s_clip, "clip"))
		{
			rc = winet_create_listener(port);
			if (rc == -1) {
				fclose(conf_file);
				return -1;
			}
			if (rc == 0) npmaps++;
		}
	}
	fclose(conf_file);
	if (!npmaps) {
		winet_log(ERR, "[%s] empty config file: file='%s'\n", WINET_APPNAME, cfgfilename);
		return -1;
	}
	return 0;
}

static void winet_cleanup(void) {
	/* TODO: Looks like we don't wait for threads, that may use 'user','pass' or 'envSnapshot'  */
/*
	for (i = 0; i < npmaps; i++) {
		closesocket(pmaps[i].sock);
		if (pmaps[i].a.lsn.user)
			free(pmaps[i].a.lsn.user);
		if (pmaps[i].a.lsn.pass)
			free(pmaps[i].a.lsn.pass);
	}
	*/
}


static _TCHAR *winet_inet_ntoa(struct in_addr addr, _TCHAR *buf, int size) {
	char const *ip;

	ip = inet_ntoa(addr);

	return winet_a2t(ip, buf, size);
}


int winet_stop_service(void) {

	stopsvc++;
	return 0;
}

int winet_main(int argc, char const **argv) {
	int i;
	char const *cfgfile = NULL;
	WSADATA WD;
	char cfgpath[MAX_PATH];
	int rc = 1;

	sk_timeout = -1;
	stopsvc = 0;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--cfgfile")) {
			if (++i < argc)
				cfgfile = argv[i];
		} else if (!strcmp(argv[i], "--timeout")) {
			if (++i < argc)
				sk_timeout = atoi(argv[i]);
		} else if (!strcmp(argv[i], "--linger-timeout")) {
			if (++i < argc)
				linger_timeo = atoi(argv[i]);
		}

	}
	if (!cfgfile) {
		i = (int) GetWindowsDirectoryA(cfgpath, sizeof(cfgpath) - sizeof(CFGFILENAME) - 1);
		if (cfgpath[i - 1] != '\\')
			strcat(cfgpath, "\\");
		strcat(cfgpath, CFGFILENAME);
		cfgfile = cfgpath;
	}

	if (WSAStartup(MAKEWORD(2, 0), &WD)) {
		winet_log(WINET_LOG_ERROR, "[%s] unable to initialize socket layer\n",
			  WINET_APPNAME);
		return 1;
	}

	if (winet_load_cfg(cfgfile) < 0) {
		WSACleanup();
		return 2;
	}

	rc = 0;
	for (; !stopsvc;) {
		int funcrslt;
		DWORD i_ev;
		WSAEVENT *tmp_evs;
		wsaevent_handler *handler;

		tmp_evs = hEvents_getall();

		i_ev = WSAWaitForMultipleEvents(hEvents_size(), tmp_evs, FALSE, INFINITE /*ACCEPT_TIMEOUT*1000*/, FALSE);
		{
			int sfsdf;
			//SOCKET lsock = ((data_accept *)datas_get(i_ev)->param)->lsock;
			//SOCKET asock;
			
			//asock = accept(lsock, NULL, NULL);
			//SOCKET asock = accept(lsock, (struct sockaddr *) &saddr, &adrlen);
			//asock = 0;
		}
		switch (i_ev) {
			case WSA_WAIT_FAILED:
				pWinsockError(ERR, "WSAWaitForMultipleEvents() failed");
				return 1;
			/*case WSA_WAIT_TIMEOUT:
				break;*/
			default:
				handler = datas_get(i_ev);
				funcrslt = handler->func(i_ev, tmp_evs[i_ev], handler->param);
				if (funcrslt == -1) return 1;
		}

		//netevs.
			/*
		if (!(selres = select(0, &lsnset, NULL, NULL, &tmo))) {

			continue;
		}
		if (selres < 0) {
			winet_log(WINET_LOG_WARNING, "[%s] select error\n", WINET_APPNAME);
			continue;
		}
		*/
		for (i = 0; i < 5/*npmaps*/; i++) {
			/*
			if (!FD_ISSET(pmaps[i].sock, &lsnset))
				continue;
				*/
/*
			adrlen = sizeof(saddr);
			if ((asock = accept(pmaps[i].sock, (struct sockaddr *) &saddr,
					    &adrlen)) != INVALID_SOCKET)
						*/
						{
						
				/*
				if (winet_handle_client(&pmaps[i], asock, &saddr) < 0) {

					winet_log(WINET_LOG_ERROR, "[%s] unable to serve client: %s:%d\n",
						  WINET_APPNAME, inet_ntoa(saddr.sin_addr), (int) ntohs(saddr.sin_port));

					closesocket(asock);
				} else {

					winet_log(WINET_LOG_MESSAGE, "[%s] client served: %s:%d -> '%s'\n",
						  WINET_APPNAME, inet_ntoa(saddr.sin_addr), (int) ntohs(saddr.sin_port),
						  pmaps[i].a.lsn.arg2);

				}
				*/
			}
		}
	}

//cleanup:
	winet_cleanup();
	WSACleanup();

	return rc;
}

