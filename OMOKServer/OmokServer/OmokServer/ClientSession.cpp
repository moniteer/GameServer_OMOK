#include "ClientSession.h"
#include <iostream>

cClientSession::cClientSession() // �ɺ� ���� �ʱ�ȭ
{	
	ZeroMemory( m_PacketBuff, sizeof( BUFFSIZE ) );
	ClientList.reserve( MAX_CLIENT );
	m_pstClientItem = nullptr;

	m_nConnectAllClientCount = 1;
	m_nNowConnectClientCount = 0;
	m_nAllRoomCount = 0;

	m_bWaitRoom = false;
};

cClientSession::~cClientSession() // vector �ʱ�ȭ
{
	if ( !ClientList.empty() )
	{
		ClientList.clear();
	}
}

int cClientSession::GetEmptyClientList() // [] �����ڷ� ������ Index ��ȯ �Լ�
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
