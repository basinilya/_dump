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

#define WINET_VERSION "0.1"

int winet_stop_service(void);
int winet_main(int argc, char const **argv);
char *winet_inet_ntoa(struct in_addr addr, char *buf, int size);
int winet_inet_aton(const char *cp, struct in_addr *inp);

#ifdef __cplusplus
}
#endif

#include <atlbase.h>

typedef IUnknown ISimpleRefcount;

struct SimpleRefcount : ISimpleRefcount {
	volatile LONG refcount;
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject) { abort(); return 0; }
	inline SimpleRefcount() : refcount(1) {}
protected:
	virtual ~SimpleRefcount() {}
};

#define DeclRefcountMethods(x) \
	ULONG STDMETHODCALLTYPE AddRef() { return x AddRef(); } \
	ULONG STDMETHODCALLTYPE Release() { return x Release(); } \
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject) { abort(); return 0; }

#endif /* #if !defined(_CLIPTUND_H) */

