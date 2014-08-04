#ifndef _CLIPTUN_LOGGING_H
#define _CLIPTUN_LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

#define MYPROG_APPNAME "winzerofree"

#define MYPROG_LOG_DEBUG 1
#define MYPROG_LOG_MESSAGE 2
#define MYPROG_LOG_WARNING 3
#define MYPROG_LOG_ERROR 4

void winzerofree_pSysError(int lvl, char const *fmt, ...);
void winzerofree_pWinsockError(int lvl, char const *fmt, ...);
void winzerofree_pWin32Error(int lvl, char const *fmt, ...);
void winzerofree_log(int lvl, char const *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
