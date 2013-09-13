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

#if !defined(_CLIPTUND_H)
#define _CLIPTUND_H

#include <winsock2.h>
#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WINET_APPNAME "cliptund"
#define WINET_VERSION "0.7p.a" /* 'pump' or 'pipes' */


#define COUNTOF(a) (sizeof(a) / sizeof(a[0]))



int winet_stop_service(void);
int winet_main(int argc, char const **argv);
_TCHAR *winet_inet_ntoa(struct in_addr addr, _TCHAR *buf, int size);
int winet_inet_aton(const char *cp, struct in_addr *inp);

/*
typedef struct wsaevent_handler {
	int (*func)(DWORD i_event, WSAEVENT ev, void *param);
	void *param;
} wsaevent_handler;

void hEvents_add(WSAEVENT ev, const wsaevent_handler *handler_model);
WSAEVENT *hEvents_getall();
DWORD hEvents_size();
wsaevent_handler *datas_get(DWORD index);
*/

#ifdef __cplusplus
}
#endif

struct ISimpleRefcount {
	virtual void addref() = 0;
	virtual void deref() = 0;
};

struct SimpleRefcount : ISimpleRefcount {
	volatile LONG refcount;
	void addref();
	void deref();
	inline SimpleRefcount() : refcount(1) {}
protected:
	virtual ~SimpleRefcount() {}
};

#endif /* #if !defined(_CLIPTUND_H) */

