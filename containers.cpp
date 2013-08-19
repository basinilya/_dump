#include "cliptund.h"

#include <winsock2.h>
#include <windows.h>
#include <vector>

using namespace std;

static vector<WSAEVENT> hEvents;

void hEvents_add(WSAEVENT ev)
{
	hEvents.push_back(ev);
}

WSAEVENT *hEvents_get()
{
	return hEvents.empty() ? NULL : &hEvents.front();
}

DWORD hEvents_size()
{
	return (DWORD)hEvents.size();
}
