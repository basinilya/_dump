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

extern DWORD clipsrv_nseq;

#define CLIPTUN_DATA_HEADER "!cliptun!"
extern const char cliptun_data_header[sizeof(CLIPTUN_DATA_HEADER)];

typedef enum cnnstate {
	STATE_NEW_CL, STATE_NEW_SRV, STATE_SYN, STATE_FIRST_ACK, STATE_EST
} cnnstate;

typedef struct net_uuid_t {
	unsigned char   __u_bits[16];
} net_uuid_t;


typedef struct clipaddr {
	net_uuid_t addr;
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
	ClipConnection(Tunnel *tun);

	void bufferavail();
	void havedata();

	cnnstate state;
	u_long local_nchannel;
	union {
		char clipname[40+1];
		clipaddr clipaddr;
	} remote;
};

void _clipsrv_reg_cnn(ClipConnection *conn);

#endif
