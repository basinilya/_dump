/*

Use WaitForMultipleObjects().
Multiple threads can wait for the same event.
When event is set, only one thread is released.

Arbitrary thread can add or remove events.
Adding or removing an event modifies the global array.
The global event array can be modified while other threads are still waiting for it.
Removing an event from the array doesn't mean closing it, because then the behavior is undefined.
Removed event is closed when it's not used by all threads.

When event is addeded or removed, all threads must be released and start waiting on the new array.
Each thread must be released just once, for exampe, if you use a manual-reset control event to release
all threads, one of them may be busy and the others will loop, eating CPU.
semaphores are bad for this, because you can't choose, which thread will be released.
events are bad, because multiple notifications in a queue are possible.

Perhaps, each thread will have its own control event.
It will be auto-reset.
Notification will set all the events.

When adding an event, you pass:
- handler function
- function parameter
- pointer where to save the new event, e.g. &overlapped.hEvent

You don't create event yourself.

The returned event is later used as a key to remove.
You don't close the event yourself.

We'll close the event, when all threads waiting for it are notified.

When remove a handler, you don't need to wait for all threads to be notified.
You can free the data structure immediately, the handler function won't be called.

*/

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*evloop_func_t)(void *param);
typedef struct evloop_data {
	void *data1;
	void *data2;
} evloop_data;

/** init */
int evloop_init();

/** Add event listener
 *  Waiting threads are notified.
 */
HANDLE evloop_addlistener(evloop_func_t func, void *param);

/** Remove event listener
 *  Waiting threads are notified.
 *  The last notified thread closes event.
 */
int evloop_removelistener(HANDLE ev);

/** wait for events and process next event */
int evloop_processnext(evloop_data *data);

#ifdef __cplusplus
}
#endif

#include <stdio.h>
#include <vector>

using namespace std;

typedef struct evloop_handler {
	//LONG refcount;
	evloop_func_t func;
	void *param;
	//HANDLE hEvent;
} evloop_handler;

static struct {
	CRITICAL_SECTION lock;
	vector<HANDLE> events;
	vector<HANDLE> controlEventsPool;
	vector<HANDLE> controlEvents;
	vector<evloop_handler> handlers;
} ctx = { NULL, 0 };

static void _evloop_notifyall();
static void _evloop_release_control_event(HANDLE ev);
static HANDLE _evloop_acquire_control_event();

HANDLE evloop_addlistener(evloop_func_t func, void *param)
{
	evloop_handler handler;
	HANDLE newev;

	//handler.hEvent = 
	newev = CreateEvent(NULL, FALSE, FALSE, NULL);
	//handler.refcount = 1;
	handler.func = func;
	handler.param = param;

	EnterCriticalSection(&ctx.lock);
	if (ctx.handlers.size() < MAXIMUM_WAIT_OBJECTS-1) {
		ctx.handlers.push_back(handler);
		ctx.events.push_back(newev);
	} else {
		printf("too many events\n");
		CloseHandle(newev);
		newev = NULL;
	}
	_evloop_notifyall();
	LeaveCriticalSection(&ctx.lock);

	return newev;
}

int evloop_removelistener(HANDLE ev)
{
	EnterCriticalSection(&ctx.lock);
	HANDLE *events = ctx.events.front();
	for (int i = 0; /*it's there*/; i++) {
		if (events[i] == ev) {
			//ctx.events.erase(ctx.events.begin()+i);
			ctx.handlers[i].func = NULL;
			break;
		}
	}

	LeaveCriticalSection(&ctx.lock);
	return 0;
}

int evloop_processnext(evloop_data *data)
{
	HANDLE ev;
	HANDLE handles[MAXIMUM_WAIT_OBJECTS];
	DWORD nCount;
	DWORD dwrslt;

	EnterCriticalSection(&ctx.lock);
	if (ctx.controlEventsPool.empty()) {
		ev = CreateEvent(NULL, FALSE, FALSE, NULL);
	} else {
		ev = ctx.controlEventsPool.pop_back();
	}
	ctx.controlEvents.push_back(ev);
	LeaveCriticalSection(&ctx.lock); /* If handlers array changes after this, we'll be notified */

	handles[0] = ev;
	nCount = 1;

	EnterCriticalSection(&ctx.lock);
	for (vector<evloop_handler>::iterator it = ctx.handlers.begin(); it != ctx.handlers.end(); it++) {
		if (it->func) {
			it->refcount++;
			handles[nCount++] = it->hEvent;
		}
	}
	LeaveCriticalSection(&ctx.lock); /* If handlers array changes after this, we'll be notified */

	dwrslt = WaitForMultipleObjects(nCount, handles, FALSE, INFINITE);

	//EnterCriticalSection(&ctx.lock);
	switch(dwrslt) {
		case WAIT_OBJECT_0:
			_evloop_release_control_event(ev);
			data->data1 = NULL;
			if (nextev) SetEvent(nextev);
			break;
		default:
			dwrslt -= WAIT_OBJECT_0;
			(&ctx.handlers.front())[dwrslt];
			;
	}
	//LeaveCriticalSection(&ctx.lock);

	EnterCriticalSection(&ctx.lock);
	for (vector<evloop_handler>::iterator it = ctx.handlers.begin(); it != ctx.handlers.end(); it++) {
		_evloop_release_handler(it);
		break;
	}
	LeaveCriticalSection(&ctx.lock);

	return 0;
}

int evloop_init() {
	InitializeCriticalSection(&ctx.lock);
	return 0;
}

static HANDLE _evloop_acquire_control_event()
{
	/* TODO: optimize */
	return CreateEvent(NULL, FALSE, FALSE, NULL);
}

static void _evloop_release_control_event(HANDLE ev)
{
	/* TODO: optimize */
	CloseHandle(ev);
}

static void _evloop_notifyall()
{
	for (vector<HANDLE>::iterator it = ctx.controlEvents.begin(); it != ctx.controlEvents.end(); it++) {
		SetEvent(*it);
	}
}
