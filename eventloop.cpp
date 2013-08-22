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

/** init */
int evloop_init();
/** add event listener */
HANDLE evloop_addlistener(evloop_func_t func, void *param);
/** remove event listener */
int evloop_removelistener(HANDLE ev);
/** wait for events and process next event */
int evloop_processnext();

#ifdef __cplusplus
}
#endif

#include <stdio.h>
#include <vector>

using namespace std;

typedef struct evloop_handler {
	LONG refcount;
	evloop_func_t func;
	void *param;
	HANDLE hEvent;
} evloop_handler;

static struct {
	volatile HANDLE controlEvent;
	CRITICAL_SECTION critsec;
	vector<evloop_handler> handlers;
} ctx = { NULL };

static void _evloop_notifyall();
static void _evloop_release_handler(vector<evloop_handler>::iterator &_Where);
static void _evloop_release_control_event(HANDLE ev);
static HANDLE _evloop_acquire_control_event();

HANDLE evloop_addlistener(evloop_func_t func, void *param)
{
	evloop_handler handler;
	HANDLE newev;

	handler.hEvent = newev = CreateEvent(NULL, FALSE, FALSE, NULL);
	handler.refcount = 1;
	handler.func = func;
	handler.param = param;

	EnterCriticalSection(&ctx.critsec);
	if (ctx.handlers.size() < MAXIMUM_WAIT_OBJECTS-1) {
		ctx.handlers.push_back(handler);
	} else {
		printf("too many events\n");
		CloseHandle(newev);
		newev = NULL;
	}
	LeaveCriticalSection(&ctx.critsec);

	_evloop_notifyall();
	return newev;
}

int evloop_removelistener(HANDLE ev)
{
	EnterCriticalSection(&ctx.critsec);
	for (vector<evloop_handler>::iterator it = ctx.handlers.begin(); it != ctx.handlers.end(); it++) {
		if (it->hEvent == ev) {
			it->func = NULL;
			_evloop_release_handler(it);
			break;
		}
	}
	LeaveCriticalSection(&ctx.critsec);
	return 0;
}

int evloop_processnext()
{
	HANDLE ev, nextev;
	HANDLE handles[MAXIMUM_WAIT_OBJECTS];
	DWORD nCount;
	DWORD dwrslt;

	handles[0] = ev = _evloop_acquire_control_event();
	nCount = 1;

	nextev = InterlockedExchangePointer(&ctx.controlEvent, ev);

	EnterCriticalSection(&ctx.critsec);
	for (vector<evloop_handler>::iterator it = ctx.handlers.begin(); it != ctx.handlers.end(); it++) {
		if (it->func) {
			it->refcount++;
			handles[nCount++] = it->hEvent;
		}
	}
	LeaveCriticalSection(&ctx.critsec); /* If handlers array changes after this, we'll be notified */

	dwrslt = WaitForMultipleObjects(nCount, handles, FALSE, INFINITE);
	_evloop_release_control_event(ev);

	EnterCriticalSection(&ctx.critsec);
	dwrslt -= WAIT_OBJECT_0;
	switch(dwrslt) {
		case 0:
			if (nextev) SetEvent(nextev);
			break;
		default:
			(&ctx.handlers.front())[dwrslt];
			;
	}
	LeaveCriticalSection(&ctx.critsec);

	EnterCriticalSection(&ctx.critsec);
	for (vector<evloop_handler>::iterator it = ctx.handlers.begin(); it != ctx.handlers.end(); it++) {
		_evloop_release_handler(it);
		break;
	}
	LeaveCriticalSection(&ctx.critsec);

	return 0;
}

static void _evloop_release_handler(vector<evloop_handler>::iterator &_Where)
{
	if ( (--_Where->refcount) == 0) {
		CloseHandle(_Where->hEvent);
		ctx.handlers.erase(_Where);
	}
}

int evloop_init() {
	InitializeCriticalSection(&ctx.critsec);
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
	HANDLE ev;

	/* all waits after this are part of a new chain */
	ev = InterlockedExchangePointer(&ctx.controlEvent, NULL);
	if (ev) SetEvent(ev);
}
