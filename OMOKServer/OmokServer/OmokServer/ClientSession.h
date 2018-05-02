#pragma once

#include <WinSock2.h>
#include <queue>
#include <vector>
#include <list>
#include <algorithm>

#include "PacketSession.h"

#define BUFFSIZE				60	// Send ��Ŷ ������ ���ۻ�����
#define MAX_CLIENT				50	// �ִ� ���� ���� Ŭ���̾�Ʈ ����
#define MAX_PACKET_SIZE			128	// ��Ŷ RECV�� ���� ������


const char Key[] = "HongIk"; // XOR ��ȣȭ�� Key��


// Overlapped I/O �۾��� Send, Recv ���п� ����ü
enum Operation
{
	OP_RECV, OP_SEND
};


// ��Ŷ Ŭ���� ��ä���� �����ص� ����ü( ���� ���� Send, Recv )
struct stClientPacketInfo
{			
	PacketHead				    m_cPacketHead;			// ��Ŷ ���
	Login_Request			    m_cLoginRequest;		// �α��� ��û Recv ��Ŷ,							Recv[0]
	Login_Success				m_cLoginSuccess;		// �α��� ���� Send ��Ŷ,							Send[0]
	Login_Fail					m_cLoginFail;			// �α��� ���� Send ��Ŷ,							Send[1]
	Logout_Another				m_cLogoutAnother;		// �ٸ� Ŭ���̾�Ʈ �α׾ƿ� Send ��Ŷ,				Send[2]

	Create_User_Request			m_cCreateUserRequest;	// �α��� ID, PW ���� ��û Recv ��Ŷ,				Recv[1]
	Create_User_Success			m_cCreateUserSuccess;	// �α��� ID, PW ���� ���� Send ��Ŷ,				Send[3]
	Create_User_Fail			m_cCreateUserFail;		// �α��� ID, PW ���� ���� Send ��Ŷ,				Send[4]

	Game_My_Stone_Put			m_cMyStonePut;			// �������� �� ���� Recv ��Ŷ,						Recv[2]
	Game_Another_Stone_Put		m_cAnotherStonePut;		// �������� �� ���� Send ��Ŷ,						Send[5]

	My_Game_Time_Out			m_cMyTimeOut;			// Ÿ�� �ƿ� �� ���� Recv ��Ŷ,						Recv[3]
	Another_Game_Time_Out		m_cAnotherTimeOut;		// Ÿ�� �ƿ� �� ���� Send ��Ŷ,						Send[6]

	Game_Start_Request			m_cStartRequest;		// ���� ���� ��û Recv ��Ŷ,						Recv[4]
	Game_Start_Success			m_cStartSuccess;		// ���� ���� ���� Send ��Ŷ,						Send[7]
	Game_Start_Fail				m_cStartFail;			// ���� ���� ���� Send ��Ŷ,						Send[8]
	Game_End					m_cEnd;					// ���� ���� Recv ��Ŷ,							Recv[5]

	Make_Room_Request			m_cMakeRoomRequest;		// �� ���� ��û Recv ��Ŷ,							Recv[6]
	Make_Room_Success			m_cMakeRoomSuccess;		// �� ���� ���� Send ��Ŷ,							Send[9]
	Make_Room_Fail				m_cMakeRoomFail;		// �� ���� ���� Send ��Ŷ,							Send[10]
	Another_Make_Room			m_cAnotherMakeRoom;		// �ٸ� Ŭ���̾�Ʈ �� ���� send ��Ŷ,				Send[11]
	Already_Make_Room			m_cAlreadyMakeRoom;		// �̹� ���� �����Ǿ� ���� send ��Ŷ,				Send[12]
	 
	Join_Room_Request			m_cJoinRoomRequest;		// �� ���� ��û Recv ��Ŷ,							Recv[7]
	Join_Room_Success			m_cJoinRoomSuccess;		// �� ���� ���� Send ��Ŷ,							Send[13]
	Join_Room_Fail				m_cJoinRoomFail;		// �� ���� ���� Send ��Ŷ,							Send[14]
	Another_Join_Room			m_cAnotherJoinRoom;		// �ٸ� Ŭ���̾�Ʈ �� ���� �뺸 send ��Ŷ,			Send[15]
	Already_Join_Room			m_cAlreadyJoinRoom;		// �̹� ���� �Ǿ��ִ� �ٸ� Ŭ���̾�Ʈ �뺸 Send ��Ŷ,	Send[16]

	My_Chat						m_cMyChat;				// ä�� Recv ��Ŷ,								Recv[8]
	Another_Chat				m_cAnotherchat;			// ä�� Send ��Ŷ,								Send[17]

	MakeDump_Error				m_cMakeDumpError;		// ���� �߻� ���� Recv ��Ŷ,						Recv[9]
	Program_Shutdown			m_cProgramShutdown;		// ���� �ٿ� �� Ŭ���̾�Ʈ�鿡�� �����뺸 Send ��Ŷ,	Send[18]

	Check_Alive_Request			m_cCheckAliveRequest;	// ��� Ŭ���̾�Ʈ ������� Ȯ�� ��û Recv ��Ŷ,		Recv[10]
	Check_Alive					m_cCheckAlive;			// Ŭ���̾�Ʈ ������� Ȯ�� Send ��Ŷ,				Send[19]
	Still_Alive					m_cStillAlive;			// Ŭ���̾�Ʈ�� ����Ǿ������� �˷��ִ� Recv ��Ŷ,		Recv[11]
	Program_Quit				m_cProgramQuit;			// Ŭ���̾�Ʈ ����� ������ �����뺸 Recv ��Ŷ,		Recv[12]

	Echo_Send					m_cEchoSend;			// ���� �׽�Ʈ�� Recv ��Ŷ
	Echo_Recv					m_cEchoRecv;			// ���� �׽�Ʈ�� Send ��Ŷ

};



// WSAOVERLAPPED ����ü�� �����ϴ� ����� ���� ����ü
struct stOverLapped_Item
{
	WSAOVERLAPPED		m_WSAOverlapped;					// Overlapped I/O�� ����ü
	Operation			m_eOperation;						// Recv, Send ���п� ����ü
	WSABUF				m_WSABuff;							// WSABUF
	char				m_IOCPBuff[MAX_PACKET_SIZE];		// IOCP Send, Recv�� ����
	char				m_ParshingBuff[MAX_PACKET_SIZE];	// Recv���� ��Ŷ�� ������ ����
	char				m_DeCryptionBuff[MAX_PACKET_SIZE];	// XOR Decryption ������ ����� ����
	char				m_EnCryptionBuff[MAX_PACKET_SIZE];  // XOR Encryption ������ ����� ����
	int					m_nSuccessToSend;					// Send�� ������ �ϴ� ��Ŷ ũ�� �� ���� ���� ũ��
	int					m_nReceived;						// ���� ���� �� ������ ũ�� ����
};


// Ŭ���̾�Ʈ ������ ��� ���� ����� ���� ����ü
struct stClient_Item
{
	SOCKET				m_ClientSocket;			// ����� Ŭ���̾�Ʈ ����
	DWORD				m_ClientID;				// ����� Ŭ���̾�Ʈ ID
	DWORD				m_dwRoomNumber;			// ������ �� ��ȣ
	std::string			m_IDString;				// �α��� ID
	stOverLapped_Item	m_stRecvOverlapped;		// Recv �� Overlapped I/O�� ���� ����ü
	stOverLapped_Item	m_stSendOverlapped;		// Send �� Overlapped I/O�� ���� ����ü
	stClientPacketInfo  m_stClientPacketInfo;	// ��Ŷ ������ ��Ƶ� ����ü

	// �ʱ�ȭ ����
	stClient_Item()
	{		
		m_ClientID = 0;
		m_IDString = "";
		m_dwRoomNumber = 0;
		m_ClientSocket = INVALID_SOCKET;

		ZeroMemory( &m_stRecvOverlapped.m_WSAOverlapped, sizeof(WSAOVERLAPPED) );
		ZeroMemory( &m_stRecvOverlapped.m_WSABuff, sizeof(WSABUF) );
		m_stRecvOverlapped.m_eOperation = OP_RECV;
		m_stRecvOverlapped.m_nSuccessToSend = 0;
		m_stRecvOverlapped.m_nReceived = 0;
				

		ZeroMemory( &m_stSendOverlapped.m_WSAOverlapped, sizeof(WSAOVERLAPPED) );
		ZeroMemory( &m_stSendOverlapped.m_WSABuff, sizeof(WSABUF) );
		m_stSendOverlapped.m_eOperation = OP_SEND;
		m_stSendOverlapped.m_nSuccessToSend = 0;
		m_stSendOverlapped.m_nReceived = 0;
	}
};





class cClientSession
{

public:	
	std::list<std::vector<stClient_Item>> RoomList;		// ��ü Room ������ list
	std::vector<stClient_Item> Room;					// �������� Room ������ vector


	std::vector<stClient_Item> ClientList;		// ���ӵ� ��� Ŭ���̾�Ʈ ������ vector
	stClient_Item* m_pstClientItem;				// ClientList�� ���Ҹ� ����ų ������

	
	char m_PacketBuff[BUFFSIZE];				// ��Ŷ ������ ����
	int m_nConnectAllClientCount;				// ������� ������ ������ Ŭ���̾�Ʈ ����
	int m_nNowConnectClientCount;				// ���� �������� Ŭ���̾�Ʈ ����
	int m_nAllRoomCount;						// ������ Room�� �� ����	
	bool m_bWaitRoom;							// ���������� ���� �������� Flag��

public:
	cClientSession();
	~cClientSession();

	int GetEmptyClientList();	// vector�� �߰��� Ŭ���̾�Ʈ�� Index�� ������ �Լ�
	void CryptionPacket(char* Original, char* Cryption, const char* Key, int Size);	 // XOR En, Decryption ó���� �Լ�
};