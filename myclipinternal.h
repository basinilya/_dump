#ifndef _MYCLIPINTERNAL
#define _MYCLIPINTERNAL

#include "mytunnel.h"

#include <rpc.h>

using namespace cliptund;



#ifdef __cplusplus
extern "C" {
#endif


#define MY_CF CF_RIFF

extern DWORD clipsrv_nseq;

#define CLIPTUN_DATA_HEADER "!cliptun!"
extern const char cliptun_data_header[sizeof(CLIPTUN_DATA_HEADER)];

typedef enum cnnstate {
	STATE_SYN
} cnnstate;

typedef struct net_uuid_t {
	unsigned char   __u_bits[16];
} net_uuid_t;


/*
typedef struct clipaddr {
	net_uuid_t addr;
	u_long nchannel;
} clipaddr;
*/

int clipsrv_init();
int clipsrv_havenewdata();
int clipsrv_create_listener(const char clipname[40+1], const char host[40+1], short port);


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

#endif
