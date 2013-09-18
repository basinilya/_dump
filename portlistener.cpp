#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#include <mswsock.h>
#include <windows.h>

#include "cliptund.h"
#include "mytunnel.h"
#include "myeventloop.h"
#include "myportlistener.h"
#include "myclipserver.h"
#include "mylogging.h"

#include <stdio.h>

#include "mylastheader.h"

using namespace cliptund;

#define ADDRESSLENGTH (sizeof(struct sockaddr_in)+16)
#define LSN_BKLOG 128

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

static void overlap_reset(LPOVERLAPPED overlap) {
	HANDLE ev = overlap->hEvent;
	memset(overlap, 0, sizeof(OVERLAPPED));
	overlap->hEvent = ev;
}

struct TCPConnection : PinRecv, PinSend {
	DeclRefcountMethods(tun->)

	int firstread;

	OVERLAPPED overlap_recv, overlap_send;

	volatile LONG lock_recv;

	void bufferavail() {
		rfifo_t *rfifo = &pump_recv->buf;
		DWORD nb = rfifo_availwrite(rfifo);
		if (nb != 0 || pump_recv->writeerror) {
			if (0 == InterlockedExchange(&lock_recv, 1)) {
				if (pump_recv->writeerror) {
					shutdown(sock, SD_RECEIVE);
					if (overlap_recv.hEvent) evloop_removelistener(overlap_recv.hEvent);
				} else {
					char *data = rfifo_pfree(rfifo);
					if (!overlap_recv.hEvent) overlap_recv.hEvent = evloop_addlistener(static_cast<PinRecv*>(this));
					overlap_reset(&overlap_recv);

					if (!ReadFile((HANDLE)sock, data, nb, &nb, &overlap_recv) && GetLastError() != ERROR_IO_PENDING) {
						readerror(FALSE);
					}
				}
			}
		}
	}

	void getpeer(char buf[100], struct sockaddr_in *saddr) {
		DWORD lasterr = GetLastError();
		int namelen = sizeof(struct sockaddr_in);
		getpeername(sock, (struct sockaddr*)saddr, &namelen);
		winet_inet_ntoa(saddr->sin_addr, buf, 100);
		SetLastError(lasterr);
	}

	void readerror(BOOL normal) {
		DWORD lasterr = GetLastError();
		char buf[100];
		struct sockaddr_in saddr;
		getpeer(buf, &saddr);

		if (normal) {
			log(INFO, "peer %s:%d, disconnected", buf, ntohs(saddr.sin_port));
		} else {
			pWin32Error(INFO, "peer %s:%d, read failed", buf, ntohs(saddr.sin_port));
		}
		/* keep locked */
		pump_recv->eof = 1;
		evloop_removelistener(overlap_recv.hEvent);
		pump_send->havedata();
	}

	void onEventRecv() {
		if (firstread) {
			firstread = 0;
			tun->connected();
			return;
		}
		DWORD nb;
		rfifo_t *rfifo = &pump_recv->buf;
		BOOL b = GetOverlappedResult((HANDLE)sock, &overlap_recv, &nb, FALSE);
		if (nb == 0) {
			readerror(b);
		} else {
			rfifo_markwrite(rfifo, nb);
			InterlockedExchange(&lock_recv, 0);
			pump_send->havedata();
			bufferavail();
		}
	}

	volatile LONG lock_send;

	void havedata() {
		rfifo_t *rfifo = &pump_send->buf;
		DWORD nb = rfifo_availread(rfifo);
		if (nb != 0 || pump_send->eof) {
			if (0 == InterlockedExchange(&lock_send, 1)) {

				if (nb == 0) {
					shutdown(sock, SD_SEND);
					if (overlap_send.hEvent) evloop_removelistener(overlap_send.hEvent);
				} else {
					char *data = rfifo_pdata(rfifo);
					rfifo_markread(rfifo, nb);
					if (!overlap_send.hEvent) overlap_send.hEvent = evloop_addlistener(static_cast<PinSend*>(this));
					overlap_reset(&overlap_send);

					if (!WriteFile((HANDLE)sock, data, nb, &nb, &overlap_send) && GetLastError() != ERROR_IO_PENDING) {
						writeerror();
					}
				}
			}
		}
	}

	void writeerror() {
		char buf[100];
		struct sockaddr_in saddr;
		getpeer(buf, &saddr);
		pWin32Error(INFO, "peer %s:%d, write failed", buf, ntohs(saddr.sin_port));
		evloop_removelistener(overlap_send.hEvent);
		pump_recv->bufferavail();
	}

	void onEventSend() {
		DWORD nb;
		GetOverlappedResult((HANDLE)sock, &overlap_send, &nb, FALSE);
		if (nb == 0) {
			writeerror();
		} else {
			rfifo_t *rfifo = &pump_send->buf;
			rfifo_confirmread(rfifo, nb);
			InterlockedExchange(&lock_send, 0);
			pump_recv->bufferavail();
			havedata();
		}
	}

	TCPConnection(Tunnel *_tun, SOCKET _sock) :
			Connection(_tun),
			sock(_sock),
			firstread(0),
			lock_send(0),lock_recv(0)
	{
		overlap_send.hEvent = NULL;
		overlap_recv.hEvent = NULL;
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

struct TcpListener : SimpleRefcount, IEventPin {
	SOCKET lsock;
	SOCKET asock;
	OVERLAPPED overlap;
	char buf[ADDRESSLENGTH*2];
	ConnectionFactory *connfact;

	~TcpListener() {
		closesocket(lsock);
		closesocket(asock);
		delete connfact;
	}

	DeclRefcountMethods(SimpleRefcount::)

	void onEvent() {
		DWORD nb;
		struct sockaddr_in localSockaddr, remoteSockaddr;
		struct sockaddr_in *pLocalSockaddr, *pRemoteSockaddr;
		int lenL, lenR;

		GetOverlappedResult((HANDLE)lsock, &overlap, &nb, FALSE);

		GetAcceptExSockaddrs(buf, 0, ADDRESSLENGTH, ADDRESSLENGTH, (LPSOCKADDR*)&pLocalSockaddr, &lenL, (LPSOCKADDR*)&pRemoteSockaddr, &lenR);
		memcpy(&localSockaddr, pLocalSockaddr, sizeof(struct sockaddr_in));
		memcpy(&remoteSockaddr, pRemoteSockaddr, sizeof(struct sockaddr_in));
		{
			TCHAR buf[100];
			winet_inet_ntoa(remoteSockaddr.sin_addr, buf, 100);
			log(INFO, "accepted sock=%d, %s:%d", (int)asock, buf, ntohs(remoteSockaddr.sin_port));
			setsockopt(asock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&lsock, sizeof(lsock));
		}
		{
			TCPConnection *conn = new TCPConnection(NULL, asock);
			Tunnel *tun = conn->tun;
			connfact->connect(tun);
			tun->Release();
		}

		asock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		AcceptEx(lsock, asock, buf, 0, ADDRESSLENGTH, ADDRESSLENGTH, &nb, &overlap);
	}

};

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

			log(INFO, "connected sock=%d, %s:%hd", (int)conn->sock, conn->a.toresolv.hostname, conn->a.toresolv.port);

			conn->firstread = 1;

			HANDLE ev = conn->overlap_recv.hEvent = evloop_addlistener(static_cast<PinRecv*>(conn));
			SetEvent(ev);

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
	struct sockaddr_in saddr;
	TcpListener *listener;
	DWORD nb;
	SOCKET lsock, asock;

	listener = new TcpListener();
	listener->connfact = connfact;
	listener->lsock = lsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	listener->asock = asock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_addr.S_un.S_addr = INADDR_ANY;
	saddr.sin_port = htons((short int) port);
	saddr.sin_family = AF_INET;

	if (0 != bind(lsock, (const struct sockaddr *) &saddr, sizeof(saddr))) {
		pWinsockError(WARN, "failed to bind port %d", port);
		delete listener;
		return -2;
	}

	listen(lsock, LSN_BKLOG);

	listener->overlap.hEvent = evloop_addlistener(listener);
	overlap_reset(&listener->overlap);

	AcceptEx(lsock, asock, listener->buf, 0, ADDRESSLENGTH, ADDRESSLENGTH, &nb, &listener->overlap);
	log(INFO, "listening on port: %d\n", port);
	return 0;
}
