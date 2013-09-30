#include "myclipserver.h"
#include "myeventloop.h"
#include "mytunnel.h"
#include "myutilitywindow.h"
#include "mylogging.h"
#include "rfifo.h"

#include <windows.h>
#include <rpc.h> /* for UuidCreate() */
#include <stdio.h>
#include <tchar.h>

#include <vector>
#include <map>

#include "mylastheader.h"

using namespace std;
using namespace cliptund;

#define MY_CF CF_RIFF
#define MAXPACKETSIZE 30000

#define CLIPTUN_DATA_HEADER "!cliptun!"

#define WM_HAVEDATA (WM_USER+0)
#define WM_ADVERTISE_PACKET (WM_USER+1)
#define WM_UNREGVIEWER (WM_USER+2)

#define OVERFLOWS(sz) ((sz) > (size_t)(pend - p))

typedef enum cnnstate_t {
	STATE_NEW_SRV, STATE_SYN, STATE_EST
} cnnstate_t;

typedef enum packtype_t {
	PACK_SYN, PACK_ACK, PACK_DATA, PACK_FIN
} packtype_t;

typedef struct net_uuid_t {
	unsigned char   __u_bits[16];
} net_uuid_t;

struct clipaddr {
	union {
		net_uuid_t addr;
		UUID _align;
	};
	char _hdr[4];
	u_long net_channel;
};

struct subpackheader_base {
	char _hdr[4];
	u_long net_src_channel;
	u_long net_packtype;
};

template<class _T> struct SubPackWrap : _T {
	SubPackWrap() {
		subpackheader_base &_this = *this;
		strncpy(_this._hdr, "pack", 4);
	}
};

struct subpack_syn : subpackheader_base {
	char dst_clipname[1];
};

#define subpack_syn_size(dst_clipname_size) (sizeof(subpackheader_base) + (dst_clipname_size))

struct subpackheader : subpackheader_base {
	clipaddr dst;
};

struct subpack_ack : subpackheader {
	u_long net_new_pos;
};

struct subpack_data : subpack_ack {
	u_long net_count;
};

#define subpack_data_size(data_size) (sizeof(subpack_data) + (data_size))

#define sizeofpacketheader (sizeof(cliptun_data_header) + sizeof(u_long) + sizeof(net_uuid_t))

struct ClipConnection : Connection {
	cnnstate_t state;
	long prev_recv_pos;

	ClipConnection(Tunnel *tun);

	void bufferavail();
	void havedata();

	struct {
		u_long net_channel;
		char clipname[40+1];
	} local;
	union {
		char clipname[40+1];
		clipaddr clipaddr;
	} remote;
};

struct Cliplistener {
	u_long net_channel;
	char clipname[40+1];
	ConnectionFactory *connfact;
	~Cliplistener() {
		delete connfact;
	}
};

struct _clipdata {
	HANDLE hglbl;
	BOOL (WINAPI *freefunc)(HANDLE h);
};
typedef map<UINT, _clipdata> savedclip_t;

struct clipsrvctx {
	SIZE_T lastgot, lastsent;
	int delay;
	int clip_opened;
	vector<Cliplistener*> listeners;

	int flag_sending;

	volatile LONG viewer_installed;
	HWND nextWnd;

	union {
		net_uuid_t net;
		UUID _align;
	} localclipuuid;
	volatile LONG nchannel;
	long npacket;
	HWND hwnd;
	CRITICAL_SECTION lock;
	std::vector<ClipConnection *> connections;


	HGLOBAL hglob;
	char *pbeg;
	char *p;
	char *pend;

	void newbuf();
	void fillpack();

	savedclip_t datas;
	int we_own_clip;
	void saveclip();
} ctx;

const char cliptun_data_header[] = CLIPTUN_DATA_HEADER;

void _clipsrv_parsepacket(const char *pend, const char *p);

void _clipsrv_reg_cnn(ClipConnection *conn);

static void _clipsrv_unreg_viewer()
{
	if (0 == InterlockedExchange(&ctx.viewer_installed, 0)) return;
	HWND hwnd = ctx.hwnd;
	if (hwnd) {
		log(INFO, "Removing self from chain: %p <- %p", (void*)hwnd, (void*)ctx.nextWnd);
		if (!ChangeClipboardChain(hwnd, ctx.nextWnd) && GetLastError() != 0) {
			pWin32Error(ERR, "ChangeClipboardChain() failed");
		}
	}
}

static
void warncorrupted() {
	log(WARN, "corrupted subpacket");
}

static
void ensure_openclip(HWND hwnd)
{
	int i;
	log(DBG, "ensure_openclip() begin");
	for (i = 0; !OpenClipboard(hwnd); i++) {
		if (i == 1000) {
			pWin32Error(WARN, "can't OpenClipboard for too long");
		}
		Sleep(1);
	}
	ctx.clip_opened = 1;
	log(DBG, "ensure_openclip() end");
}

static
void closeclip() {
	CloseClipboard();
	ctx.clip_opened = 0;
}

static bool get_clip_data_and_parse() {
	log(DBG, "get_clip_data_and_parse()");
	char buf[MAXPACKETSIZE];

	const char *pend, *p;

	int flag = 0;

	if (GetClipboardOwner() == ctx.hwnd) {
		return false;
	}

	ensure_openclip(ctx.hwnd);

	{
		UINT fmtid;
		char buf[100], *s = buf;
		buf[0] = '\0';
		for (fmtid = 0;;) {
			fmtid = EnumClipboardFormats(fmtid);
			if (fmtid == 0) break;
			s += sprintf(s, " %u", fmtid);
		}
		log(DBG, "formats:%s", buf);
	}

	SIZE_T sz;
	HGLOBAL hglob = GetClipboardData(MY_CF);
	if (hglob) {
		sz = GlobalSize(hglob);
		if (sz >= sizeofpacketheader) {
			p = (char*)GlobalLock(hglob);
			pend = p + sz;
			if (0 == memcmp(p, cliptun_data_header, sizeof(cliptun_data_header))) {
				p += sizeof(cliptun_data_header);

				if (pend - p > sizeof(buf)) {
					warncorrupted();
				} else {
					memcpy(buf, p, pend - p);
					pend = buf + (pend - p);
					p = buf;
					flag = 1;
				}
				//printf("received %d\n", (int)sz);
			} else {
				log(DBG, "header not matching");
			}
			GlobalUnlock(hglob);
		} else {
			log(DBG, "data size too small: %ld", (long)sz);
		}
	} else if (GetLastError() != ERROR_SUCCESS) {
		pWin32Error(INFO, "GetClipboardData() failed");
	}
	closeclip();
	if (flag) {
		clipaddr remote;
		memcpy(&remote.addr, p, sizeof(net_uuid_t));

		u_long ul;
		memcpy(&ul, p + sizeof(net_uuid_t), sizeof(u_long));
		long npacket = ntohl(ul);

		sz -= sizeofpacketheader;
		ctx.lastgot = sz;

		RPC_CSTR s;
		UuidToStringA(&remote._align, &s);
		log(DBG, "received packet: %s %ld; sz = %d", s, npacket, (int)sz);
		RpcStringFree(&s);

		EnterCriticalSection(&ctx.lock);
		_clipsrv_parsepacket(pend, p);
		LeaveCriticalSection(&ctx.lock);
	}
	return flag != 0;
}

void _clipsrv_parsepacket(const char *pend, const char *p)
{
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
				&& cnn->local.net_channel == header.dst.net_channel;
		}
		bool localandremoteequal(const ClipConnection *cnn) const {
			return localequal(cnn) && remoteequal(cnn);
		}
	} u;

	memcpy(&u.remote.addr, p, sizeof(net_uuid_t));
	p += sizeof(net_uuid_t);

	p += sizeof(u_long);

	for (; pend != p;) {
		ClipConnection *cnn;
		#define FIND_cnn(cond) do { for (vector<ClipConnection*>::iterator it = ctx.connections.begin();; it++) { \
				if (it == ctx.connections.end()) { cnn = NULL; break; } cnn = *it; if (cond) break; \
			} } while(0)

		#define GET_PARTIAL(already_have, fullsz) do { \
				if (OVERFLOWS((fullsz) - (already_have))) { \
					warncorrupted(); \
					return; \
				} \
				memcpy(&u.c + (already_have), p, ((fullsz) - (already_have))); \
				p += ((fullsz) - (already_have)); \
			} while(0)

		GET_PARTIAL(0, sizeof(subpackheader_base));

		packtype_t packtype = (packtype_t)ntohl(u.header.net_packtype);
		switch(packtype) {
			case PACK_SYN:
				FIND_cnn(u.remoteequal(cnn) && 0 == strncmp(p, cnn->local.clipname, pend - p));
				if (cnn) {
					cnn->prev_recv_pos--;
					cnn->havedata();
				} else {
					for (vector<Cliplistener*>::iterator it = ctx.listeners.begin(); it != ctx.listeners.end(); it++) {
						Cliplistener *cliplistener = *it;
						if (0 == strncmp(p, cliplistener->clipname, pend - p)) {
							log(INFO, "accepted clip %ld -> %s", ntohl(u.remote.net_channel), cliplistener->clipname);
							ClipConnection *cnn = new ClipConnection(NULL);
							Tunnel *tun = cnn->tun;
							strcpy(cnn->local.clipname, cliplistener->clipname);
							cnn->local.net_channel = cliplistener->net_channel;
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
				GET_PARTIAL(sizeof(subpackheader_base), sizeof(subpack_ack));

				for (vector<ClipConnection*>::iterator it = ctx.connections.begin(); it != ctx.connections.end(); it++) {
					cnn = *it;
					if (u.localequal(cnn))
					{
						if (cnn->state == STATE_SYN) {
							log(INFO, "connected clip %ld -> %s", ntohl(cnn->local.net_channel), cnn->remote.clipname);
							cnn->state = STATE_EST;
							cnn->remote.clipaddr = u.remote;
							cnn->tun->connected();
							break;
						} else if (u.remoteequal(cnn)) {
							rfifo_t *rfifo = &cnn->pump_send->buf;
							long new_pos = ntohl(u.data.net_new_pos);
							long ofs_beg = (u_long)rfifo->ofs_beg;
							long ofs_mid = (u_long)rfifo->ofs_mid;
							log(DBG, "got ack: ..%ld", new_pos);
							if (ofs_mid - new_pos >= 0 && new_pos - ofs_beg >= 0) {
								rfifo_confirmread(rfifo, new_pos - ofs_beg);
								cnn->pump_recv->bufferavail();
							}
							break;
						}
					}
				}
				break;
			case PACK_DATA:
				GET_PARTIAL(sizeof(subpackheader_base), subpack_data_size(0));

				long count;
				count = ntohl(u.data.net_count);
				if (OVERFLOWS((unsigned long)count)) {
					warncorrupted();
					return;
				}

				FIND_cnn(u.localandremoteequal(cnn));
				if (cnn)
				{
					rfifo_t *rfifo = &cnn->pump_recv->buf;
					long new_pos = ntohl(u.data.net_new_pos);
					long ofs_end = (u_long)rfifo->ofs_end;

					dumpdata(p, (int)count, "got data subpacket %d", (int)count);
					log(DBG, "got data: %ld..%ld (%ld)", new_pos - count, new_pos, count);
					if (new_pos - ofs_end >= 0) {
						if (cnn->prev_recv_pos - (new_pos - count) > 0) {
							cnn->prev_recv_pos = new_pos - count;
						}

						long skip = ofs_end - (new_pos - count);
						long datasz = new_pos - ofs_end;

						for(;;) {
							if (!datasz) break;
							long chunksz = rfifo_availwrite(rfifo);
							if (!chunksz) break;
							if (chunksz > datasz) chunksz = datasz;
							memcpy(rfifo_pfree(rfifo), p + skip, chunksz);
							rfifo_markwrite(rfifo, chunksz);
							skip += chunksz;
							datasz -= chunksz;
						}

						cnn->havedata();
						cnn->pump_send->havedata();
					}
				}
				p += count;
				break;
			case PACK_FIN:
				GET_PARTIAL(sizeof(subpackheader_base), sizeof(subpack_ack));

				FIND_cnn(u.localandremoteequal(cnn));
				if (cnn)
				{
					rfifo_t *rfifo = &cnn->pump_recv->buf;
					long new_pos = ntohl(u.data.net_new_pos);
					long ofs_end = (u_long)rfifo->ofs_end;
					if (ofs_end == new_pos) {
						if (!cnn->pump_recv->eof) {
							log(INFO, "disconnected clip %ld", ntohl(u.remote.net_channel));
							cnn->pump_recv->eof = 1;
							cnn->pump_send->havedata();
						}
					}
				}
				break;
			default:
				warncorrupted();
				return;
		}
	}
}

static
void _clipsrv_exitproc(void) {
	fprintf(stderr, "\n\n");
	log(INFO, "_clipsrv_exitproc()");
	_clipsrv_unreg_viewer();
}

static
BOOL WINAPI _clipsrv_ctrlhandler( DWORD fdwCtrlType )
{
	fprintf(stderr, "\n\n");
	log(INFO, "_clipsrv_ctrlhandler()");
	SendMessage(ctx.hwnd, WM_UNREGVIEWER, 0, 0);

	switch ( fdwCtrlType ) {
		case CTRL_C_EVENT:
			//fprintf(stderr, "\n\n Stopping due to Ctrl-C.\n" );
			break;

		case CTRL_CLOSE_EVENT:
			//fprintf(stderr, "\n\n Stopping due to closing the window.\n" );
			break;
		
		case CTRL_BREAK_EVENT:
			//fprintf(stderr, "\n\n Stopping due to Ctrl-Break.\n" );
			break;

		// Pass other signals to the next handler - ret = FALSE

		case CTRL_LOGOFF_EVENT:
			//fprintf(stderr, "\n\n Stopping due to logoff.\n" );
			break;

		case CTRL_SHUTDOWN_EVENT:
			//fprintf(stderr, "\n\n Stopping due to system shutdown.\n" );
			break;
		
		default:
			//fprintf(stderr, "\n\n Stopping due to unknown event.\n" );
			;
	}

	return FALSE;
}

int clipsrv_create_listener(ConnectionFactory *connfact, const char clipname[40+1])
{
	Cliplistener *listener = new Cliplistener();
	listener->net_channel = htonl(InterlockedIncrement(&ctx.nchannel));
	listener->connfact = connfact;
	strcpy(listener->clipname, clipname);
	ctx.listeners.push_back(listener);
	log(INFO, "listening on clip %s", clipname);
	return 0;
}

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

void _clipsrv_reg_cnn(ClipConnection *conn)
{
	conn->tun->AddRef();
	ctx.connections.push_back(conn);
}

void _clipsrv_unreg_cnn(ptrdiff_t n)
{
	log(DBG, "_clipsrv_unreg_cnn()");
	ClipConnection *cnn = ctx.connections[n];
	ctx.connections[n] = ctx.connections.back();
	ctx.connections.pop_back();
	cnn->tun->Release();
}


struct ClipsrvConnectionFactory : ConnectionFactory {
	char clipname[40+1];
	void connect(Tunnel *tun) {
		ClipConnection *cnn = new ClipConnection(tun);
		cnn->local.net_channel = htonl(InterlockedIncrement(&ctx.nchannel));
		cnn->state = STATE_SYN;
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
	}
	havedata();
}

void ClipConnection::havedata() {
	if (ctx.flag_sending) return;
	PostMessage(ctx.hwnd, WM_HAVEDATA, 0, 0);
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

void clipsrvctx::fillpack()
{
	log(DBG, "fillpack()");

	u_long ul = htonl(npacket);
	memcpy(p, &ul, sizeof(u_long));
	p += sizeof(u_long);

	for(unsigned u = 0; u < connections.size();) {
		ClipConnection *conn = connections[u];
		cnnstate_t state = conn->state;
		switch (state) {
			case STATE_SYN:
				{
					size_t clipnamesize = strlen(conn->remote.clipname)+1;

					if (OVERFLOWS(subpack_syn_size(clipnamesize))) {
						return;
					}

					SubPackWrap<subpack_syn> subpack;
					subpack.net_src_channel = conn->local.net_channel;
					subpack.net_packtype = htonl(PACK_SYN);

					memcpy(p, &subpack, subpack_syn_size(0));
					p += subpack_syn_size(0);

					strcpy(p, conn->remote.clipname);
					p += clipnamesize;
				}
				break;
			case STATE_EST:
				rfifo_t *rfifo;
				long cur_pos;
				int datasz, chunksz;

				cur_pos = conn->pump_recv->buf.ofs_end;
				if (conn->prev_recv_pos != cur_pos) {

					SubPackWrap<subpack_ack> subpack;

					if (OVERFLOWS(sizeof(subpack))) {
						return;
					}
					subpack.net_src_channel = conn->local.net_channel;
					subpack.net_packtype = htonl(PACK_ACK);
					subpack.dst = conn->remote.clipaddr;
					subpack.net_new_pos = htonl(cur_pos);
					conn->prev_recv_pos = cur_pos;

					memcpy(p, &subpack, sizeof(subpack));
					p += sizeof(subpack);

				}

				rfifo = &conn->pump_send->buf;

				chunksz = rfifo_availread(rfifo);
				if (chunksz != 0) {

					if (OVERFLOWS(subpack_data_size(1))) {
						return;
					}

					SubPackWrap<subpack_data> subpack;

					subpack.net_src_channel = conn->local.net_channel;
					subpack.net_packtype = htonl(PACK_DATA);
					subpack.dst = conn->remote.clipaddr;

					char *phdr = p;
					p += subpack_data_size(0);
					int bufsz = (int)(pend - p);

					datasz = 0;
					do {

						if (chunksz > bufsz) chunksz = bufsz;

						memcpy(p, rfifo_pdata(rfifo), chunksz);
						dumpdata(p, (int)chunksz, "reading %d at %llx(%lld) from %p: ", (int)chunksz, (long long)rfifo->ofs_mid, (long long)rfifo->ofs_mid, rfifo);
						p += chunksz;
						rfifo_markread(rfifo, chunksz);
						datasz += chunksz;
						bufsz -= chunksz;

						chunksz = rfifo_availread(rfifo);
					} while (chunksz && bufsz);
					dumpdata(p - datasz, (int)datasz, "send data subpacket %d at %llx(%lld)", (int)datasz, (long long)(p - datasz), (long long)(p - datasz));

					subpack.net_new_pos = htonl(rfifo->ofs_mid);
					subpack.net_count = htonl(datasz);
					memcpy(phdr, &subpack, subpack_data_size(0));

					//log(DBG, "Sending DATA %ld..%ld (%ld bytes)", ntohl(subpack.net_prev_pos), ntohl(subpack.net_prev_pos) + ntohl(subpack.net_count), ntohl(subpack.net_count));

				}

				if (conn->pump_send->eof) {
					if (rfifo->ofs_end == rfifo->ofs_beg) {
						/* all sent data is confirmed */
					}

					if (OVERFLOWS(sizeof(subpack_ack))) {
						return;
					}

					SubPackWrap<subpack_ack> subpack;

					subpack.net_src_channel = conn->local.net_channel;
					subpack.net_packtype = htonl(PACK_FIN);
					subpack.dst = conn->remote.clipaddr;

					subpack.net_new_pos = htonl(rfifo->ofs_end);

					memcpy(p, &subpack, sizeof(subpack));
					p += sizeof(subpack);

					if (rfifo->ofs_end == rfifo->ofs_beg) {
						_clipsrv_unreg_cnn(u);
						continue;
					}
				}
				break;
			case STATE_NEW_SRV:
				break;
			default:
				abort();
		}
		u++;
	}
}

static
VOID CALLBACK sleep_timeout(HWND _hwnd_null, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	KillTimer(_hwnd_null, idEvent);
	PostMessage(ctx.hwnd, WM_ADVERTISE_PACKET, 0, 0);
}

static
LRESULT CALLBACK _clipsrv_wndproc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	UINT fmtid;
	switch (uMsg) {
		case WM_HAVEDATA:
			log(DBG, "WM_HAVEDATA");
			if (ctx.flag_sending) break;
			ctx.flag_sending = 1;
			/* fallthrough */
		case WM_ADVERTISE_PACKET:
			log(DBG, "WM_ADVERTISE_PACKET");
			ctx.npacket++;
			log(DBG, "advertise_packet(); npacket = %d", ctx.npacket);
			ensure_openclip(ctx.hwnd);
			if (!ctx.we_own_clip) {
				ctx.saveclip();
			}
			if (EmptyClipboard()) {
				SetClipboardData(MY_CF, NULL);

				for (savedclip_t::iterator it = ctx.datas.begin(); it != ctx.datas.end(); it++) {
					fmtid = it->first;
					SetClipboardData(fmtid, NULL);
				}

				ctx.we_own_clip = 1;
			}
			closeclip();
			break;
		case WM_RENDERFORMAT:
			fmtid = wParam;
			log(DBG, "WM_RENDERFORMAT %u", fmtid);
			if (fmtid == MY_CF) {
				EnterCriticalSection(&ctx.lock);
				ctx.fillpack();
				LeaveCriticalSection(&ctx.lock);
				GlobalUnlock(ctx.hglob);
				SIZE_T newsz;
				newsz = ctx.p - ctx.pbeg;
				ctx.hglob = GlobalReAlloc(ctx.hglob, newsz, 0);
				newsz -= sizeofpacketheader;
				ctx.lastsent = newsz;
				SetClipboardData(MY_CF, ctx.hglob);
				{
					RPC_CSTR s;
					UuidToStringA(&ctx.localclipuuid._align, &s);
					log(DBG, "sending packet %s %ld; sz = %d", s, ctx.npacket, (int)newsz);
					RpcStringFree(&s);
				}
				ctx.newbuf();
			} else {
				savedclip_t::iterator it = ctx.datas.find(fmtid);
				SetClipboardData(fmtid, it->second.hglbl);
				ctx.datas.erase(it);
			}
			break;
		case WM_DRAWCLIPBOARD:
			log(DBG, "WM_DRAWCLIPBOARD");

			if (!ctx.clip_opened) {
				if (get_clip_data_and_parse()) {
					ctx.flag_sending = 1;
					int delay = ctx.delay;
					if (ctx.lastgot + ctx.lastsent == 0) {
						delay = delay + 300;
					}
					log(DBG, "sleeping %d ms", delay);
					SetTimer(NULL, 0, delay, sleep_timeout);
				} else {
					ctx.we_own_clip = 0;
				}
			}

			/* SetClipboardViewer() sends WM_DRAWCLIPBOARD and fails with our last error */
			SetLastError(ERROR_SUCCESS);

			if (ctx.nextWnd) {
				return SendMessage(ctx.nextWnd,uMsg,wParam,lParam);
			}
			break;
		case WM_CHANGECBCHAIN:
			{
				HWND wndRemove = (HWND)wParam;
				HWND wndNextNext = (HWND)lParam;
				log(INFO, "WM_CHANGECBCHAIN %p %p", (void*)wndRemove, (void*)wndNextNext);
				if (ctx.nextWnd == wndRemove) {
					log(INFO, "saving next window %p", (void*)wndNextNext);
					ctx.nextWnd = wndNextNext;
				} else if (ctx.nextWnd) {
					log(INFO, "notifying next window %p", (void*)ctx.nextWnd);
					return SendMessage(ctx.nextWnd,uMsg,wParam,lParam);
				} else {
					log(INFO, "not notifying next window %p", (void*)ctx.nextWnd);
				}
			}
			break;
		case WM_UNREGVIEWER:
			log(INFO, "WM_UNREGVIEWER");
			_clipsrv_unreg_viewer();
			break;
		default:
			return DefWindowProc( hwnd,uMsg,wParam,lParam);
	}
	return 0;
}

static DWORD WINAPI _clipserv_wnd_thread(void *param)
{
	HANDLE ev_inited = (HANDLE)param;
	MSG msg;
	createutilitywindowwithproc(&ctx.hwnd, _clipsrv_wndproc, _T("myclipsrv"));
	if (!ctx.hwnd) abort();

	atexit(_clipsrv_exitproc);
	SetConsoleCtrlHandler(_clipsrv_ctrlhandler, TRUE);

	/* Not processing WM_DRAWCLIPBOARD somehow prevents SetClipboardViewer() from failure */
	ctx.clip_opened = 1;
	ctx.nextWnd = SetClipboardViewer(ctx.hwnd);
	ctx.viewer_installed = 1;
	ctx.clip_opened = 0;

	SetEvent(ev_inited);
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

void clipsrv_init()
{
	DWORD tid;

	InitializeCriticalSection(&ctx.lock);

	const char *s = getenv("CLIPTUN_DELAY");
	if (s && s[0]) {
		ctx.delay = atoi(s);
	} else {
		ctx.delay = 10;
	}
	log(INFO, "delay: %d ms", ctx.delay);
	UuidCreate(&ctx.localclipuuid._align);
	for (int i = 0; i < sizeof(ctx.localclipuuid.net.__u_bits); i++) {
		ctx.nchannel = ctx.nchannel ^ (ctx.nchannel << 8) ^ ctx.localclipuuid.net.__u_bits[i];
	}

	ctx.newbuf();

	HANDLE ev_inited = CreateEvent(NULL, FALSE, FALSE, NULL);

	HANDLE hTread = CreateThread(NULL, 0, _clipserv_wnd_thread, ev_inited, 0, &tid);
	CloseHandle(hTread);

	WaitForSingleObject(ev_inited, INFINITE);
	CloseHandle(ev_inited);
}

/*
template<typename T>
typename T::iterator
insertNoCheck(T &map, const typename T::key_type &key ) {
  return map.insert(map.begin(), typename T::value_type(key, typename T::mapped_type()));
}
*/

void clipsrvctx::saveclip() {

	for (savedclip_t::iterator it = datas.begin(); it != datas.end(); it++) {
		_clipdata &data = it->second;
		data.freefunc(data.hglbl);
	}
	datas.clear();

	UINT fmtid;
	HANDLE hglbsrc;

	char ignored[CF_MAX] = { 0 };
	#undef IGNORE
	#define IGNORE(fmt) ignored[fmt] = 1;

	LPVOID psrc, pdst;
	SIZE_T sz;

	/* duplicate clipboard contents */
	for (fmtid = 0;;) {
		fmtid = EnumClipboardFormats(fmtid);
		if (fmtid == 0) break;
		if (fmtid == MY_CF) {
			//printf("not duplicating my format\n");
			continue;
		}

		if (ignored[fmtid])
		{
			//printf("ignoring %d\n", (int)fmtid);
			continue;
		}

		hglbsrc = GetClipboardData(fmtid);
		//printf("duplicating format %d, handle = %p\n", (int)fmtid, (void*)hglbsrc);
		if (!hglbsrc && GetLastError() != ERROR_SUCCESS) {
			pWin32Error(WARN, "cf %u GetClipboardData() failed", fmtid);
			continue;
		}

		_clipdata data;
		data.hglbl = hglbsrc;
		if (hglbsrc) {
			data.freefunc = freefunc_GlobalFree;

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
					data.freefunc = DeleteObject;
					data.hglbl = (HANDLE)dupbitmap((HBITMAP)hglbsrc);
					if (!data.hglbl) continue;
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
				pWin32Error(WARN, "cf %d GlobalSize() failed", (int)fmtid);
				continue;
			}
			data.hglbl = GlobalAlloc(GMEM_MOVEABLE, sz);
			psrc = GlobalLock(hglbsrc);
			pdst = GlobalLock(data.hglbl);
			memcpy(pdst, psrc, sz);
			GlobalUnlock(data.hglbl);
			GlobalUnlock(hglbsrc);
		}

cont_ok:
		datas[fmtid] = data;

	}
}

#if 0
int foo() {
	int ndatas = 4;
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
#endif
