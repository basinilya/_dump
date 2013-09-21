#include "mylogging.h"
#include <stdio.h>
#include <windows.h>
#include <tchar.h>

#include "mylastheader.h"

static void winet_evtlog(char const *logmsg, long type);

static void _winet_log(int level, char const *emsg)
{
	SYSTEMTIME time;
	if (level == WINET_LOG_DEBUG) return;
	GetLocalTime(&time);
	printf("%02d:%02d.%03d %s", time.wMinute, time.wSecond, time.wMilliseconds, emsg);

	if (level == WINET_LOG_ERROR)
		winet_evtlog(emsg, EVENTLOG_ERROR_TYPE);
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


static void __cliptund_log(int level, char mode, DWORD eNum, const char* fmt, va_list args)
{
	char emsg[1024];
	char *pend = emsg + sizeof(emsg);
	size_t count = sizeof(emsg);
	unsigned u;

	do {
		u = _snprintf(pend - count, count, "[%u] ", GetCurrentThreadId());
		if (u >= count) break;
		count -= u;

		u = (unsigned)_vsnprintf(pend - count, count, fmt, args);
		if (u >= count) break;
		count -= u;

		if (mode != 'm') {
			u = (unsigned)_snprintf(pend - count, count, ": 0x%08x (%d): ", eNum, eNum);
			if (u >= count) break;
			count -= u;

			if (mode == 'w') {
				u = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
																	NULL, eNum,
																	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
																	pend - count, count, NULL );
			} else if (mode == 's') {
				u = (unsigned)_snprintf(pend - count, count, "%s", strerror(eNum));
			}
		}
	} while(0);

	emsg[sizeof(emsg)-1] = '\0';
	pend = cleanstr(emsg);

	if (pend < emsg + sizeof(emsg)-1) {
		pend++;
		*pend = '\0';
	}
	pend[-1] = '\n';
	_winet_log(level, emsg);
}

void cliptund_pSysError(int lvl, char const *fmt, ...)
{
	va_list args;
	int myerrno = errno;

	va_start(args, fmt);
	__cliptund_log(lvl, 's', myerrno, fmt, args);
	va_end(args);
}

void cliptund_pWin32Error(int lvl, char const *fmt, ...)
{
	va_list args;
	DWORD eNum = GetLastError();

	va_start(args, fmt);
	__cliptund_log(lvl, 'w', eNum, fmt, args);
	va_end(args);
}

void cliptund_pWinsockError(int lvl, char const *fmt, ...)
{
	va_list args;
	DWORD eNum = WSAGetLastError();

	va_start(args, fmt);
	__cliptund_log(lvl, 'w', eNum, fmt, args);
	va_end(args);
}

static void winet_evtlog(char const *logmsg, long type) {
	DWORD err;
	HANDLE hesrc;
	LPTSTR tmsg;
	_TCHAR lmsg[128];
	LPCTSTR strs[2];
	_TCHAR wmsg[1024];

	winet_a2t(logmsg, wmsg, COUNTOF(wmsg));
	tmsg = wmsg;

	err = GetLastError();
	hesrc = RegisterEventSource(NULL, _TEXT(WINET_APPNAME));

	_stprintf(lmsg, _TEXT("%s error: 0x%08x"), _TEXT(WINET_APPNAME), err);
	strs[0] = lmsg;
	strs[1] = tmsg;

	if (hesrc != NULL) {
		ReportEvent(hesrc, (WORD) type, 0, 0, NULL, 2, 0, strs, NULL);

		DeregisterEventSource(hesrc);
	}
}

void cliptund_log(int lvl, char const *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	__cliptund_log(lvl, 'm', 0, fmt, args);
	va_end(args);
}
