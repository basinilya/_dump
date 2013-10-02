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


static int winet_load_cfg(char const *cfgfile);
static void winet_cleanup(void);

static int sk_timeout = -1;
static int linger_timeo = 60;
static int stopsvc;

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
		log(ERR, "unable to open config file: '%s'", cfgfilename);
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
								goto connfact_ok;
							}
						}
					}
				} else if (0 == strcmp(*pword, "clip")) {
					pword++;
					if (pword < pwordend) {
						word_t *pclipname = pword;
						pword++;
						connfact = clipsrv_CreateConnectionFactory(*pclipname);
						goto connfact_ok;
					}
				}
				continue;
				connfact_ok:
				if (pword < pwordend && 0 == strcmp(*pword, "listen")) {
					pword++;
					if (pword < pwordend && 0 == strcmp(*pword, "port")) {
						pword++;
						if (pword < pwordend && sscanf(*pword, "%hd", &port) == 1) {
							if (0 == tcp_create_listener(connfact, port)) {
								npmaps++;
							}
						}
					} else if (pword < pwordend && 0 == strcmp(*pword, "clip")) {
						pword++;
						if (pword < pwordend) {
							word_t *pclipname = pword;
							clipsrv_create_listener(connfact, *pclipname);
							npmaps++;
						}
					}
				}
			}
		}
	}
	fclose(conf_file);
	if (!npmaps) {
		log(ERR, "empty config file: '%s'\n", cfgfilename);
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

char *winet_inet_ntoa(struct in_addr addr, char *buf, int size) {
	char const *ip;

	ip = inet_ntoa(addr);

	return strncpy(buf, ip, size);
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
		log(ERR, "unable to initialize socket layer");
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

ULONG SimpleRefcount::AddRef() {
	LONG newrefcount = InterlockedIncrement(&this->refcount);
	//winet_log(INFO, "SimpleRefcount::AddRef %p %ld\n", this, (long)newrefcount);
	return newrefcount;
}

ULONG SimpleRefcount::Release() {
	LONG newrefcount = InterlockedDecrement(&this->refcount);
	//winet_log(INFO, "SimpleRefcount::Release %p %ld\n", this, (long)newrefcount);
	if (newrefcount == 0) {
		delete this;
	}
	return newrefcount;
}
