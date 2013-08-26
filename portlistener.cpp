#include "cliptund.h"
#include "myeventloop.h"
#include "myportlistener.h"
#include "myclipserver.h"
#include "mylogging.h"

#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <stdio.h>

//#include <vector>
//using namespace std;

#include "mylastheader.h"

#define ADDRESSLENGTH (sizeof(struct sockaddr_in)+16)
#define LSN_BKLOG 128

typedef struct data_accept {
	SOCKET lsock;
	SOCKET asock;
	OVERLAPPED overlap;
	char buf[ADDRESSLENGTH*2];
	char clipname[40+1];
} data_accept;

enum cnnstate {
	STATE_SYN
};

typedef struct data_connection {
	cnnstate state;
	SOCKET sock;
	HANDLE ev;
	clipaddr remote;
} data_connection;

static int _cliptund_connection_func(void *param)
{
	data_connection *data = (data_connection *)param;
	switch (data->state) {
		case STATE_SYN:
			;
	}
	return 0;
}

int _cliptund_sock_to_clip(SOCKET sock, const char *clipname)
{
	data_connection *newdata;

	newdata = (data_connection*)malloc(sizeof(data_connection));
	newdata->state = STATE_SYN;
	newdata->sock = sock;
	newdata->ev = evloop_addlistener(_cliptund_connection_func, newdata);
	clipsrv_havenewdata();
	//clipsrv_connect(clipname, newdata->ev, &newdata->remote);
	return 0;
}

static int _cliptund_accept_func(void *param)
{
	data_accept *data = (data_accept *)param;
	DWORD nb;
	struct sockaddr_in localSockaddr, remoteSockaddr;
	struct sockaddr_in *pLocalSockaddr, *pRemoteSockaddr;
	int lenL, lenR;

	if (!GetOverlappedResult((HANDLE)data->lsock, &data->overlap, &nb, FALSE)) {
		pWin32Error(ERR, "GetOverlappedResult() failed");
		return -1;
	}

	GetAcceptExSockaddrs(data->buf, 0, ADDRESSLENGTH, ADDRESSLENGTH, (LPSOCKADDR*)&pLocalSockaddr, &lenL, (LPSOCKADDR*)&pRemoteSockaddr, &lenR);
	memcpy(&localSockaddr, pLocalSockaddr, sizeof(struct sockaddr_in));
	memcpy(&remoteSockaddr, pRemoteSockaddr, sizeof(struct sockaddr_in));
	{
		TCHAR buf[100];
		winet_inet_ntoa(remoteSockaddr.sin_addr, buf, 100);
		winet_log(INFO, "accepted %s:%d\n", buf, ntohs(remoteSockaddr.sin_port));
	}
	_cliptund_sock_to_clip(data->asock, data->clipname);

	if ((data->asock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		pWinsockError(ERR, "socket() failed");
		abort();
		//goto cleanup3;
	}

	if (!AcceptEx(data->lsock, data->asock, data->buf, 0, ADDRESSLENGTH, ADDRESSLENGTH, &nb, &data->overlap) && WSAGetLastError() != ERROR_IO_PENDING) {
		pWinsockError(ERR, "AcceptEx() failed");
		//evloop_removelistener(ev);
		//goto cleanup4;
	}
	return 0;
}

int cliptund_create_listener(short port, const char *clipname)
{
	int rc = -1;
	struct sockaddr_in saddr;
	data_accept *newdata;
	DWORD nb;
	HANDLE ev;
	SOCKET lsock, asock;

	newdata = (data_accept *)malloc(sizeof(data_accept));
	if (!newdata) {
		pSysError(ERR, "malloc() failed");
		return -1;
	}

	memset(&newdata->overlap, 0, sizeof(OVERLAPPED));

	if ((newdata->lsock = lsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		pWinsockError(ERR, "socket() failed");
		goto cleanup2;
	}

	if ((newdata->asock = asock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		pWinsockError(ERR, "socket() failed");
		goto cleanup3;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_addr.S_un.S_addr = INADDR_ANY;
	saddr.sin_port = htons((short int) port);
	saddr.sin_family = AF_INET;

	if (0 != bind(lsock, (const struct sockaddr *) &saddr, sizeof(saddr))) {
		pWinsockError(WARN, "failed to bind port %d", port);
		rc = -2; /* not fatal */
		goto cleanup4;
	}

	if (0 != listen(lsock, LSN_BKLOG)) {
		pWinsockError(ERR, "listen() failed");
		goto cleanup4;
	}

	newdata->overlap.hEvent = ev = evloop_addlistener(_cliptund_accept_func, newdata);

	strcpy(newdata->clipname, clipname);

	if (!AcceptEx(lsock, asock, newdata->buf, 0, ADDRESSLENGTH, ADDRESSLENGTH, &nb, &newdata->overlap) && WSAGetLastError() != ERROR_IO_PENDING) {
		pWinsockError(ERR, "AcceptEx() failed");
		evloop_removelistener(ev);
		goto cleanup4;
	}
	winet_log(INFO, "[%s] listening on port: %d\n", WINET_APPNAME, port);
	//CreateThread(NULL, 0, foo, lsock, 0, &nb);
	return 0;
cleanup4:
	closesocket(asock);
cleanup3:
	closesocket(lsock);
cleanup2:
	free(newdata);
	return rc;
}
