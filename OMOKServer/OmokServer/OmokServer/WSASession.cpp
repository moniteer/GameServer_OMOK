#include "WSASession.h"

cWSASession::cWSASession()
{

}

cWSASession::~cWSASession()
{

}

bool cWSASession::WSAStart() // ���� ���̺귯�� ����
{
	int Ret = WSAStartup( MAKEWORD(2, 2), &wsa );
	if ( Ret == 0 )
	{
		return true;
	}
	else
	{
		return false;
	}
}

void cWSASession::WSAClear() // ���� ���̺귯�� ����
{
	std::cout << "[ Server ]  WSACleanup..." << std::endl;
	WSACleanup();
}
