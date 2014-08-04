#ifndef _CLIPTUN_LOGGING_H
#define _CLIPTUN_LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

#define WINET_APPNAME "winzerofree"

#define WINET_LOG_DEBUG 1
#define WINET_LOG_MESSAGE 2
#define WINET_LOG_WARNING 3
#define WINET_LOG_ERROR 4

void winzerofree_pSysError(int lvl, char const *fmt, ...);
void winzerofree_pWinsockError(int lvl, char const *fmt, ...);
void winzerofree_pWin32Error(int lvl, char const *fmt, ...);
void winzerofree_log(int lvl, char const *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
