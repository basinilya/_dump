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

#define MAX_PMAPS 128
#define CFGFILENAME "cliptund.conf"
#define ACCEPT_TIMEOUT 4
#define LSN_BKLOG 128


typedef struct s_portmap {
	SOCKET sock;
	union {
		struct {
			char *user;
			char *pass;
			char *cmdline;
		} lsn;
		struct {
			int i;
		} cnn;
	} a;
} portmap_t;


static _TCHAR *winet_a2t(char const *str, _TCHAR *buf, int size);
static void winet_evtlog(char const *logmsg, long type);
static int winet_log(int level, char const *fmt, ...);
static int winet_load_cfg(char const *cfgfile);
static int winet_create_listeners(void);
static void winet_cleanup(void);
static char *winet_get_syserror(void);
static _TCHAR *winet_inet_ntoa(struct in_addr addr, _TCHAR *buf, int size);

static int npmaps = 0;
static portmap_t pmaps[MAX_PMAPS];
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

static void __pWin32Error(int level, DWORD eNum, const char* fmt, va_list args)
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

		u = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
															NULL, eNum,
															MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
															pend - count, count, NULL );
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

void pWin32Error(char const *fmt, ...)
{
	va_list args;
	DWORD eNum = GetLastError();

	va_start(args, fmt);
	__pWin32Error(WINET_LOG_WARNING, eNum, fmt, args);
	va_end(args);
}

void pWinsockError(char const *fmt, ...)
{
	va_list args;
	DWORD eNum = WSAGetLastError();

	va_start(args, fmt);
	__pWin32Error(WINET_LOG_WARNING, eNum, fmt, args);
	va_end(args);
}

#ifdef _DEBUG
static void dbg_CloseHandle(const char *file, int line, HANDLE hObject) {
	if (!CloseHandle(hObject)) {
		pWin32Error("CloseHandle() failed at %s:%d", file, line);
	}
}
static void dbg_closesocket(const char *file, int line, SOCKET s) {
	if (closesocket(s) == SOCKET_ERROR) {
		pWinsockError("closesocket() failed at %s:%d", file, line);
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


static int winet_load_cfg(char const *cfgfile) {
	FILE *file;
	char *cmdline, *user, *pass;
	char cfgline[1024];
	int port;
	int timeo;
	struct sockaddr_in saddr;
	struct linger ling;

	if (!(file = fopen(cfgfile, "rt"))) {
		winet_log(WINET_LOG_ERROR, "[%s] unable to open config file: file='%s'\n",
			  WINET_APPNAME, cfgfile);
		return -1;
	}
	for (npmaps = 0; fgets(cfgline, sizeof(cfgline) - 1, file);) {
		cfgline[strlen(cfgline) - 1] = '\0';
		if (!isdigit(cfgline[0]))
			continue;
		port = atoi(cfgline);

		for (user = cfgline; isdigit(*user) || strchr(" \t", *user); user++);
		for (cmdline = user; *cmdline && !strchr(" \t", *cmdline); cmdline++);
		if (*cmdline) {
			*cmdline++ = '\0';
			for (; strchr(" \t", *cmdline); cmdline++);
			if (*cmdline) {
				if ((pass = strchr(user, ':')) != NULL)
					*pass++ = '\0';
				pmaps[npmaps].a.lsn.cmdline = strdup(cmdline);
				pmaps[npmaps].a.lsn.user = strdup(user);
				pmaps[npmaps].a.lsn.pass = pass ? strdup(pass): NULL;

				if ((pmaps[npmaps].sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
					winet_log(WINET_LOG_ERROR, "[%s] unable to create socket\n",
						  WINET_APPNAME);
					return -1;
				}

				if (sk_timeout > 0) {
					timeo = sk_timeout * 1000;
					if (setsockopt(pmaps[npmaps].sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeo, sizeof(timeo))) {
						winet_log(WINET_LOG_ERROR, "[%s] unable to set socket option: opt=SO_RCVTIMEO\n",
							  WINET_APPNAME);
						return -1;
					}
					timeo = sk_timeout * 1000;
					if (setsockopt(pmaps[npmaps].sock, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeo, sizeof(timeo))) {
						winet_log(WINET_LOG_ERROR, "[%s] unable to set socket option: opt=SO_SNDTIMEO\n",
							  WINET_APPNAME);
						return -1;
					}
				}

				ling.l_onoff = 1;
				ling.l_linger = linger_timeo;
				if (setsockopt(pmaps[npmaps].sock, SOL_SOCKET, SO_LINGER, (char *) &ling, sizeof(ling))) {
					winet_log(WINET_LOG_ERROR, "[%s] unable to set socket option: opt=SO_LINGER\n",
						  WINET_APPNAME);
					return -1;
				}

				memset(&saddr, 0, sizeof(saddr));
				saddr.sin_addr.S_un.S_addr = INADDR_ANY;
				saddr.sin_port = htons((short int) port);
				saddr.sin_family = AF_INET;

				if (bind(pmaps[npmaps].sock, (const struct sockaddr *) &saddr, sizeof(saddr))) {
					winet_log(WINET_LOG_ERROR, "[%s] unable to bind to port: port=%d\n",
						  WINET_APPNAME, port);
					return -1;
				}


				npmaps++;
			}
		}
	}

	fclose(file);

	if (!npmaps) {
		winet_log(WINET_LOG_ERROR, "[%s] empty config file: file='%s'\n",
			  WINET_APPNAME, cfgfile);
		return -1;
	}

	return 0;
}


static int winet_create_listeners(void) {
	int i;

	for (i = 0; i < npmaps; i++) {
		{
			int x;
			WSAEVENT ev = WSACreateEvent();
			hEvents_add(ev);
			x = WSAEventSelect(pmaps[i].sock, ev, FD_ACCEPT | FD_CLOSE);
			x = 0;
		}
		listen(pmaps[i].sock, LSN_BKLOG);
	}

	return 0;
}


static void winet_cleanup(void) {
	int i;

	/* TODO: Looks like we don't wait for threads, that may use 'user','pass' or 'envSnapshot'  */

	for (i = 0; i < npmaps; i++) {
		closesocket(pmaps[i].sock);
		if (pmaps[i].a.lsn.user)
			free(pmaps[i].a.lsn.user);
		if (pmaps[i].a.lsn.pass)
			free(pmaps[i].a.lsn.pass);
	}
}


static char *winet_get_syserror(void) {
	int len;
	LPVOID msg;
	char *emsg;

	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &msg,
		0,
		NULL);

	emsg = strdup((char *) msg);

	LocalFree(msg);

	if ((len = strlen(emsg)) > 0)
		emsg[len - 1] = '\0';

	return emsg;
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
	int i, adrlen;
	SOCKET asock;
	char const *cfgfile = NULL;
	WSADATA WD;
	struct sockaddr_in saddr;
	char cfgpath[MAX_PATH];
	int rc = 1;

	npmaps = 0;
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

	if (winet_load_cfg(cfgfile) < 0 ||
	    winet_create_listeners() < 0) {
		WSACleanup();
		return 2;
	}

	rc = 0;
	for (; !stopsvc;) {

		DWORD i_ev, x;
		WSANETWORKEVENTS netevs;
		WSAEVENT *evs = hEvents_get();

		i_ev = WSAWaitForMultipleEvents(hEvents_size(), evs, FALSE, INFINITE /*ACCEPT_TIMEOUT*1000*/, FALSE);
		if (i_ev < npmaps) {
		} else {
		}
		x = WSAEnumNetworkEvents(pmaps[i_ev].sock, evs[i_ev], &netevs);
		asock = accept(pmaps[i_ev].sock, NULL, NULL);

		asock = 0;
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
		for (i = 0; i < npmaps; i++) {
			/*
			if (!FD_ISSET(pmaps[i].sock, &lsnset))
				continue;
				*/

			adrlen = sizeof(saddr);
			if ((asock = accept(pmaps[i].sock, (struct sockaddr *) &saddr,
					    &adrlen)) != INVALID_SOCKET) {
				/*
				if (winet_handle_client(&pmaps[i], asock, &saddr) < 0) {

					winet_log(WINET_LOG_ERROR, "[%s] unable to serve client: %s:%d\n",
						  WINET_APPNAME, inet_ntoa(saddr.sin_addr), (int) ntohs(saddr.sin_port));

					closesocket(asock);
				} else {

					winet_log(WINET_LOG_MESSAGE, "[%s] client served: %s:%d -> '%s'\n",
						  WINET_APPNAME, inet_ntoa(saddr.sin_addr), (int) ntohs(saddr.sin_port),
						  pmaps[i].a.lsn.cmdline);

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

