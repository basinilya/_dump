#ifndef _MYLOGGING_H
#define _MYLOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

#define WINET_LOG_MESSAGE 1
#define WINET_LOG_WARNING 2
#define WINET_LOG_ERROR 3

void pSysError(int lvl, char const *fmt, ...);
void pWinsockError(int lvl, char const *fmt, ...);
void pWin32Error(int lvl, char const *fmt, ...);
int winet_log(int level, char const *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
