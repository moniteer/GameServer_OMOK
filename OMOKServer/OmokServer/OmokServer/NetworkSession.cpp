#include "NetworkSession.h"
#include <iostream>

cNetworkSession::cNetworkSession()
{
	ListenSock = INVALID_SOCKET;
}

cNetworkSession::~cNetworkSession()
{

}

bool cNetworkSession::SetListenSocket()
{
	ListenSock = WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED );
	if ( ListenSock == INVALID_SOCKET )
	{
		std::cout << "[ Server ]  WSASocket() Failed, Error Code = " << WSAGetLastError() << std::endl;
		return false;
	}

	SOCKADDR_IN ServerAddr;
	ZeroMemory( &ServerAddr, sizeof( SOCKADDR_IN ) );
	ServerAddr.sin_addr.s_addr = htonl( INADDR_ANY );
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons( 9001 );

	int Ret = bind( ListenSock, (PSOCKADDR)&ServerAddr, sizeof( SOCKADDR_IN ) );
	if ( Ret == SOCKET_ERROR )
	{
		std::cout << "[ Server ]  bind() Failed, Error Code = " << GetLastError() << std::endl;
		closesocket( ListenSock );
		return false;
	}

	Ret = listen( ListenSock, 4 );
	if ( Ret == SOCKET_ERROR )
	{
		std::cout << "[ Server ]  listen() Failed, Error Code = " << GetLastError() << std::endl;
		closesocket( ListenSock );
		return false;
	}

	return true;
}

SOCKET cNetworkSession::GetListenSocket()
{
	return ListenSock;
}