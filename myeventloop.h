#ifndef _MY_EVENTLOOP_H
#define _MY_EVENTLOOP_H

#include <windows.h>
//#include <atlbase.h>

struct ISimpleRefcount {
	virtual void addref() = 0;
	virtual void deref() = 0;
};

struct SimpleRefcount : virtual ISimpleRefcount {
	volatile LONG refcount;
	void addref();
	void deref();
	inline SimpleRefcount() : refcount(1) {}
protected:
	virtual ~SimpleRefcount() {}
};

struct IEventPin : virtual ISimpleRefcount {
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
