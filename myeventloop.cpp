#include "myeventloop.h"
#include <stdio.h>
#include <vector>

using namespace std;

#ifdef _DEBUG
static void dbg_CloseHandle(const char *file, int line, HANDLE hObject) {
	if (!CloseHandle(hObject)) {
		printf("CloseHandle() failed at %s:%d\n", file, line);
	}
}

#define CloseHandle(hObject) dbg_CloseHandle(__FILE__, __LINE__, hObject)
#endif /* _DEBUG */

typedef struct evloop_handler {
	evloop_func_t func;
	void *param;
	HANDLE ev;
} evloop_handler;

struct handlers_t {
	vector<evloop_handler> a;

	handlers_t() : refcount(1) {}
	handlers_t(const vector<evloop_handler> &from) : refcount(1), a(from) {}

	void inline addref() { this->refcount++; }
	void deref() {
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
	LONG refcount;
	inline ~handlers_t() {}
};

static struct {
	CRITICAL_SECTION lock;
	vector<HANDLE> events;
	vector<HANDLE> controlEventsPool;
	vector<HANDLE> controlEvents;
	handlers_t *current_handlers;
} ctx;

static void _evloop_notifyall();

int evloop_init() {
	InitializeCriticalSection(&ctx.lock);
	ctx.current_handlers = new handlers_t();
	return 0;
}

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
		/* can append without duplicating the array */
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
	if (!ctx.controlEvents.empty()) {
		/* someone is waiting, need to duplicate the array, so index of signalled event is valid */
		ctx.current_handlers = new handlers_t(current_handlers->a);
		current_handlers->deref();
		current_handlers = ctx.current_handlers;
		_evloop_notifyall();
	} else {
		CloseHandle(ev);
	}
	ctx.events.erase(ctx.events.begin()+i);
	current_handlers->a.erase(current_handlers->a.begin()+i);
	LeaveCriticalSection(&ctx.lock);
	return 0;
}

int evloop_processnext()
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
	current_handlers->addref();

	nCount = ctx.events.size();
	if (nCount != 0) {
		memcpy(handles+1, &ctx.events.front(), nCount*sizeof(handles[0]));
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

	current_handlers->deref();

	LeaveCriticalSection(&ctx.lock);

	if (func) func(param);

	return 0;
}

static void _evloop_notifyall()
{
	for (vector<HANDLE>::iterator it = ctx.controlEvents.begin(); it != ctx.controlEvents.end(); it++) {
		SetEvent(*it);
	}
}
