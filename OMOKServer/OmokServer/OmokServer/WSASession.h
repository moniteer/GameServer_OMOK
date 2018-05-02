#pragma once

#include <WinSock2.h>
#include <iostream>

#pragma comment(lib,"ws2_32.lib")

class cWSASession
{
public:
	WSADATA wsa;

public:
	cWSASession();
	~cWSASession();
	
	bool WSAStart();
	void WSAClear();
};
