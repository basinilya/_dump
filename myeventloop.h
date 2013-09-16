#ifndef _MY_EVENTLOOP_H
#define _MY_EVENTLOOP_H

#include <windows.h>

struct IEventPin : IUnknown {
	virtual void onEvent() = 0;
};

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

#endif
