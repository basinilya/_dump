#ifndef _MYCLIPINTERNAL
#define _MYCLIPINTERNAL

#include "mytunnel.h"

#include <rpc.h>

using namespace cliptund;

#define MY_CF CF_RIFF
#define MAXPACKETSIZE 4096

#ifdef __cplusplus
extern "C" {
#endif

#define CLIPTUN_DATA_HEADER "!cliptun!"
extern const char cliptun_data_header[sizeof(CLIPTUN_DATA_HEADER)];

typedef enum cnnstate_t {
	STATE_NEW_SRV, STATE_SYN, STATE_EST
} cnnstate_t;

typedef enum packtype_t {
	PACK_SYN, PACK_ACK, PACK_DATA, PACK_FIN
} packtype_t;

typedef struct net_uuid_t {
	unsigned char   __u_bits[16];
} net_uuid_t;


typedef struct clipaddr {
	net_uuid_t addr;
	char _hdr[4];
	u_long nchannel;
} clipaddr;

DWORD WINAPI clipmon_wnd_thread(void *param);

HWND _createutilitywindow(WNDCLASS *wndclass);
#define createutilitywindowwithproc(phwnd, lpfnWndProc, lpszClassName) do {   \
	static WNDCLASS wndclass = {                                 \
		0,            /*    UINT        style;*/                 \
		(lpfnWndProc),/*    WNDPROC     lpfnWndProc;*/           \
		0,            /*    int         cbClsExtra;*/            \
		0,            /*    int         cbWndExtra;*/            \
		NULL,         /*    HINSTANCE   hInstance;*/             \
		NULL,         /*    HICON       hIcon;*/                 \
		NULL,         /*    HCURSOR     hCursor;*/               \
		NULL,         /*    HBRUSH      hbrBackground;*/         \
		NULL,         /*    LPCWSTR     lpszMenuName;*/          \
		(lpszClassName) /*    LPCWSTR     lpszClassName;*/       \
	};                                                           \
	*(phwnd) = _createutilitywindow(&wndclass);                  \
} while(0)

#define createutilitywindow(phwnd) createutilitywindowwithproc(phwnd, DefWindowProc, _T("mydefproc"))

#ifdef __cplusplus
}
#endif

struct ClipConnection : Connection {
	cnnstate_t state;
	rfifo_long prev_recv_pos;
	ClipConnection(Tunnel *tun);

	void bufferavail();
	void havedata();

	struct {
		u_long nchannel;
		char clipname[40+1];
	} local;
	union {
		char clipname[40+1];
		clipaddr clipaddr;
	} remote;
};

void _clipsrv_reg_cnn(ClipConnection *conn);

#include <vector>

extern struct clipsrvctx {
	DWORD clipsrv_nseq;
	union {
		net_uuid_t net;
		UUID _align;
	} localclipuuid;
	volatile LONG nchannel;
	HWND hwnd;
	HANDLE havedata_ev;
	int havedata;
	CRITICAL_SECTION lock;
	std::vector<ClipConnection *> connections;


	HGLOBAL hglob;
	char *pbeg;
	char *p;
	char *pend;
	int wantmore;

	void newbuf();
	void unlock_and_send_and_newbuf();
	void unlock_and_send_and_newbuf_and_lock();
	void mainloop();
} ctx;

struct subpackheader_base {
	char _hdr[4];
	u_long net_src_channel;
	u_long net_packtype;
#ifdef CONSTRHDR
	subpackheader_base() {
		strncpy(_hdr, "pack", 4);
	}
#endif
};

struct subpack_syn : subpackheader_base {
	char dst_clipname[1];
};

#define subpack_syn_size(dst_clipname_size) (sizeof(subpackheader_base) + (dst_clipname_size))

struct subpackheader : subpackheader_base {
	clipaddr dst;
};

struct subpack_ack : subpackheader {
	u_long net_prev_pos;
	u_long net_count;
};

struct subpack_data : subpack_ack {
	char data[1];
};

#define subpack_data_size(data_size) (sizeof(subpack_ack) + (data_size))

#endif
