#include "myeventloop.h"

#include <windows.h>
#include <stdio.h>
#include <vector>
#include <stdlib.h> /* for abort() */

using namespace std;

#ifdef _DEBUG
static void dbg_CloseHandle(const char *file, int line, HANDLE hObject) {
	if (!CloseHandle(hObject)) {
		printf("CloseHandle() failed at %s:%d\n", file, line);
		abort();
	}
}

#define CloseHandle(hObject) dbg_CloseHandle(__FILE__, __LINE__, hObject)
#endif /* _DEBUG */

#include "mylogging.h"
#include "mylastheader.h"
void SimpleRefcount::addref() {
	LONG newrefcount = InterlockedIncrement(&this->refcount);
	//winet_log(INFO, "SimpleRefcount::addref %p %ld\n", this, (long)newrefcount);
}

void SimpleRefcount::deref() {
	LONG newrefcount = InterlockedDecrement(&this->refcount);
	//winet_log(INFO, "SimpleRefcount::deref %p %ld\n", this, (long)newrefcount);
	if (newrefcount == 0) {
		delete this;
	}
}

typedef struct evloop_handler {
	IEventPin *pin;
	HANDLE ev;
} evloop_handler;

struct handlerswrap {
	vector<evloop_handler> a;

	handlerswrap() : refcount(1) {}
	handlerswrap(const vector<evloop_handler> &from) : refcount(1), a(from) {}

	inline
	void addref() { this->refcount++; }

	void deref() {
		this->refcount--;
		if (this->refcount == 0) {
			/* Close unused events */
			evloop_handler *phandlers = &this->a.front();
			for (int i = this->a.size()-1; i >= 0; i--) {
				if (!phandlers[i].pin) CloseHandle(phandlers[i].ev);
			}
			delete this;
		}
	}

private:
	LONG refcount;
	inline ~handlerswrap() {}
};

static struct {
	CRITICAL_SECTION lock;
	vector<HANDLE> events;
	vector<HANDLE> controlEventsPool;
	vector<HANDLE> controlEvents;
	handlerswrap *current_handlers;
} ctx;

static void _evloop_notifyall();

int evloop_init() {
	InitializeCriticalSection(&ctx.lock);
	ctx.current_handlers = new handlerswrap();
	return 0;
}

HANDLE evloop_addlistener(IEventPin *pin)
{
	evloop_handler handler;
	HANDLE newev;

	handler.ev = newev = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!newev) {
		printf("CreateEvent() failed\n");
		abort();
	}
	pin->addref();
	handler.pin = pin;

	EnterCriticalSection(&ctx.lock);
	vector<evloop_handler> &handlers = ctx.current_handlers->a;
	if (handlers.size() < MAXIMUM_WAIT_OBJECTS-1) {
		/* can append without duplicating the array */
		handlers.push_back(handler);
		ctx.events.push_back(newev);
	} else {
		printf("too many events\n");
		abort();
		//CloseHandle(newev);
		//newev = NULL;
	}
	_evloop_notifyall();
	LeaveCriticalSection(&ctx.lock);

	return newev;
}

int evloop_removelistener(HANDLE ev)
{
	EnterCriticalSection(&ctx.lock);
	HANDLE *pevents = &ctx.events.front();
	int i;

	for (i = 0; pevents[i] != ev; i++);

	if (i >= (int)ctx.events.size()) {
		printf("bad event to remove\n");
		abort();
	}

	handlerswrap *phandlerswrap = ctx.current_handlers;
	evloop_handler &handler = phandlerswrap->a[i];
	handler.pin->deref();
	handler.pin = NULL;
	if (!ctx.controlEvents.empty()) {
		/* ev is not used anymore, but someone is waiting for it. Can't close it now */
		/* Besides, when wait returns, the index of signalled event must be valid */
		/* Duplicating the array, so new waits don't wait for ev */
		/* Current waits use the old array */
		ctx.current_handlers = new handlerswrap(phandlerswrap->a);
		phandlerswrap->deref();
		phandlerswrap = ctx.current_handlers;
		_evloop_notifyall();
	} else {
		CloseHandle(ev);
	}
	ctx.events.erase(ctx.events.begin()+i);
	phandlerswrap->a.erase(phandlerswrap->a.begin()+i);
	LeaveCriticalSection(&ctx.lock);
	return 0;
}

int evloop_processnext()
{
	HANDLE ev;
	HANDLE handles[MAXIMUM_WAIT_OBJECTS];
	DWORD nCount;
	DWORD dwrslt;
	handlerswrap *phandlerswrap;
	IEventPin *pin = NULL;

	EnterCriticalSection(&ctx.lock);
	if (ctx.controlEventsPool.empty()) {
		ev = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (!ev) {
			printf("CreateEvent() failed\n");
			abort();
		}
	} else {
		ev = ctx.controlEventsPool.back();
		ctx.controlEventsPool.pop_back();
	}
	ctx.controlEvents.push_back(ev);

//notified_retry:
	nCount = ctx.events.size();
	if (nCount != 0) {
		memcpy(handles+1, &ctx.events.front(), nCount*sizeof(handles[0]));
	}

	phandlerswrap = ctx.current_handlers;
	phandlerswrap->addref();

	LeaveCriticalSection(&ctx.lock); /* If handlers array changes after this, we'll be notified */

	handles[0] = ev;
	nCount += 1;

	dwrslt = WaitForMultipleObjects(nCount, handles, FALSE, INFINITE);

	EnterCriticalSection(&ctx.lock);

	switch(dwrslt) {
		case WAIT_FAILED:
			printf("WaitForMultipleObjects() failed\n");
			abort();
		case WAIT_OBJECT_0:
			phandlerswrap->deref();
			break;
			//goto notified_retry;
		default:
			dwrslt -= (WAIT_OBJECT_0 + 1);
			pin = phandlerswrap->a[dwrslt].pin;
			if (pin) {
				pin->addref();
				winet_log(INFO, "event triggered %p\n", (void*)handles[dwrslt+1]);
			}
	}

	phandlerswrap->deref();

	/* remove my control event from array */
	vector<HANDLE>::iterator it;
	for (it = ctx.controlEvents.begin(); *it != ev; it++);
	ctx.controlEvents.erase(it);

	/* put my control event to pool */
	ctx.controlEventsPool.push_back(ev);

	LeaveCriticalSection(&ctx.lock);

	/* at this point phandlerswrap->a[dwrslt].func may change to NULL,
	   but it's no different from when already inside func */

	if (pin) {
		pin->onEvent();
		pin->deref();
	}

	return 0;
}

static void _evloop_notifyall()
{
	for (vector<HANDLE>::iterator it = ctx.controlEvents.begin(); it != ctx.controlEvents.end(); it++) {
		if (!SetEvent(*it)) {
			printf("SetEvent() failed\n");
			abort();
		}
	}
}
