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
	evloop_func_t func;
	void *param;
	HANDLE ev;
} evloop_handler;

typedef struct handlers_t {
	volatile LONG refcount;
	vector<evloop_handler> a;

	handlers_t() : refcount(1) {}

	void release() {
		this->refcount--;
		if (this->refcount == 0) {
			/* Close unused events */
			evloop_handler *handlers = &this->a.front();
			for (int i = this->a.size()-1; i >= 0; i--) {
				if (!handlers[i].func) CloseHandle(handlers[i].ev);
			}
			delete this;
		}
	}

private:
	~handlers_t() {}
} handlers_t;

static struct {
	CRITICAL_SECTION lock;
	vector<HANDLE> events;
	vector<HANDLE> controlEventsPool;
	vector<HANDLE> controlEvents;
	handlers_t *current_handlers;
} ctx = { NULL, 0 };

static void _evloop_notifyall();
static void _evloop_release_control_event(HANDLE ev);
static HANDLE _evloop_acquire_control_event();

HANDLE evloop_addlistener(evloop_func_t func, void *param)
{
	evloop_handler handler;
	HANDLE newev;

	handler.ev = newev = CreateEvent(NULL, FALSE, FALSE, NULL);
	handler.func = func;
	handler.param = param;

	EnterCriticalSection(&ctx.lock);
	vector<evloop_handler> &handlers = ctx.current_handlers->a;
	if (handlers.size() < MAXIMUM_WAIT_OBJECTS-1) {
		handlers.push_back(handler);
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
	HANDLE *events = &ctx.events.front();
	int i;

	for (i = 0; events[i] != ev; i++);

	handlers_t *current_handlers = ctx.current_handlers;
	current_handlers->a[i].func = NULL;
	if (current_handlers->refcount > 1) {
		/* someone is waiting, need to duplicate */
		ctx.current_handlers = new handlers_t();
		ctx.current_handlers->a = current_handlers->a;
		current_handlers->release();
		current_handlers = ctx.current_handlers;
	}
	ctx.events.erase(ctx.events.begin()+i);
	current_handlers->a.erase(current_handlers->a.begin()+i);
	LeaveCriticalSection(&ctx.lock);
	return 0;
}

int evloop_processnext(evloop_data *data)
{
	HANDLE ev;
	HANDLE handles[MAXIMUM_WAIT_OBJECTS];
	DWORD nCount;
	DWORD dwrslt;
	handlers_t *current_handlers;

	EnterCriticalSection(&ctx.lock);
	if (ctx.controlEventsPool.empty()) {
		ev = CreateEvent(NULL, FALSE, FALSE, NULL);
	} else {
		ev = ctx.controlEventsPool.back();
		ctx.controlEventsPool.pop_back();
	}
	ctx.controlEvents.push_back(ev);

	current_handlers = ctx.current_handlers;
	current_handlers->refcount++;

	nCount = ctx.events.size();
	if (nCount != 0) {
		memcpy(handles, &ctx.events.front(), nCount*sizeof(handles[0]));
	}
	LeaveCriticalSection(&ctx.lock); /* If handlers array changes after this, we'll be notified */

	handles[0] = ev;
	nCount += 1;

	dwrslt = WaitForMultipleObjects(nCount, handles, FALSE, INFINITE);

	evloop_func_t func = NULL;
	void *param;

	EnterCriticalSection(&ctx.lock);

	/* remove my control event from array */
	vector<HANDLE>::iterator it;
	for (it = ctx.controlEvents.begin(); *it != ev; it++);
	ctx.controlEvents.erase(it);

	/* put my control event to pool */
	ctx.controlEventsPool.push_back(ev);

	switch(dwrslt) {
		case WAIT_OBJECT_0:
			break;
		default:
			dwrslt -= WAIT_OBJECT_0;
			func = current_handlers->a[dwrslt].func;
			param = current_handlers->a[dwrslt].param;
	}

	current_handlers->release();

	LeaveCriticalSection(&ctx.lock);

	if (func) func(param);

	return 0;
}

int evloop_init() {
	InitializeCriticalSection(&ctx.lock);
	ctx.current_handlers = new handlers_t();
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
