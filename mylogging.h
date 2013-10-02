#ifndef _MYLOGGING_H
#define _MYLOGGING_H

#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WINET_APPNAME "cliptund"

#define WINET_LOG_DEBUG 1
#define WINET_LOG_MESSAGE 2
#define WINET_LOG_WARNING 3
#define WINET_LOG_ERROR 4

void cliptund_pSysError(int lvl, char const *fmt, ...);
void cliptund_pWinsockError(int lvl, char const *fmt, ...);
void cliptund_pWin32Error(int lvl, char const *fmt, ...);
void cliptund_log(int lvl, char const *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
