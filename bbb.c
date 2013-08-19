
#include <stdio.h>
#include <time.h>
#include <windows.h>
#include <tchar.h>
#include <stdarg.h>

//#define CLIP_FMT_NAME "XenAppTraversalFormat"
//#define CLIP_TEXTDATA "XenAppTraversalFormat"
#define MY_CF CF_RIFF

static void pWin32Error(const char* fmt, ...);
static int put_string_to_clipboard(UINT fmtid, int empty, LPCSTR str);
static int change_clip_str(int empty, UINT fmtid);

#define MAX_FORMATS 100

static HWND _createutilitywindow(WNDCLASS *wndclass) {
	ATOM classatom;
	HINSTANCE hinst = GetModuleHandle(NULL);
	HWND hwnd;

	wndclass->hInstance = hinst;
	classatom = RegisterClass(wndclass);
	if (classatom == 0) {
		pWin32Error("RegisterClass() failed");
		return NULL;
	}
	hwnd = CreateWindowEx(0, (LPCTSTR)classatom, NULL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hinst, NULL);
	if (!hwnd) {
		pWin32Error("CreateWindowEx() failed");
		return NULL;
	}
	return hwnd;
}

#define createutilitywindow(phwnd, lpfnWndProc, lpszClassName) do {   \
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


static HBITMAP dupbitmap(HBITMAP hbitmap)
{
	HDC dcSrc, dcDst;
	HBITMAP bDst;
	BITMAP bitmap;
	HGDIOBJ hbmOld, hbmOld2;
	HBITMAP rcBitmap = NULL;

	if (GetObject(hbitmap, sizeof(BITMAP), &bitmap) == 0) {
		pWin32Error("GetObject() failed");
		return NULL;
	}
	dcSrc = CreateCompatibleDC(NULL);
	if (!dcSrc) {
		pWin32Error("CreateCompatibleDC() failed");
		return NULL;
	}
	hbmOld = SelectObject(dcSrc, hbitmap);
	if (!hbmOld || hbmOld == HGDI_ERROR) {
		pWin32Error("SelectObject() failed");
		goto err;
	}
	dcDst = CreateCompatibleDC(NULL);
	if (!dcDst) {
		pWin32Error("CreateCompatibleDC() failed");
		goto err;
	}
	bDst = CreateBitmap(bitmap.bmWidth, bitmap.bmHeight, bitmap.bmPlanes, bitmap.bmBitsPixel, NULL);
	if (!bDst) {
		pWin32Error("CreateCompatibleBitmap() failed");
		goto err2;
	}

	hbmOld2 = SelectObject(dcDst, bDst);
	if (!hbmOld2 || hbmOld2 == HGDI_ERROR) {
		pWin32Error("SelectObject() failed");
		goto err3;
	}

	if (!BitBlt(dcDst, 0, 0, bitmap.bmWidth, bitmap.bmHeight, dcSrc, 0, 0, SRCCOPY)) {
		pWin32Error("BitBlt() failed");
		goto err3;
	}

	hbmOld = SelectObject(dcSrc, hbmOld);
	if (!hbmOld || hbmOld == HGDI_ERROR) {
		pWin32Error("SelectObject() failed");
		goto err3;
	}

	hbmOld2 = SelectObject(dcSrc, hbmOld2);
	if (!hbmOld2 || hbmOld2 == HGDI_ERROR) {
		pWin32Error("SelectObject() failed");
		goto err3;
	}
	
	rcBitmap = bDst;
	goto err2;
err3:
	if (!DeleteObject(bDst)) pWin32Error("DeleteObject() failed");
err2:
	if (!DeleteDC(dcDst)) pWin32Error("DeleteDC() failed");
err:
	if (!DeleteDC(dcSrc)) pWin32Error("DeleteDC() failed");
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
				pWin32Error("EnumClipboardFormats() failed");
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
			pWin32Error("cf %d GetClipboardData() failed", (int)fmtid);
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
				datas[ndatas].hglbl = dupbitmap(hglbsrc);
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
			pWin32Error("cf %d GlobalSize() failed", (int)fmtid);
			goto err;
		}
		datas[ndatas].hglbl = GlobalAlloc(GMEM_MOVEABLE, sz);
		if (datas[ndatas].hglbl == NULL) {
			pWin32Error("GlobalAlloc() failed");
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
		pWin32Error("EmptyClipboard() failed");

	 err:
		for (i = 0; i < ndatas; i++) {
			datas[i].freefunc(datas[i].hglbl);
		}

		return -1;
	}

	/* replace clipboard contents */
	for (i = 0; i < ndatas; i++) {
		if (!SetClipboardData(datas[i].fmtid, datas[i].hglbl)) {
			pWin32Error("SetClipboardData() failed");
			datas[i].freefunc(datas[i].hglbl);
		}
	}

	return 0;
}

HWND hwnd = NULL;

int senddata(HGLOBAL hdata) {
	static DWORD nseq = 0;
	DWORD newnseq;
	int rc = -1;

	while (!OpenClipboard(hwnd)) {
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
		pWin32Error("SetClipboardData() failed");
		goto err;
	}

	rc = 0;
err:
	if (!CloseClipboard()) {
		pWin32Error("CloseClipboard() failed");
	}
	nseq = GetClipboardSequenceNumber();
	printf("after %d\n", nseq);
	return rc;
}

#define BUFSZ 4

int main(int argc, char* argv[]) {
	HGLOBAL hglbl, hnew;
	LPVOID p;
	size_t sz;
	FILE *f = stdin;
	createutilitywindow(&hwnd, DefWindowProc, _T("myclipowner")); if (!hwnd) return 1;
	//setvbuf(f, NULL, _IONBF, 0); 
	for (;!feof(f);) {
		printf("*************\n");
		sz = BUFSZ;
		hglbl = GlobalAlloc(GMEM_MOVEABLE, sz);
		if (hglbl == NULL) {
			pWin32Error("GlobalAlloc() failed");
			return 1;
		}
		//sz = GlobalSize(hglbl);
		p = GlobalLock(hglbl);

		sz = fread(p, 1, sz, f);
		if (sz == 0) {
			if (!feof(f)) perror("fread() failed");
			goto err;
		}
		//*/

		GlobalUnlock(hglbl);

		hnew = GlobalReAlloc(hglbl, sz, 0);
		if (!hnew) {
			pWin32Error("GlobalReAlloc() failed");
			goto err;
		}
		hglbl = hnew;

		if (senddata(hglbl) < 0) {
err:
			printf("calling GlobalFree()\n");
			GlobalFree(hglbl);
			return 1;
		}

	}
	return 0;
}

static int change_clip_str(int empty, UINT fmtid)
{
	HGLOBAL hglb;
	LPSTR pclipdata;
	int i;
	char buf[20];

	if (!IsClipboardFormatAvailable(fmtid)) {
		printf("Data not available\n");
		return put_string_to_clipboard(fmtid, empty, "1");
	}
	hglb = GetClipboardData(fmtid);
	if (!hglb) {
		pWin32Error("GetClipboardData() failed");
		return -1;
	}

	pclipdata = GlobalLock(hglb);
	i = atoi(pclipdata);
	printf("got %s\n", pclipdata);
	GlobalUnlock(hglb);

	i++;
	sprintf(buf, "%d", i);
	return put_string_to_clipboard(fmtid, empty, buf);
}

static int put_string_to_clipboard(UINT fmtid, int empty, LPCSTR str)
{
	/*
	HGLOBAL hglb;
	LPSTR pclipdata; 

	if (empty) {
		if (!EmptyClipboard()) {
			pWin32Error("EmptyClipboard() failed");
			return -1;
		}
	}
	*/
	//if (dupandreplace(fmtid, empty) != 0) return -1;
	/*
	hglb = GlobalAlloc(GMEM_MOVEABLE, strlen(str)+1);
	if (hglb == NULL) {
		pWin32Error("GlobalAlloc() failed");
		return -1;
	}
	pclipdata = GlobalLock(hglb);
	strcpy(pclipdata, str);
	GlobalUnlock(hglb);
	if (!SetClipboardData(fmtid, hglb)) {
		pWin32Error("SetClipboardData() failed");
		GlobalFree(hglb);
	}
	*/
	return 0;
}

static char *cleanstr(char *s)
{
	while(*s) {
		switch((int)*s){
			case 13:
			case 10:
			*s=' ';
			break;
		}
		s++;
	}
	return s;
}

static void __pWin32Error(int level, DWORD eNum, const char* fmt, va_list args)
{
	char emsg[1024];
	char *pend = emsg + sizeof(emsg);
	size_t count = sizeof(emsg);
	unsigned u;

	do {
		u = (unsigned)_vsnprintf(pend - count, count, fmt, args);
		if (u >= count) break;
		count -= u;

		u = (unsigned)_snprintf(pend - count, count, ": ");
		if (u >= count) break;
		count -= u;

		u = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
															NULL, eNum,
															MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
															pend - count, (DWORD)count, NULL );
		if (u == 0) {
			u = (unsigned)_snprintf(pend - count, count, "0x%08x (%d)", eNum, eNum);
		}
	} while(0);

	emsg[sizeof(emsg)-1] = '\0';
	pend = cleanstr(emsg);

	if (pend < emsg + sizeof(emsg)-1) {
		pend++;
		*pend = '\0';
	}
	pend[-1] = '\n';
	puts(emsg);
}

static void pWin32Error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	__pWin32Error(0, GetLastError(), fmt, args);
	va_end(args);
}