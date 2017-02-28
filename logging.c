#include "mylogging.h"
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#ifdef _MSC_VER
#include <windows.h>
#include <tchar.h>
#undef snprintf
#define snprintf _snprintf
#endif

#include "mylastheader.h"

int MYPROG_LOG_LEVEL = MYPROG_LOG_WARNING;

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

static void _myprog_log(int level, char *emsg, size_t emsgsz)
{
	char *pend;

	if (level >= MYPROG_LOG_LEVEL) {
		pend = emsg + strlen(emsg);

		if (pend < emsg + emsgsz-1) {
			pend++;
			*pend = '\0';
		}
		pend[-1] = '\n';

		fputs(emsg, stdout);
		fflush(stdout);

		pend[-1] = '\0';
	}

	if (level >= MYPROG_LOG_LEVEL) {
		pend = cleanstr(emsg);
		/* TODO: write to log file */
	}
}

static void __myprog_log(int level, char mode, int eNum, const char* fmt, va_list args)
{
	char emsg[4096];
	char *pend = emsg + sizeof(emsg);
	size_t count = sizeof(emsg);
	unsigned u;

	do {
		u = (unsigned)vsnprintf(pend - count, count, fmt, args);
		if (u >= count) break;
		count -= u;

		if (mode != 'm') {
			u = (unsigned)snprintf(pend - count, count, ": 0x%08x (%d): ", eNum, eNum);
			if (u >= count) break;
			count -= u;

			if (mode == 's') {
				u = (unsigned)snprintf(pend - count, count, "%s", strerror(eNum));
			}
#ifdef WIN32
			else if (mode == 'w') {
				u = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
																	NULL, eNum,
																	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
																	pend - count, count, NULL );
			}
#endif
			else {
				abort();
			}
		}
	} while(0);

	emsg[sizeof(emsg)-1] = '\0';

	_myprog_log(level, emsg, sizeof(emsg));
}

void myprog_pSysError(int lvl, char const *fmt, ...)
{
	va_list args;
	int myerrno = errno;

	va_start(args, fmt);
	__myprog_log(lvl, 's', myerrno, fmt, args);
	va_end(args);
}

#ifdef WIN32
void myprog_pWin32Error(int lvl, char const *fmt, ...)
{
	va_list args;
	DWORD eNum = GetLastError();

	va_start(args, fmt);
	__myprog_log(lvl, 'w', eNum, fmt, args);
	va_end(args);
}
#endif

void myprog_log(int lvl, char const *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	__myprog_log(lvl, 'm', 0, fmt, args);
	va_end(args);
}
