#ifndef _MY_EVENTLOOP_H
#define _MY_EVENTLOOP_H

#include "cliptund.h"
#include <windows.h>
//#include <atlbase.h>

struct IEventPin : ISimpleRefcount {
	virtual void onEvent() = 0;
};


#ifdef __cplusplus
extern "C" {
#endif

/** init */
int evloop_init();

/** Add event listener
 *  Waiting threads are notified.
 */
HANDLE evloop_addlistener(IEventPin *pin);

/** Remove event listener
 *  Waiting threads are notified.
 *  The last notified thread closes event.
 */
int evloop_removelistener(HANDLE ev);

/** wait for events and process next event */
int evloop_processnext();

#ifdef __cplusplus
}
#endif


#endif
