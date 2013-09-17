#include "cliptund.h"
#include "mytunnel.h"
#include "myeventloop.h"
#include "myportlistener.h"
#include "myclipserver.h"
#include "mylogging.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#include <mswsock.h>
#include <windows.h>


#include <stdio.h>

//#include <vector>
//using namespace std;

#include "mylastheader.h"

using namespace cliptund;

#define ADDRESSLENGTH (sizeof(struct sockaddr_in)+16)
#define LSN_BKLOG 128

static int _cliptund_accept_func(void *param);

struct PinRecv : IEventPin, virtual Connection {
	virtual void onEventRecv() = 0;
	void onEvent() { onEventRecv(); }
	PinRecv() : Connection((abort(),NULL)) { }
};

struct PinSend : IEventPin, virtual Connection {
	virtual void onEventSend() = 0;
	void onEvent() { onEventSend(); }
	PinSend() : Connection((abort(),NULL)) { }
};

struct TCPConnection : PinRecv, PinSend {
	DeclRefcountMethods(tun->)

	int firstread;

	HANDLE ev_recv, ev_send;
	OVERLAPPED overlap_recv, overlap_send;

	volatile LONG lock_recv;

	void bufferavail() {
		if (pump_recv->eof) {
			return;
		}
		rfifo_t *rfifo = &pump_recv->buf;
		DWORD nb = rfifo_availwrite(rfifo);
		if (nb != 0) {
			if (0 == InterlockedExchange(&lock_recv, 1)) {
				char *data = rfifo_pfree(rfifo);
				memset(&overlap_recv, 0, sizeof(OVERLAPPED));
				if (!ev_recv) ev_recv = evloop_addlistener(static_cast<PinRecv*>(this));
				overlap_recv.hEvent = ev_recv;

				ReadFile((HANDLE)sock, data, nb, &nb, &overlap_recv);
			}
		}
	}

	void onEventRecv() {
		if (firstread) {
			firstread = 0;
			tun->connected();
			//Release();
			return;
		}
		DWORD nb;
		rfifo_t *rfifo = &pump_recv->buf;
		GetOverlappedResult((HANDLE)sock, &overlap_recv, &nb, FALSE);
		if (nb == 0) {
			pump_recv->eof = 1;
			InterlockedExchange(&lock_recv, 0);
			evloop_removelistener(ev_recv);
			pump_send->havedata();
		} else {
			rfifo_markwrite(rfifo, nb);
			InterlockedExchange(&lock_recv, 0);
			pump_send->havedata();
			bufferavail();
		}
		//Release();
	}

	volatile LONG lock_send;

	void havedata() {
		rfifo_t *rfifo = &pump_send->buf;
		DWORD nb = rfifo_availread(rfifo);
		if (nb != 0 || pump_send->eof) {
			if (0 == InterlockedExchange(&lock_send, 1)) {

				if (nb == 0) {
					shutdown(sock, SD_SEND);
					InterlockedExchange(&lock_send, 0);
					if (ev_send) evloop_removelistener(ev_send);
				} else {
					char *data = rfifo_pdata(rfifo);
					rfifo_markread(rfifo, nb);
					memset(&overlap_send, 0, sizeof(OVERLAPPED));
					if (!ev_send) ev_send = evloop_addlistener(static_cast<PinSend*>(this));
					overlap_send.hEvent = ev_send;

					WriteFile((HANDLE)sock, data, nb, &nb, &overlap_send);
				}
			}
		}
	}

	void onEventSend() {
		DWORD nb;
		GetOverlappedResult((HANDLE)sock, &overlap_send, &nb, FALSE);
		rfifo_t *rfifo = &pump_send->buf;
		rfifo_confirmread(rfifo, nb);
		InterlockedExchange(&lock_send, 0);
		pump_recv->bufferavail();
		havedata();
		//Release();
	}

	TCPConnection(Tunnel *_tun, SOCKET _sock) : 
			Connection(_tun),
			sock(_sock),
			firstread(0),
			ev_recv(0), ev_send(0),
			lock_send(0),lock_recv(0)
	{
	}

	~TCPConnection() {
		closesocket(sock);
	}
	SOCKET sock;
	union {
		struct {
			char hostname[40+1];
			short port;
		} toresolv;
	} a;
};

struct data_accept : SimpleRefcount, IEventPin {
	SOCKET lsock;
	SOCKET asock;
	OVERLAPPED overlap;
	char buf[ADDRESSLENGTH*2];
	ConnectionFactory *connfact;

	~data_accept() {
		if (lsock != INVALID_SOCKET) closesocket(lsock);
		if (asock != INVALID_SOCKET) closesocket(asock);
		delete connfact;
	}

	DeclRefcountMethods(SimpleRefcount::)

	void onEvent() {
		data_accept *data = this;
		DWORD nb;
		struct sockaddr_in localSockaddr, remoteSockaddr;
		struct sockaddr_in *pLocalSockaddr, *pRemoteSockaddr;
		int lenL, lenR;

		GetOverlappedResult((HANDLE)data->lsock, &data->overlap, &nb, FALSE);

		GetAcceptExSockaddrs(data->buf, 0, ADDRESSLENGTH, ADDRESSLENGTH, (LPSOCKADDR*)&pLocalSockaddr, &lenL, (LPSOCKADDR*)&pRemoteSockaddr, &lenR);
		memcpy(&localSockaddr, pLocalSockaddr, sizeof(struct sockaddr_in));
		memcpy(&remoteSockaddr, pRemoteSockaddr, sizeof(struct sockaddr_in));
		{
			TCHAR buf[100];
			winet_inet_ntoa(remoteSockaddr.sin_addr, buf, 100);
			winet_log(INFO, "accepted sock=%d, %s:%d\n", (int)data->asock, buf, ntohs(remoteSockaddr.sin_port));
			setsockopt(data->asock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&data->lsock, sizeof(data->lsock));
		}
		{
			TCPConnection *conn = new TCPConnection(NULL, data->asock);
			Tunnel *tun = conn->tun;
			connfact->connect(tun);
			tun->Release();
		}

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
	}

};

/*
typedef struct data_connection {
	clip_connection conn;
	SOCKET sock;
	HANDLE ev;
} data_connection;
*/
/*
static int _cliptund_connection_func(void *param)
{
	data_connection *data = (data_connection *)param;
	return 0;
}
*/

static DWORD WINAPI resolvethread(LPVOID param) {
	TCPConnection *conn = (TCPConnection *)param;

	static const ADDRINFO hints = {
		0,              /*  int             ai_flags;*/
		AF_INET,        /*  int             ai_family;*/
		0,              /*  int             ai_socktype;*/
		0,              /*  int             ai_protocol;*/
		0,              /*  size_t          ai_addrlen;*/
		NULL,           /*  char            *ai_canonname;*/
		NULL,           /*  struct sockaddr  *ai_addr;*/
		NULL            /*  struct addrinfo  *ai_next;*/
	};
	ADDRINFO *pres;

	if (0 == getaddrinfo(conn->a.toresolv.hostname, NULL, &hints, &pres)) {
		struct sockaddr_in *inaddr ((struct sockaddr_in*)pres->ai_addr);
		inaddr->sin_port = htons(conn->a.toresolv.port);
		if (0 == connect(conn->sock, pres->ai_addr, pres->ai_addrlen)) {

			winet_log(INFO, "connected sock=%d, %s:%hd\n", (int)conn->sock, conn->a.toresolv.hostname, conn->a.toresolv.port);

			conn->firstread = 1;

			//conn->AddRef();
			conn->ev_recv = evloop_addlistener(static_cast<PinRecv*>(conn));
			SetEvent(conn->ev_recv);

		} else {
			pWinsockError(WARN, "connect() failed");
		}
		freeaddrinfo(pres);
	} else {
		pWinsockError(WARN, "getaddrinfo() failed");
	}
	conn->tun->Release();

	return 0;
}

struct TCPConnectionFactory : ConnectionFactory {
	char host[40+1];
	short port;
	void connect(Tunnel *tun) {
		DWORD tid;
		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		TCPConnection *conn = new TCPConnection(tun, sock);
		strcpy(conn->a.toresolv.hostname, host);
		conn->a.toresolv.port = port;
		//conn->tun = tun;

		tun->AddRef();
		CreateThread(NULL, 0, resolvethread, conn, 0, &tid);
	}
};

ConnectionFactory *tcp_CreateConnectionFactory(const char host[40+1], short port)
{
	TCPConnectionFactory *connfact = new TCPConnectionFactory();
	strcpy(connfact->host, host);
	connfact->port = port;
	return connfact;
}

int tcp_create_listener(ConnectionFactory *connfact, short port)
{
	int rc = -1;
	struct sockaddr_in saddr;
	data_accept *newdata;
	DWORD nb;
	HANDLE ev;
	SOCKET lsock, asock;

	newdata = new data_accept();

	newdata->connfact = connfact;

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

	newdata->overlap.hEvent = ev = evloop_addlistener(newdata);

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
