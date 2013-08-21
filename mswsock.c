#if 0

#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>

static BOOL (PASCAL FAR *_AcceptEx)(
    IN SOCKET sListenSocket,
    IN SOCKET sAcceptSocket,
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT LPDWORD lpdwBytesReceived,
    IN LPOVERLAPPED lpOverlapped
    );
static VOID (PASCAL FAR *_GetAcceptExSockaddrs) (
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT struct sockaddr **LocalSockaddr,
    OUT LPINT LocalSockaddrLength,
    OUT struct sockaddr **RemoteSockaddr,
    OUT LPINT RemoteSockaddrLength
    );

static const GUID wsaid_acceptex = WSAID_ACCEPTEX;
static const GUID wsaid_getacceptexsockaddrs = WSAID_GETACCEPTEXSOCKADDRS;


static int init_AcceptEx()
{
	int rc;
	SOCKET s;
	DWORD nb;

	if (INVALID_SOCKET == (s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))) return -1;
	rc = 0;
	rc += WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, (void*)&wsaid_acceptex, sizeof(wsaid_acceptex)
		, &_AcceptEx, sizeof(_AcceptEx), &nb, NULL, NULL);
	rc += WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, (void*)&wsaid_getacceptexsockaddrs, sizeof(wsaid_getacceptexsockaddrs)
		, &_GetAcceptExSockaddrs, sizeof(_GetAcceptExSockaddrs), &nb, NULL, NULL);
	closesocket(s);
	return rc;
}

#endif
