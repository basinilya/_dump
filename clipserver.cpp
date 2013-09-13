#include "myclipserver.h"
#include "myclipinternal.h"

#include "myeventloop.h"
#include "mylogging.h"

#include <windows.h>
#include <rpc.h>
#include <stdio.h>
#include <tchar.h>

#include <vector>
using namespace std;

#include "mylastheader.h"

#define MAX_FORMATS 100

const char cliptun_data_header[] = CLIPTUN_DATA_HEADER;

clipsrvctx ctx;


static HBITMAP dupbitmap(HBITMAP hbitmap)
{
	HDC dcSrc, dcDst;
	HBITMAP bDst;
	BITMAP bitmap;
	HGDIOBJ hbmOld, hbmOld2;
	HBITMAP rcBitmap = NULL;

	if (GetObject(hbitmap, sizeof(BITMAP), &bitmap) == 0) {
		pWin32Error(ERR, "GetObject() failed");
		return NULL;
	}
	dcSrc = CreateCompatibleDC(NULL);
	if (!dcSrc) {
		pWin32Error(ERR, "CreateCompatibleDC() failed");
		return NULL;
	}
	hbmOld = SelectObject(dcSrc, hbitmap);
	if (!hbmOld || hbmOld == HGDI_ERROR) {
		pWin32Error(ERR, "SelectObject() failed");
		goto err;
	}
	dcDst = CreateCompatibleDC(NULL);
	if (!dcDst) {
		pWin32Error(ERR, "CreateCompatibleDC() failed");
		goto err;
	}
	bDst = CreateBitmap(bitmap.bmWidth, bitmap.bmHeight, bitmap.bmPlanes, bitmap.bmBitsPixel, NULL);
	if (!bDst) {
		pWin32Error(ERR, "CreateCompatibleBitmap() failed");
		goto err2;
	}

	hbmOld2 = SelectObject(dcDst, bDst);
	if (!hbmOld2 || hbmOld2 == HGDI_ERROR) {
		pWin32Error(ERR, "SelectObject() failed");
		goto err3;
	}

	if (!BitBlt(dcDst, 0, 0, bitmap.bmWidth, bitmap.bmHeight, dcSrc, 0, 0, SRCCOPY)) {
		pWin32Error(ERR, "BitBlt() failed");
		goto err3;
	}

	hbmOld = SelectObject(dcSrc, hbmOld);
	if (!hbmOld || hbmOld == HGDI_ERROR) {
		pWin32Error(ERR, "SelectObject() failed");
		goto err3;
	}

	hbmOld2 = SelectObject(dcSrc, hbmOld2);
	if (!hbmOld2 || hbmOld2 == HGDI_ERROR) {
		pWin32Error(ERR, "SelectObject() failed");
		goto err3;
	}
	
	rcBitmap = bDst;
	goto err2;
err3:
	if (!DeleteObject(bDst)) pWin32Error(ERR, "DeleteObject() failed");
err2:
	if (!DeleteDC(dcDst)) pWin32Error(ERR, "DeleteDC() failed");
err:
	if (!DeleteDC(dcSrc)) pWin32Error(ERR, "DeleteDC() failed");
	return rcBitmap;
}

static BOOL WINAPI freefunc_GlobalFree(HANDLE h)
{
	return GlobalFree(h) == NULL;
}

static int dupandreplace() {
	UINT fmtid;
	HANDLE hglbsrc;
	struct {
		HANDLE hglbl;
		UINT fmtid;
		BOOL (WINAPI *freefunc)(HANDLE h);
	} datas[MAX_FORMATS];
	char ignored[CF_MAX] = { 0 };
	#undef IGNORE
	#define IGNORE(fmt) ignored[fmt] = 1;

	int ndatas, i;

	LPVOID psrc, pdst;
	SIZE_T sz;

	/* duplicate clipboard contents */
	for (ndatas = 0, fmtid = 0;;) {
		fmtid = EnumClipboardFormats(fmtid);
		if (fmtid == 0) {
			if (GetLastError() != ERROR_SUCCESS) {
				pWin32Error(ERR, "EnumClipboardFormats() failed");
				goto err;
			}
			break;
		}
		if (fmtid == MY_CF) {
			printf("not duplicating my format\n");
			continue;
		}
		if (ndatas >= MAX_FORMATS) {
			printf("too many clipboard formats\n");
			goto err;
		}

		if (fmtid < MAX_FORMATS && ignored[fmtid])
		{
			printf("ignoring %d\n", (int)fmtid);
			continue;
		}

		hglbsrc = GetClipboardData(fmtid);
		printf("duplicating format %d, handle = %p\n", (int)fmtid, (void*)hglbsrc);
		if (!hglbsrc) {
			pWin32Error(ERR, "cf %d GetClipboardData() failed", (int)fmtid);
			goto err;
		}

		datas[ndatas].fmtid = fmtid;
		datas[ndatas].freefunc = freefunc_GlobalFree;

		switch(fmtid) {
			/* none */
			case CF_OWNERDISPLAY:
				printf("not duplicating CF_OWNERDISPLAY\n");
				continue;

			/* DeleteMetaFile */
			case CF_DSPENHMETAFILE:
			case CF_DSPMETAFILEPICT:
				printf("not duplicating CF_DSPENHMETAFILE or CF_DSPMETAFILEPICT\n");
				continue;
			case CF_ENHMETAFILE:
				IGNORE(CF_METAFILEPICT);
				printf("not duplicating CF_ENHMETAFILE\n");
				continue;
			case CF_METAFILEPICT:
				IGNORE(CF_ENHMETAFILE);
				printf("not duplicating CF_METAFILEPICT\n");
				continue;

			/* DeleteObject */
			case CF_PALETTE:
			case CF_DSPBITMAP:
				printf("not duplicating CF_METAFILEPICT or CF_DSPBITMAP\n");
				continue;
			case CF_BITMAP:
				IGNORE(CF_DIB);
				IGNORE(CF_DIBV5);
				datas[ndatas].freefunc = DeleteObject;
				datas[ndatas].hglbl = (HANDLE)dupbitmap((HBITMAP)hglbsrc);
				if (!datas[ndatas].hglbl) goto err;
				goto cont_ok;

			/* GlobalFree */
			case CF_DIB:
				IGNORE(CF_BITMAP);
				IGNORE(CF_PALETTE);
				IGNORE(CF_DIBV5);
				break;
			case CF_DIBV5:
				IGNORE(CF_BITMAP);
				IGNORE(CF_DIB);
				IGNORE(CF_PALETTE);
				break;
			case CF_OEMTEXT:
				IGNORE(CF_TEXT);
				IGNORE(CF_UNICODETEXT);
				break;
			case CF_TEXT:
				IGNORE(CF_OEMTEXT);
				IGNORE(CF_UNICODETEXT);
				break;
			case CF_UNICODETEXT:
				IGNORE(CF_OEMTEXT);
				IGNORE(CF_TEXT);
				break;
		}

		sz = GlobalSize(hglbsrc);
		if (sz == 0) {
			pWin32Error(ERR, "cf %d GlobalSize() failed", (int)fmtid);
			goto err;
		}
		datas[ndatas].hglbl = GlobalAlloc(GMEM_MOVEABLE, sz);
		if (datas[ndatas].hglbl == NULL) {
			pWin32Error(ERR, "GlobalAlloc() failed");
			goto err;
		}

		psrc = GlobalLock(hglbsrc);
		pdst = GlobalLock(datas[ndatas].hglbl);
		memcpy(pdst, psrc, sz);
		GlobalUnlock(datas[ndatas].hglbl);
		GlobalUnlock(hglbsrc);

cont_ok:
		ndatas++;

	}

	/* clear clipboard contents */
	printf("emptying clipboard\n");
	if (!EmptyClipboard()) {
		pWin32Error(ERR, "EmptyClipboard() failed");

	 err:
		for (i = 0; i < ndatas; i++) {
			datas[i].freefunc(datas[i].hglbl);
		}

		return -1;
	}

	/* replace clipboard contents */
	for (i = 0; i < ndatas; i++) {
		if (!SetClipboardData(datas[i].fmtid, datas[i].hglbl)) {
			pWin32Error(ERR, "SetClipboardData() failed");
			datas[i].freefunc(datas[i].hglbl);
		}
	}

	return 0;
}

int senddata(HGLOBAL hdata) {
	DWORD newnseq;
	int rc = -1;

	while (!OpenClipboard(ctx.hwnd)) {
		//pWin32Error("OpenClipboard() failed");
		Sleep(1);
	}
	newnseq = GetClipboardSequenceNumber();
	printf("before %d\n", newnseq);
	if (ctx.clipsrv_nseq != newnseq) {
		if (dupandreplace() < 0) {
			goto err;
		}
	}
	if (!SetClipboardData(MY_CF, hdata)) {
		pWin32Error(ERR, "SetClipboardData() failed");
		goto err;
	}

	rc = 0;
err:
	if (!CloseClipboard()) {
		pWin32Error(ERR, "CloseClipboard() failed");
	}
	ctx.clipsrv_nseq = GetClipboardSequenceNumber();
	printf("after %d\n", ctx.clipsrv_nseq);
	return rc;
}

/*

- if triggered by timer and no data
-   kill timer
-   wait for event
- fi

- if triggered by event and have data
-   set timer
-   wait for timer
- fi

*/
/*
VOID CALLBACK _clipserv_timerproc(HWND hwnd, UINT a, UINT_PTR b, DWORD c)
{
}

static int _clipserv_havedata_func(void *_param)
{
	return 0;
}
*/

void _clipsrv_reg_cnn(ClipConnection *conn)
{
	EnterCriticalSection(&ctx.lock);
	conn->tun->AddRef();
	ctx.connections.push_back(conn);
	LeaveCriticalSection(&ctx.lock);
}

void _clipsrv_havenewdata()
{
	SetEvent(ctx.havedata_ev);
}

struct ClipsrvConnectionFactory : ConnectionFactory {
	char clipname[40+1];
	void connect(Tunnel *tun) {
		ClipConnection *cnn = new ClipConnection(tun);
		cnn->local.nchannel = htonl(InterlockedIncrement(&ctx.nchannel));
		cnn->state = STATE_SYN;
		strcpy(cnn->remote.clipname, clipname);

		cnn->tun = tun;
		_clipsrv_reg_cnn(cnn);
		_clipsrv_havenewdata();
	}
};

ConnectionFactory *clipsrv_CreateConnectionFactory(const char clipname[40+1])
{
	ClipsrvConnectionFactory *connfact = new ClipsrvConnectionFactory();
	strcpy(connfact->clipname, clipname);
	return connfact;
}

void ClipConnection::bufferavail() {
	if (state == STATE_NEW_SRV) {
		prev_recv_pos--;
		state = STATE_EST;
		_clipsrv_havenewdata();
		return;
	}
	abort();
}

void ClipConnection::havedata() {
	abort();
}

ClipConnection::ClipConnection(Tunnel *tun) : Connection(tun), prev_recv_pos(0)
{
}

void clipsrvctx::newbuf()
{
	hglob = GlobalAlloc(GMEM_MOVEABLE, MAXPACKETSIZE);
	pbeg = (char*)GlobalLock(hglob);
	pend = pbeg + MAXPACKETSIZE;
	p = pbeg;

	memcpy(p, cliptun_data_header, sizeof(cliptun_data_header));
	p += sizeof(cliptun_data_header);

	memcpy(p, &localclipuuid.net, sizeof(localclipuuid.net));
	p += sizeof(localclipuuid.net);
}

void clipsrvctx::unlock_and_send_and_newbuf()
{
	LeaveCriticalSection(&lock);

	GlobalUnlock(hglob);
	hglob = GlobalReAlloc(hglob, p - pbeg, 0);
	senddata(hglob);
	Sleep(1000);

	newbuf();

}

void clipsrvctx::unlock_and_send_and_newbuf_and_lock()
{
	unlock_and_send_and_newbuf();
	wantmore = 1;
	EnterCriticalSection(&lock);
}

void clipsrvctx::bbb()
{
	newbuf();

	for(;;) {
		WaitForSingleObject(havedata_ev, INFINITE);

		EnterCriticalSection(&lock);

		do {
			wantmore = 0;
			for(unsigned u = 0; u < connections.size(); u++) {
				ClipConnection *conn = connections[u];
				cnnstate_t state = conn->state;
				switch (state) {
					case STATE_SYN:
						{
							/* send SYNs repeatedly, until we get ACK */
							/* TODO: add delay and max tries */
							size_t clipnamesize = strlen(conn->remote.clipname)+1;
							if (p + subpack_syn_size(clipnamesize) > pend) {
								unlock_and_send_and_newbuf_and_lock();
							}

							subpack_syn subpack;
							subpack.net_src_channel = conn->local.nchannel;
							subpack.net_packtype = htonl(PACK_SYN);

							memcpy(p, &subpack, subpack_syn_size(0));
							p += subpack_syn_size(0);

							strcpy(p, conn->remote.clipname);
							p += clipnamesize;

						}
						break;
					case STATE_NEW_SRV:
						break;
					case STATE_EST:
						{
							rfifo_long cur_pos = conn->pump_recv->buf.ofs_end;
							if (conn->prev_recv_pos != cur_pos) {
								/* if this ack is lost, they will just resend the data */

								subpack_ack subpack;

								if (p + sizeof(subpack) > pend) {
									unlock_and_send_and_newbuf_and_lock();
								}
								subpack.net_src_channel = conn->local.nchannel;
								subpack.net_packtype = htonl(PACK_ACK);
								subpack.dst = conn->remote.clipaddr;
								subpack.net_count = htonl(cur_pos - conn->prev_recv_pos);
								subpack.net_prev_pos = htonl(conn->prev_recv_pos);
								conn->prev_recv_pos = cur_pos;

								memcpy(p, &subpack, sizeof(subpack));
								p += sizeof(subpack);

							}
							rfifo_t *rfifo;
							rfifo = &conn->pump_send->buf;
							int datasz = rfifo_availread(rfifo);
							if (datasz != 0) {
								if (p + subpack_data_size(1) > pend) {
									unlock_and_send_and_newbuf_and_lock();
								}
								subpack_data subpack;

								int bufsz = (int)(pend - p - subpack_data_size(0));
								if (datasz > bufsz) datasz = bufsz;
								subpack.net_src_channel = conn->local.nchannel;
								subpack.net_packtype = htonl(PACK_ACK);
								subpack.dst = conn->remote.clipaddr;
								subpack.net_prev_pos = htonl(rfifo->ofs_mid);
								subpack.net_count = htonl(datasz);
								memcpy(p, &subpack, subpack_data_size(0));
								p += subpack_data_size(0);

								memcpy(p, rfifo_pdata(rfifo), datasz);
								p += datasz;
								rfifo_markread(rfifo, datasz);
							}
						}
					default:
						abort();
				}
			}
		} while (wantmore);
		if (p - pbeg != sizeof(cliptun_data_header) + sizeof(localclipuuid.net)) {
			unlock_and_send_and_newbuf();
		} else {
			LeaveCriticalSection(&lock);
		}
	}
}

static DWORD WINAPI _clipserv_send_thread(void *param)
{
	ctx.bbb();
	return 0;
}

static DWORD WINAPI _clipserv_wnd_thread(void *param)
{
	MSG msg;
	createutilitywindow(&ctx.hwnd); if (!ctx.hwnd) abort();
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

int clipsrv_init()
{
	DWORD tid;
	InitializeCriticalSection(&ctx.lock);

	UuidCreate(&ctx.localclipuuid._align);

	ctx.havedata_ev = CreateEvent(NULL, FALSE, FALSE, NULL);
	CreateThread(NULL, 0, _clipserv_wnd_thread, NULL, 0, &tid);
	CreateThread(NULL, 0, clipmon_wnd_thread, NULL, 0, &tid);
	CreateThread(NULL, 0, _clipserv_send_thread, NULL, 0, &tid);
	createutilitywindow(&ctx.hwnd);
	return 0;
}

/*
int clipsrv_connect(const char *clipname, HANDLE ev, clipaddr *remote)
{
	return 0;
}
*/
