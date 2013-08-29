#include "myclipserver.h"
#include "myeventloop.h"
#include "mylogging.h"

#include <windows.h>
#include <rpc.h>
#include <stdio.h>
#include <tchar.h>

#include <vector>
using namespace std;

#include "mylastheader.h"

#define MY_CF CF_RIFF
#define MAX_FORMATS 100


static struct {
	UUID localclipuuid;
	int nchannel;
	HWND hwnd;
	HANDLE havedata_ev;
	int havedata;
	UINT_PTR nTimer;
	CRITICAL_SECTION lock;
	vector<clip_connection*> connections;
} ctx;


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
	static DWORD nseq = 0;
	DWORD newnseq;
	int rc = -1;

	while (!OpenClipboard(ctx.hwnd)) {
		//pWin32Error("OpenClipboard() failed");
		Sleep(1);
	}
	newnseq = GetClipboardSequenceNumber();
	printf("before %d\n", newnseq);
	if (nseq != newnseq) {
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
	nseq = GetClipboardSequenceNumber();
	printf("after %d\n", nseq);
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

int clipsrv_reg_cnn(clip_connection *conn)
{
	EnterCriticalSection(&ctx.lock);
	ctx.connections.push_back(conn);
	LeaveCriticalSection(&ctx.lock);
	return 0;
}

static DWORD WINAPI _clipserv_send_thread(void *param)
{
	for(;;) {
		WaitForSingleObject(ctx.havedata_ev, INFINITE);
		HGLOBAL hglob = GlobalAlloc(GMEM_MOVEABLE, 4096);
		char *pbeg = (char*)GlobalLock(hglob);
		char *pend = pbeg + 4096;
		char *p = pbeg;
		EnterCriticalSection(&ctx.lock);
		for(vector<clip_connection*>::iterator it = ctx.connections.begin(); it != ctx.connections.end(); it++) {
			clip_connection *conn = *it;
			cnnstate state = conn->state;
			switch (state) {
				case STATE_SYN:
					u_long netstate;
					size_t clipnamesize = strlen(conn->a.clipname)+1;
					SSIZE_T sz = sizeof(netstate) + clipnamesize;
					if (pend - p < sz) goto break_loop;
					netstate = htonl(state);
					memcpy(p, &netstate, sizeof(netstate));
					p += sizeof(netstate);
					strcpy(p, conn->a.clipname);
					p += clipnamesize;
					//aaa;
			}
		}
		break_loop:
		LeaveCriticalSection(&ctx.lock);
		GlobalUnlock(hglob);
		hglob = GlobalReAlloc(hglob, p - pbeg, 0);
		senddata(hglob);
		Sleep(1000);
	}
	return 0;
}

static DWORD WINAPI _clipserv_wnd_thread(void *param)
{
	MSG msg;
	createutilitywindow(&ctx.hwnd, DefWindowProc, _T("myclipsender")); if (!ctx.hwnd) abort();
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
	UuidCreate(&ctx.localclipuuid);
	ctx.havedata_ev = CreateEvent(NULL, FALSE, FALSE, NULL);
	CreateThread(NULL, 0, _clipserv_wnd_thread, NULL, 0, &tid);
	CreateThread(NULL, 0, clipmon_wnd_thread, NULL, 0, &tid);
	CreateThread(NULL, 0, _clipserv_send_thread, NULL, 0, &tid);
	createutilitywindow(&ctx.hwnd, DefWindowProc, _T("myclipsender"));
	return 0;
}



int clipsrv_havenewdata()
{
	SetEvent(ctx.havedata_ev);
	return 0;
}
/*
int clipsrv_connect(const char *clipname, HANDLE ev, clipaddr *remote)
{
	return 0;
}
*/
