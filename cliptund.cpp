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

#include "cliptund.h"
#include "myeventloop.h"
#include "mylogging.h"
#include "myportlistener.h"
#include "myclipserver.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
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


#include "mylastheader.h"

using namespace cliptund;

#define CFGFILENAME "cliptund.conf"
//#define ACCEPT_TIMEOUT 4


static _TCHAR *winet_a2t(char const *str, _TCHAR *buf, int size);
static void winet_evtlog(char const *logmsg, long type);
static int winet_load_cfg(char const *cfgfile);
static void winet_cleanup(void);

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
		u = _snprintf(pend - count, count, "[%s] ", WINET_APPNAME);
		if (u >= count) break;
		count -= u;

		u = (unsigned)_vsnprintf(pend - count, count, fmt, args);
		if (u >= count) break;
		count -= u;

		u = (unsigned)_snprintf(pend - count, count, ": 0x%08x (%d): ", eNum, eNum);
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
		/*
		if (u == 0) {
			u = (unsigned)_snprintf(pend - count, count, "0x%08x (%d)", eNum, eNum);
		}
		*/
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
	LPCTSTR strs[2];
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


int winet_log(int level, char const *fmt, ...)
{
	va_list args;
	char emsg[1024];

	va_start(args, fmt);
	_vsnprintf(emsg, sizeof(emsg) - 1, fmt, args);
	va_end(args);

	return _winet_log(level, emsg);
}



/*
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
*/


#undef STRINGIZE2
#undef STRINGIZE
#define STRINGIZE2(x) #x
#define STRINGIZE(x) STRINGIZE2(x)

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
	word_t words[8];
	int npmaps;

	if (!(conf_file = fopen(cfgfilename, "rt"))) {
		winet_log(WINET_LOG_ERROR, "[%s] unable to open config file: file='%s'\n",
			  WINET_APPNAME, cfgfilename);
		return -1;
	}

	npmaps = 0;
	while (!feof(conf_file)) {
		rc = fscanf(conf_file, SCANWRD SKIPSP SCANWRD SKIPSP SCANWRD SKIPSP SCANWRD SKIPSP SCANWRD SKIPSP SCANWRD SKIPSP SCANWRD SKIPSP SCANWRD
			, words[0], words[1], words[2], words[3], words[4], words[5], words[6], words[7]);
		fscanf(conf_file, "%*[^\n]"); /* read till EOL */
		fscanf(conf_file, "\n"); /* read EOL */

		int n = 0;
		word_t *pword = words, *pwordend = &words[rc+1];
		if (pword < pwordend && 0 == strcmp(*pword, "forward")) {
			pword++;
			if (pword < pwordend) {
				ConnectionFactory *connfact;
				if (0 == strcmp(*pword, "host")) {
					word_t *host;
					pword++;
					if (pword < pwordend) {
						host = pword;
						pword++;
						if (pword < pwordend && 0 == strcmp(*pword, "port")) {
							pword++;
							if (pword < pwordend && sscanf(*pword, "%hd", &port) == 1) {
								pword++;
								connfact = tcp_CreateConnectionFactory(*host, port);
								goto aaa;
							}
						}
					}
				} else if (0 == strcmp(*pword, "clip")) {
					word_t *clipname;
					pword++;
					if (pword < pwordend) {
						clipname = pword;
						pword++;
						connfact = clipsrv_CreateConnectionFactory(*clipname);
						goto aaa;
					}
				}
				continue;
				aaa:
				if (pword < pwordend && 0 == strcmp(*pword, "listen")) {
					pword++;
					if (pword < pwordend && 0 == strcmp(*pword, "port")) {
						pword++;
						if (pword < pwordend && sscanf(*pword, "%hd", &port) == 1) {
							tcp_create_listener(connfact, port);
							npmaps++;
						}
					} else if (pword < pwordend && 0 == strcmp(*pword, "clip")) {
						word_t *clipname;
						pword++;
						if (pword < pwordend) {
							clipname = pword;
							clipsrv_create_listener(connfact, *clipname);
							npmaps++;
						}
					}
				}
			}
		}
#if 0
		if (rc == 6 && sscanf(s3, "%hd", &port) == 1
			&& 0 == strcmp(s1, "listen")
			&& 0 == strcmp(s2, "port")
			&& 0 == strcmp(s4, "forward")
			&& 0 == strcmp(s5, "clip"))
		{
			ConnectionFactory *connfact = clipsrv_CreateConnectionFactory(s6);
			rc = tcp_create_listener(port, connfact);
			if (rc == -1) {
				fclose(conf_file);
				return -1;
			}
			if (rc == 0) npmaps++;
		}
		else if (rc == 8 && sscanf(s8, "%hd", &port) == 1
			&& 0 == strcmp(s1, "listen")
			&& 0 == strcmp(s2, "clip")
			&& 0 == strcmp(s4, "forward")
			&& 0 == strcmp(s5, "host")
			&& 0 == strcmp(s7, "port"))
		{
			ConnectionFactory *connfact = tcp_CreateConnectionFactory(s6, port);
			rc = clipsrv_create_listener(connfact, s3);
			if (rc == -1) {
				fclose(conf_file);
				return -1;
			}
			if (rc == 0) npmaps++;
		}
#endif

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

int winet_inet_aton(const char *cp, struct in_addr *inp)
{
	static const ADDRINFO hints = {
		AI_NUMERICHOST, /*  int             ai_flags;*/
		AF_INET,        /*  int             ai_family;*/
		0,              /*  int             ai_socktype;*/
		0,              /*  int             ai_protocol;*/
		0,              /*  size_t          ai_addrlen;*/
		NULL,           /*  char            *ai_canonname;*/
		NULL,           /*  struct sockaddr  *ai_addr;*/
		NULL            /*  struct addrinfo  *ai_next;*/
	};
	ADDRINFO *pres;
	int i;

	if (!cp || cp[0] == '\0') return 0;
	i = getaddrinfo(cp, NULL, &hints, &pres);

	if (i == 0) {
		inp->S_un.S_addr = ((struct sockaddr_in *)pres->ai_addr)->sin_addr.S_un.S_addr;
		freeaddrinfo(pres);
	}

	return !i;
}

_TCHAR *winet_inet_ntoa(struct in_addr addr, _TCHAR *buf, int size) {
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

	evloop_init();

	if (winet_load_cfg(cfgfile) < 0) {
		WSACleanup();
		return 2;
	}

	clipsrv_init();

	rc = 0;
	for (; !stopsvc;) {
		evloop_processnext();
	}

//cleanup:
	winet_cleanup();
	WSACleanup();

	return rc;
}

