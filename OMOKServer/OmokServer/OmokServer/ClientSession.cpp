#include "ClientSession.h"
#include <iostream>

cClientSession::cClientSession() // 맴벼 변수 초기화
{	
	ZeroMemory( m_PacketBuff, sizeof( BUFFSIZE ) );
	ClientList.reserve( MAX_CLIENT );
	m_pstClientItem = nullptr;

	m_nConnectAllClientCount = 1;
	m_nNowConnectClientCount = 0;
	m_nAllRoomCount = 0;

	m_bWaitRoom = false;
};

cClientSession::~cClientSession() // vector 초기화
{
	if ( !ClientList.empty() )
	{
		ClientList.clear();
	}
}

int cClientSession::GetEmptyClientList() // [] 연산자로 참조할 Index 반환 함수
{
	if ( ClientList.empty() )
	{
		return 0;
	}
	else
	{
		return ClientList.size();
	}
}

void cClientSession::CryptionPacket(char* Original, char* Cryption, const char* Key, int Size)
{
	int KeySize = strlen(Key);
	int KeyCount = 0;

	for (int i = 0; i < Size; i++)
	{
		Cryption[i] = Original[i] ^ Key[KeyCount++];
		if (KeyCount == KeySize)
		{
			KeyCount = 0;
		}
	}
}
