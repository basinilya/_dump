#define CONSTRHDR

#include "myclipserver.h"
#include "myclipinternal.h"

#include "myeventloop.h"
#include "mylogging.h"

#include <windows.h>
#include <rpc.h>
#include <stdio.h>
#include <tchar.h>

#include "mylastheader.h"

using namespace std;

#define MAX_FORMATS 100

#define TIMEOUT 40
#define NUMRETRIES_SYN 40
#define NUMRETRIES_FIN 4
#define NUMRETRIES_DATA_BEFORE_RESEND 6
#define NUMRETRIES_DATA_BEFORE_GIVEUP 40

const char cliptun_data_header[] = CLIPTUN_DATA_HEADER;

clipsrvctx ctx = {};


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

static int dupandreplace1() {
	//log(INFO, "dupandreplace");
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
			//printf("not duplicating my format\n");
			continue;
		}
		if (ndatas >= MAX_FORMATS) {
			//printf("too many clipboard formats\n");
			goto err;
		}

		if (fmtid < MAX_FORMATS && ignored[fmtid])
		{
			//printf("ignoring %d\n", (int)fmtid);
			continue;
		}

		hglbsrc = GetClipboardData(fmtid);
		//printf("duplicating format %d, handle = %p\n", (int)fmtid, (void*)hglbsrc);
		if (!hglbsrc && GetLastError() != ERROR_SUCCESS) {
			pWin32Error(ERR, "cf %d GetClipboardData() failed", (int)fmtid);
			goto err;
		}

		datas[ndatas].fmtid = fmtid;
		datas[ndatas].hglbl = hglbsrc;
		if (hglbsrc) {
			datas[ndatas].freefunc = freefunc_GlobalFree;

			switch(fmtid) {
				/* none */
				case CF_OWNERDISPLAY:
					//printf("not duplicating CF_OWNERDISPLAY\n");
					continue;

				/* DeleteMetaFile */
				case CF_DSPENHMETAFILE:
				case CF_DSPMETAFILEPICT:
					//printf("not duplicating CF_DSPENHMETAFILE or CF_DSPMETAFILEPICT\n");
					continue;
				case CF_ENHMETAFILE:
					IGNORE(CF_METAFILEPICT);
					//printf("not duplicating CF_ENHMETAFILE\n");
					continue;
				case CF_METAFILEPICT:
					IGNORE(CF_ENHMETAFILE);
					//printf("not duplicating CF_METAFILEPICT\n");
					continue;

				/* DeleteObject */
				case CF_PALETTE:
				case CF_DSPBITMAP:
					//printf("not duplicating CF_METAFILEPICT or CF_DSPBITMAP\n");
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
		}

cont_ok:
		ndatas++;

	}

	/* clear clipboard contents */
	//printf("emptying clipboard\n");
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
		if (datas[i].hglbl) {
			if (!SetClipboardData(datas[i].fmtid, datas[i].hglbl)) {
				pWin32Error(ERR, "SetClipboardData() failed");
				datas[i].freefunc(datas[i].hglbl);
			}
		}
	}

	return 0;
}

volatile int breakflag;

static DWORD WINAPI breakthread(LPVOID p) {
	Sleep(100);
	if (breakflag) {
		int i = 0;
		//DebugBreak();
	}
	return 0;
}

static int dupandreplace() {
	int i, mks;
	FILETIME before, after;
	GetSystemTimeAsFileTime(&before);

	DWORD tid;
	breakflag = 1;
	//HANDLE hthr = CreateThread(NULL, 0, breakthread, 0, 0, &tid);

	i = dupandreplace1();

	breakflag = 0;

	GetSystemTimeAsFileTime(&after);

	mks =	(int)((
		((((long long)after.dwHighDateTime) << 32) + after.dwLowDateTime)
		- ((((long long)before.dwHighDateTime) << 32) + before.dwLowDateTime)
	) / 10)
	;
	//log(INFO, "dupandreplace: %d mks", mks);

	return i;
}

void _clipsrv_OpenClipboard(HWND hwnd)
{
	int i;
	log(INFO, "_clipsrv_OpenClipboard() begin");
	for (i = 0; !OpenClipboard(hwnd); i++) {
		if (i == 1000) {
			pWin32Error(WARN, "can't OpenClipboard for too long");
		}
		Sleep(1);
	}
	log(INFO, "_clipsrv_OpenClipboard() end");
}

int senddata(HGLOBAL hdata) {
	DWORD newnseq;
	int rc = -1;

	_clipsrv_OpenClipboard(ctx.hwnd);
	newnseq = GetClipboardSequenceNumber();
	//log(INFO, "ctx.clipsrv_nseq = %u, newnseq = %u", ctx.clipsrv_nseq, newnseq);
	//printf("before %d\n", newnseq);

/*
	if (ctx.clipsrv_nseq != newnseq) {
		if (dupandreplace() < 0) {
			goto err;
		}
	}
*/

	EmptyClipboard();
	if (!hdata) {
		log(ERR, "hdata is NULL");
		abort();
	}
	if (!SetClipboardData(MY_CF, hdata)) {
		pWin32Error(ERR, "SetClipboardData() failed");
		goto err;
	}

	rc = 0;
err:
	CloseClipboard();
	ctx.clipsrv_nseq = GetClipboardSequenceNumber();
	//printf("after %d\n", ctx.clipsrv_nseq);
	return rc;
}

void _clipsrv_reg_cnn(ClipConnection *conn)
{
	conn->tun->AddRef();
	ctx.connections.push_back(conn);
}

void _clipsrv_unreg_cnn(ptrdiff_t n)
{
	ClipConnection *cnn = ctx.connections[n];
	ctx.connections[n] = ctx.connections.back();
	ctx.connections.pop_back();
	cnn->tun->Release();
}


struct ClipsrvConnectionFactory : ConnectionFactory {
	char clipname[40+1];
	void connect(Tunnel *tun) {
		ClipConnection *cnn = new ClipConnection(tun);
		cnn->local.nchannel = htonl(InterlockedIncrement(&ctx.nchannel));
		cnn->state = STATE_SYN;
		cnn->resend_counter = 0;
		strcpy(cnn->remote.clipname, clipname);

		cnn->tun = tun;
		EnterCriticalSection(&ctx.lock);
		_clipsrv_reg_cnn(cnn);
		LeaveCriticalSection(&ctx.lock);
		cnn->havedata();
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
		resend_counter = 0;
	}
	havedata();
}

void ClipConnection::havedata() {
	SetEvent(ctx.havedata_ev);
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

	u_long ul = htonl(ctx.npacket);
	memcpy(p, &ul, sizeof(u_long));
	p += sizeof(u_long);

	memcpy(p, &localclipuuid.net, sizeof(localclipuuid.net));
	p += sizeof(localclipuuid.net);
}

void clipsrvctx::unlock_and_send_and_newbuf()
{
	_clipsrv_parsepacket(p, pbeg + sizeof(cliptun_data_header) + sizeof(u_long));
	LeaveCriticalSection(&lock);

	GlobalUnlock(hglob);
	hglob = GlobalReAlloc(hglob, p - pbeg, 0);

	{
		RPC_CSTR s;
		UuidToStringA(&ctx.localclipuuid._align, &s);
		log(INFO, "sending packet %s %ld", s, ctx.npacket);
		RpcStringFree(&s);
	}

	ctx.npacket++;
	senddata(hglob);
	Sleep(TIMEOUT);

	newbuf();

}

void clipsrvctx::unlock_and_send_and_newbuf_and_lock()
{
	unlock_and_send_and_newbuf();
	wantmore = 1;
	EnterCriticalSection(&lock);
}

void clipsrvctx::mainloop()
{
	newbuf();

	for(;;) {
		WaitForSingleObject(havedata_ev, 1000);

		EnterCriticalSection(&lock);

		do {
			wantmore = 0;
			for(unsigned u = 0; u < connections.size(); u++) {
				ClipConnection *conn = connections[u];
				cnnstate_t state = conn->state;
				switch (state) {
					case STATE_SYN:
						{
							if (conn->resend_counter == NUMRETRIES_SYN) {
								_clipsrv_unreg_cnn(u);
								u--;
								break;
							}
							if (conn->resend_counter == NUMRETRIES_SYN/2) {
								ctx.clipsrv_nseq = 0;
							}
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

							//log(INFO, "Sending SYN #%d", conn->resend_counter);

							conn->resend_counter++;
							//log(INFO, "incremented resend_counter: %d", conn->resend_counter);

						}
						break;
					case STATE_EST:
						{
							long cur_pos = conn->pump_recv->buf.ofs_end;
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

							if (conn->resend_counter == NUMRETRIES_DATA_BEFORE_GIVEUP) {
								_clipsrv_unreg_cnn(u);
								u--;
								break;
							}

							rfifo_t *rfifo;
							rfifo = &conn->pump_send->buf;
							if (rfifo->ofs_mid != rfifo->ofs_beg) {
								int counter = conn->resend_counter;
								int remainder = counter % (NUMRETRIES_DATA_BEFORE_RESEND+1);
								if (remainder == NUMRETRIES_DATA_BEFORE_RESEND/2) {
									ctx.clipsrv_nseq = 0;
								}
								if (remainder == NUMRETRIES_DATA_BEFORE_RESEND) {
									log(INFO, "packet lost: counter = %d, remainder = %d", counter, remainder);
									rfifo->ofs_mid = rfifo->ofs_beg;
								}
								conn->resend_counter++;
								//log(INFO, "incremented resend_counter: %d", conn->resend_counter);
							}
							int datasz = rfifo_availread(rfifo);
							if (datasz != 0) {
								if (p + subpack_data_size(1) > pend) {
									unlock_and_send_and_newbuf_and_lock();
								}
								subpack_data subpack;

								int bufsz = (int)(pend - p - subpack_data_size(0));
								if (datasz > bufsz) datasz = bufsz;
								subpack.net_src_channel = conn->local.nchannel;
								subpack.net_packtype = htonl(PACK_DATA);
								subpack.dst = conn->remote.clipaddr;
								subpack.net_prev_pos = htonl(rfifo->ofs_mid);
								subpack.net_count = htonl(datasz);
								memcpy(p, &subpack, subpack_data_size(0));
								p += subpack_data_size(0);

								memcpy(p, rfifo_pdata(rfifo), datasz);
								p += datasz;
								rfifo_markread(rfifo, datasz);

								//log(INFO, "Sending DATA %ld..%ld (%ld bytes)", ntohl(subpack.net_prev_pos), ntohl(subpack.net_prev_pos) + ntohl(subpack.net_count), ntohl(subpack.net_count));

							}
						}
						if (conn->pump_send->eof) {

							rfifo_t *rfifo;
							rfifo = &conn->pump_send->buf;

							if (rfifo->ofs_end == rfifo->ofs_mid) {
								/* all sent data is confirmed */
								if (conn->resend_counter == NUMRETRIES_FIN) {
									_clipsrv_unreg_cnn(u);
									u--;
									break;
								}
								//log(INFO, "Sending FIN #%d", conn->resend_counter);
								conn->resend_counter++;
								//log(INFO, "incremented resend_counter: %d", conn->resend_counter);
							} else {
								//log(INFO, "Sending FIN #-");
							}

							if (p + sizeof(subpack_ack) > pend) {
								unlock_and_send_and_newbuf_and_lock();
							}
							subpack_ack subpack;

							subpack.net_src_channel = conn->local.nchannel;
							subpack.net_packtype = htonl(PACK_FIN);
							subpack.dst = conn->remote.clipaddr;

							subpack.net_prev_pos = htonl(rfifo->ofs_end);
							subpack.net_count = htonl(0);

							memcpy(p, &subpack, sizeof(subpack));
							p += sizeof(subpack);
						}
						break;
					case STATE_NEW_SRV:
						break;
					default:
						abort();
				}
			}
		} while (wantmore);
		if (p - pbeg != sizeofpacketheader) {
			unlock_and_send_and_newbuf();
		} else {
			LeaveCriticalSection(&lock);
		}
	}
}

static DWORD WINAPI _clipserv_send_thread(void *param)
{
	ctx.mainloop();
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
	for (int i = 0; i < sizeof(ctx.localclipuuid.net.__u_bits); i++) {
		ctx.nchannel = ctx.nchannel ^ (ctx.nchannel << 8) ^ ctx.localclipuuid.net.__u_bits[i];
	}

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
