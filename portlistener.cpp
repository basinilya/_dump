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

#ifdef _DEBUG

#undef INFO
#undef WARN
#undef ERR
#define INFO WINET_LOG_MESSAGE
#define WARN WINET_LOG_WARNING
#define ERR WINET_LOG_ERROR

void dbg_CloseHandle(const char *file, int line, HANDLE hObject) {
	if (!CloseHandle(hObject)) {
		pWin32Error(ERR, "CloseHandle() failed at %s:%d", file, line);
		abort();
	}
}
void dbg_GetOverlappedResult(const char *file, int line, HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait) {
	BOOL b = GetOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait);
	DWORD dw = GetLastError();
	winet_log(INFO, "GetOverlappedResult(hFile=%p(%lld), nb=%d, ev=%p) == %d; err = %d\n",
		(void*)hFile, (long long)hFile, *lpNumberOfBytesTransferred, (void*)lpOverlapped->hEvent, b, dw);
	SetLastError(dw);
	if (!b) {
		pWin32Error(ERR, "GetOverlappedResult() failed at %s:%d", file, line);
		abort();
	}
}
void dbg_WriteFile(const char *file, int line, HANDLE hFile,LPCVOID lpBuffer,DWORD nNumberOfBytesToWrite,LPDWORD lpNumberOfBytesWritten,LPOVERLAPPED lpOverlapped) {
	BOOL b = WriteFile(hFile,lpBuffer,nNumberOfBytesToWrite,lpNumberOfBytesWritten,lpOverlapped);
	DWORD dw = GetLastError();
	winet_log(INFO, "WriteFile(hFile=%p(%lld), bufsz=%d, nb=%d, ev=%p) == %d; err = %d\n",
		(void*)hFile, (long long)hFile, nNumberOfBytesToWrite, *lpNumberOfBytesWritten, (void*)lpOverlapped->hEvent, b, dw);
	if (!b && dw != ERROR_IO_PENDING) {
		pWin32Error(ERR, "WriteFile() failed at %s:%d", file, line);
		abort();
	}
	SetLastError(dw);
	//return b;
}
void dbg_ReadFile(const char *file, int line, HANDLE hFile,LPVOID lpBuffer,DWORD nNumberOfBytesToRead,LPDWORD lpNumberOfBytesRead,LPOVERLAPPED lpOverlapped) {
	BOOL b = ReadFile(hFile,lpBuffer,nNumberOfBytesToRead,lpNumberOfBytesRead,lpOverlapped);
	DWORD dw = GetLastError();
	winet_log(INFO, "ReadFile(hFile=%p(%lld), bufsz=%d, nb=%d, ev=%p) == %d; err = %d\n",
		(void*)hFile, (long long)hFile, nNumberOfBytesToRead, *lpNumberOfBytesRead, (void*)lpOverlapped->hEvent, b, dw);
	if (!b && dw != ERROR_IO_PENDING) {
		pWin32Error(ERR, "ReadFile() failed at %s:%d", file, line);
		abort();
	}
	SetLastError(dw);
	//return b;
}

void dbg_AcceptEx(const char *file, int line, SOCKET sListenSocket,SOCKET sAcceptSocket,PVOID lpOutputBuffer,DWORD dwReceiveDataLength,DWORD dwLocalAddressLength,DWORD dwRemoteAddressLength,LPDWORD lpdwBytesReceived,LPOVERLAPPED lpOverlapped) {
	BOOL b = AcceptEx(sListenSocket,sAcceptSocket,lpOutputBuffer,dwReceiveDataLength,dwLocalAddressLength,dwRemoteAddressLength,lpdwBytesReceived,lpOverlapped);
	DWORD dw = WSAGetLastError();
	if (!b && dw != ERROR_IO_PENDING) {
		pWinsockError(ERR, "AcceptEx() failed");
		abort();
	}
	WSASetLastError(dw);
}

void dbg_closesocket(const char *file, int line, SOCKET s) {
	if (closesocket(s) == SOCKET_ERROR) {
		pWinsockError(ERR, "closesocket() failed at %s:%d", file, line);
		abort();
	}
}
void dbg_shutdown(const char *file, int line, SOCKET s, int how) {
	int rc = shutdown(s, how);
	if (rc != 0) {
		pWinsockError(ERR, "shutdown(%d, SD_SEND) failed at %s:%d", (int)s, file, line);
		abort();
	}
}
SOCKET dbg_socket(const char *file, int line, int af,int type,int protocol) {
	SOCKET s = socket(af,type,protocol);
	if (s == INVALID_SOCKET) {
		pWinsockError(ERR, "socket() failed");
		abort();
	}
	return s;
}
#endif /* _DEBUG */

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
		if (nb != 0) {
			if (0 == InterlockedExchange(&lock_recv, 1)) {
				char *data = rfifo_pfree(rfifo);
				if (!overlap_recv.hEvent) overlap_recv.hEvent = evloop_addlistener(static_cast<PinRecv*>(this));
				overlap_reset(&overlap_recv);

				ReadFile((HANDLE)sock, data, nb, &nb, &overlap_recv);
			}
		}
	}

	void onEventRecv() {
		if (firstread) {
			firstread = 0;
			tun->connected();
			return;
		}
		DWORD nb;
		rfifo_t *rfifo = &pump_recv->buf;
		GetOverlappedResult((HANDLE)sock, &overlap_recv, &nb, FALSE);
		if (nb == 0) {
			/* keep locked */
			pump_recv->eof = 1;
			evloop_removelistener(overlap_recv.hEvent);
			pump_send->havedata();
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

struct data_accept : SimpleRefcount, IEventPin {
	SOCKET lsock;
	SOCKET asock;
	OVERLAPPED overlap;
	char buf[ADDRESSLENGTH*2];
	ConnectionFactory *connfact;

	~data_accept() {
		closesocket(lsock);
		closesocket(asock);
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

		data->asock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		AcceptEx(data->lsock, data->asock, data->buf, 0, ADDRESSLENGTH, ADDRESSLENGTH, &nb, &data->overlap);
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

			winet_log(INFO, "connected sock=%d, %s:%hd\n", (int)conn->sock, conn->a.toresolv.hostname, conn->a.toresolv.port);

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
	int rc = -1;
	struct sockaddr_in saddr;
	data_accept *newdata;
	DWORD nb;
	SOCKET lsock, asock;

	newdata = new data_accept();
	newdata->connfact = connfact;
	newdata->lsock = lsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	newdata->asock = asock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

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

	newdata->overlap.hEvent = evloop_addlistener(newdata);
	overlap_reset(&newdata->overlap);

	AcceptEx(lsock, asock, newdata->buf, 0, ADDRESSLENGTH, ADDRESSLENGTH, &nb, &newdata->overlap);
	winet_log(INFO, "[%s] listening on port: %d\n", WINET_APPNAME, port);
	return 0;
cleanup4:
	delete newdata;
	return rc;
}
