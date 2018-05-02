#pragma once

#include <WinSock2.h>
#include "WSASession.h"

class cNetworkSession : public cWSASession
{
public:
	cNetworkSession();
	~cNetworkSession();

	bool SetListenSocket();
	SOCKET GetListenSocket();
private:
	SOCKET ListenSock;
};

