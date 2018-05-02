#include "WSASession.h"

cWSASession::cWSASession()
{

}

cWSASession::~cWSASession()
{

}

bool cWSASession::WSAStart() // 윈속 라이브러리 시작
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

void cWSASession::WSAClear() // 윈속 라이브러리 삭제
{
	std::cout << "[ Server ]  WSACleanup..." << std::endl;
	WSACleanup();
}
