#include "cliptund.h"

#include <winsock2.h>
#include <windows.h>
#include <vector>

using namespace std;

static vector<WSAEVENT> hEvents;

//typedef int (*on_event_t)(DWORD i_event, void *param);

static vector<wsaevent_handler> datas;

//static struct wsaevent_data model = {};

void hEvents_add(WSAEVENT ev, const wsaevent_handler *handler_model)
{
	hEvents.push_back(ev);
	datas.push_back(*handler_model);
}

WSAEVENT *hEvents_getall()
{
	return hEvents.empty() ? NULL : &hEvents.front();
}

DWORD hEvents_size()
{
	return (DWORD)hEvents.size();
}

wsaevent_handler *datas_get(DWORD index)
{
	return &datas.at(index);
}