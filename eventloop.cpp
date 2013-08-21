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

typedef int (*evloop_func_t)(void *param);

/** add event listener */
HANDLE evloop_addlistener(evloop_func_t func, void *param);
/** remove event listener */
int evloop_removelistener(HANDLE ev);
/** wait for events and process next event */
int evloop_processnext();

#include <vector>
#include <string>
#include <map>

using namespace std;

typedef struct evloop_handler {
	evloop_func_t func;
	void *param;
} evloop_handler;

static struct {
	basic_string<HANDLE> events;
} ctx;

//static 
//static 

static void _evloop_notifythreads()
{
}

HANDLE evloop_addlistener(evloop_func_t func, void *param)
{
	evloop_handler handler;
	HANDLE newev;

	newev = CreateEvent(NULL, FALSE, FALSE, NULL);

	handler.func = func;
	handler.param = param;

	ctx.events += newev;
	//GetCurrentThreadId
	_evloop_notifythreads();
	return newev;
}
