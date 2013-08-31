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

struct TCPConnection : Connection {
	void recv() {};
	void send() {};

	//TCPConnection(const char *host, short port) { abort(); }
	//TCPConnection(SOCKET _sock) : sock(_sock) {}

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
			winet_log(INFO, "accepted %s:%d\n", buf, ntohs(remoteSockaddr.sin_port));
		}

		{
			TCPConnection *conn = new TCPConnection();
			conn->sock = data->asock;
			Tunnel *tun = new Tunnel(conn);
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
		if (0 == connect(conn->sock, pres->ai_addr, pres->ai_addrlen)) {
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
		TCPConnection *conn = new TCPConnection();
		conn->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		strcpy(conn->a.toresolv.hostname, host);
		conn->a.toresolv.port = port;
		conn->tun = tun;

		tun->addref();
		CreateThread(NULL, 0, resolvethread, conn, 0, &tid);

		//WSAAsyncGetHostByName(
		//struct sockaddr_in addr;
		//if (!winet_inet_aton(host, &addr.sin_addr)) {
			//gethostbyname(NULL)->
		//}

		// addr.sin_addr.S_un.S_addr = 
		//unsigned long addr = inet_addr(host);

		//if (addr == INADDR_ANY)
		//	INADDR_NONE 
		//addr == INADDR_NONE || 
		//if (addr.sin_addr.S_un.S_addr 
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
