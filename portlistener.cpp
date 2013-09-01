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
	void addref() { tun->addref(); }
	void deref() { tun->deref(); }

	HANDLE ev_recv, ev_send;
	OVERLAPPED overlap_recv, overlap_send;

	volatile LONG lock_recv;

	void bufferavail() {
		rfifo_t *rfifo = &pump_recv->buf;
		DWORD nb = rfifo_availwrite(rfifo);
		if (nb != 0) {
			if (0 == InterlockedExchange(&lock_recv, 1)) {
				char *data = rfifo_pfree(rfifo);
				memset(&overlap_recv, 0, sizeof(OVERLAPPED));
				overlap_recv.hEvent = ev_recv;

				BOOL b;
				DWORD dw;
				if (1) {
					DWORD bufsz = nb;
					b = ReadFile((HANDLE)sock, data, nb, &nb, &overlap_recv);
					dw = GetLastError();
					winet_log(INFO, "ReadFile(sock=%d, bufsz=%d, nb=%d, ev=%p) == %d; err = %d\n", (int)sock, bufsz, nb, (void*)overlap_recv.hEvent, b, dw);
					static int i;
					i++;
					if (i == 2) {
						WaitForSingleObject(overlap_recv.hEvent, INFINITE);
						GetOverlappedResult((HANDLE)sock, &overlap_recv, &nb, FALSE);
						dw = 0;
					}
				} else if (0) {
					WSABUF wsabuf;
					wsabuf.buf = data;
					wsabuf.len = nb;
					DWORD flags = 0;
					int i = WSARecv(sock, &wsabuf, 1, &nb, &flags, &overlap_recv, NULL);
					dw = WSAGetLastError();
					winet_log(INFO, "WSARecv(sock=%d, n=%d, ev=%p) == %d; err = %d\n", (int)sock, nb, (void*)overlap_recv.hEvent, i, dw);
					WSASetLastError(dw);
					b = !i;
				} else {
					nb = recv(sock, data, nb, 0);
					nb = 0;
				}
				if (b || dw == ERROR_IO_PENDING) {
					addref();
				} else {
					pWin32Error(ERR, "ReadFile() failed");
					InterlockedExchange(&lock_recv, 0);
				}
			}
		}
	}

	void onEventRecv() {
		DWORD nb;
		rfifo_t *rfifo = &pump_recv->buf;
		BOOL b = GetOverlappedResult((HANDLE)sock, &overlap_recv, &nb, FALSE);
		DWORD dw = GetLastError();
		winet_log(INFO, "GetOverlappedResult(sock=%d, nb=%d, ev=%p) == %d; err = %d\n", (int)sock, nb, (void*)overlap_recv.hEvent, b, dw);
		SetLastError(dw);
		if (!b) {
			if (GetLastError() == ERROR_IO_PENDING) {
				//ReadFile((HANDLE)sock, 
			}
		}
		if (0 && nb == 0) {
			pump_recv->eof = 1;
			InterlockedExchange(&lock_recv, 0);
			pump_send->havedata();
		} else {
			rfifo_markwrite(rfifo, nb);
			InterlockedExchange(&lock_recv, 0);
			pump_send->havedata();
			bufferavail();
		}
		deref();
	}

	volatile LONG lock_send;

	void havedata() {
		rfifo_t *rfifo = &pump_send->buf;
		size_t nb = rfifo_availread(rfifo);
		if (nb != 0 || pump_send->eof) {
			if (0 == InterlockedExchange(&lock_send, 1)) {

				if (nb == 0) {
					shutdown(sock, SD_SEND);
				} else {
					char *data = rfifo_pdata(rfifo);
					rfifo_markread(rfifo, nb);
					memset(&overlap_send, 0, sizeof(OVERLAPPED));
					overlap_send.hEvent = ev_send;
					if (WriteFile((HANDLE)sock, data, nb, NULL, &overlap_send) || GetLastError() == ERROR_IO_PENDING) {
						addref();
					} else {
						pWin32Error(ERR, "WriteFile() failed");
						InterlockedExchange(&lock_send, 0);
					}
				}
			}
		}
	}

	void onEventSend() {
		DWORD nb;
		BOOL b = GetOverlappedResult((HANDLE)sock, &overlap_send, &nb, FALSE);
		b = 0;
		rfifo_t *rfifo = &pump_send->buf;
		rfifo_confirmread(rfifo, nb);
		InterlockedExchange(&lock_send, 0);
		pump_recv->bufferavail();
		havedata();
		deref();
	}

	TCPConnection(Tunnel *_tun, SOCKET _sock) : 
			Connection(_tun),
			sock(_sock),
			lock_send(0),lock_recv(0)
	{
		ev_recv = evloop_addlistener(static_cast<PinRecv*>(this));
		ev_send = evloop_addlistener(static_cast<PinSend*>(this));

		/* make it weak. Be sure not to set evs while destroying */
		tun->deref();
		tun->deref();
	}

	~TCPConnection() {
		evloop_removelistener(ev_recv);
		evloop_removelistener(ev_send);
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

	void addref() { SimpleRefcount::addref(); };
	void deref() { SimpleRefcount::deref(); };

	void onEvent() {
		data_accept *data = this;
		DWORD nb;
		struct sockaddr_in localSockaddr, remoteSockaddr;
		struct sockaddr_in *pLocalSockaddr, *pRemoteSockaddr;
		int lenL, lenR;

		if (!GetOverlappedResult((HANDLE)data->lsock, &data->overlap, &nb, FALSE)) {
			pWin32Error(ERR, "GetOverlappedResult() failed");
			abort();
		}

		GetAcceptExSockaddrs(data->buf, 0, ADDRESSLENGTH, ADDRESSLENGTH, (LPSOCKADDR*)&pLocalSockaddr, &lenL, (LPSOCKADDR*)&pRemoteSockaddr, &lenR);
		memcpy(&localSockaddr, pLocalSockaddr, sizeof(struct sockaddr_in));
		memcpy(&remoteSockaddr, pRemoteSockaddr, sizeof(struct sockaddr_in));
		{
			TCHAR buf[100];
			winet_inet_ntoa(remoteSockaddr.sin_addr, buf, 100);
			winet_log(INFO, "accepted sock=%d, %s:%d\n", (int)data->asock, buf, ntohs(remoteSockaddr.sin_port));
		}
/*
			{
				SOCKET sock;
				char buf[100];
				OVERLAPPED overlap_recv = { 0 };
				overlap_recv.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
				sock = data->asock;
				if (ReadFile((HANDLE)sock, buf, 100, NULL, &overlap_recv) || GetLastError() == ERROR_IO_PENDING) {
					pWin32Error(ERR, "ReadFile() ok");
				} else {
					pWin32Error(ERR, "ReadFile() failed");
				}
			}
*/
		{
			TCPConnection *conn = new TCPConnection(NULL, data->asock);
			Tunnel *tun = conn->tun;
			connfact->connect(tun);
			tun->deref();
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
			if (0) {
				BOOL b;
				DWORD nb;
				SOCKET sock;
				char data[2048];
				OVERLAPPED overlap_recv = { 0 };
				overlap_recv.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
				sock = conn->sock;
				if (ReadFile((HANDLE)sock, data, sizeof(data), NULL, &overlap_recv) || GetLastError() == ERROR_IO_PENDING) {
					pWin32Error(ERR, "ReadFile() ok");
					WaitForSingleObject(overlap_recv.hEvent, INFINITE);
					b = GetOverlappedResult((HANDLE)sock, &overlap_recv, &nb, FALSE);
					nb = 0;
				} else {
					pWin32Error(ERR, "ReadFile() failed");
				}
			}
			
			conn->tun->connected(conn);
		} else {
			pWinsockError(WARN, "connect() failed");
		}
		freeaddrinfo(pres);
	} else {
		pWinsockError(WARN, "getaddrinfo() failed");
	}
	conn->tun->deref();

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

		tun->addref();
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

int tcp_create_listener(short port, ConnectionFactory *connfact)
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
