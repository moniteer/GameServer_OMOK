#pragma once

#include <WinSock2.h>
#include <queue>
#include <vector>
#include <list>
#include <algorithm>

#include "PacketSession.h"

#define BUFFSIZE				60	// Send 패킷 조립용 버퍼사이즈
#define MAX_CLIENT				50	// 최대 접속 가능 클라이언트 개수
#define MAX_PACKET_SIZE			128	// 패킷 RECV용 버퍼 사이즈


const char Key[] = "HongIk"; // XOR 암호화용 Key값


// Overlapped I/O 작업시 Send, Recv 구분용 공용체
enum Operation
{
	OP_RECV, OP_SEND
};


// 패킷 클래스 객채들을 선언해둔 구조체( 서버 기준 Send, Recv )
struct stClientPacketInfo
{			
	PacketHead				    m_cPacketHead;			// 패킷 헤더
	Login_Request			    m_cLoginRequest;		// 로그인 요청 Recv 패킷,							Recv[0]
	Login_Success				m_cLoginSuccess;		// 로그인 성공 Send 패킷,							Send[0]
	Login_Fail					m_cLoginFail;			// 로그인 실패 Send 패킷,							Send[1]
	Logout_Another				m_cLogoutAnother;		// 다른 클라이언트 로그아웃 Send 패킷,				Send[2]

	Create_User_Request			m_cCreateUserRequest;	// 로그인 ID, PW 생성 요청 Recv 패킷,				Recv[1]
	Create_User_Success			m_cCreateUserSuccess;	// 로그인 ID, PW 생성 성공 Send 패킷,				Send[3]
	Create_User_Fail			m_cCreateUserFail;		// 로그인 ID, PW 생성 실패 Send 패킷,				Send[4]

	Game_My_Stone_Put			m_cMyStonePut;			// 정상적인 턴 종료 Recv 패킷,						Recv[2]
	Game_Another_Stone_Put		m_cAnotherStonePut;		// 정상적인 턴 시작 Send 패킷,						Send[5]

	My_Game_Time_Out			m_cMyTimeOut;			// 타임 아웃 턴 종료 Recv 패킷,						Recv[3]
	Another_Game_Time_Out		m_cAnotherTimeOut;		// 타임 아웃 턴 시작 Send 패킷,						Send[6]

	Game_Start_Request			m_cStartRequest;		// 게임 시작 요청 Recv 패킷,						Recv[4]
	Game_Start_Success			m_cStartSuccess;		// 게임 시작 성공 Send 패킷,						Send[7]
	Game_Start_Fail				m_cStartFail;			// 게임 시작 실패 Send 패킷,						Send[8]
	Game_End					m_cEnd;					// 게임 종료 Recv 패킷,							Recv[5]

	Make_Room_Request			m_cMakeRoomRequest;		// 방 생성 요청 Recv 패킷,							Recv[6]
	Make_Room_Success			m_cMakeRoomSuccess;		// 방 생성 성공 Send 패킷,							Send[9]
	Make_Room_Fail				m_cMakeRoomFail;		// 방 생성 실패 Send 패킷,							Send[10]
	Another_Make_Room			m_cAnotherMakeRoom;		// 다른 클라이언트 방 생성 send 패킷,				Send[11]
	Already_Make_Room			m_cAlreadyMakeRoom;		// 이미 방이 생성되어 있음 send 패킷,				Send[12]
	 
	Join_Room_Request			m_cJoinRoomRequest;		// 방 참가 요청 Recv 패킷,							Recv[7]
	Join_Room_Success			m_cJoinRoomSuccess;		// 방 참가 성공 Send 패킷,							Send[13]
	Join_Room_Fail				m_cJoinRoomFail;		// 방 참가 실패 Send 패킷,							Send[14]
	Another_Join_Room			m_cAnotherJoinRoom;		// 다른 클라이언트 방 참가 통보 send 패킷,			Send[15]
	Already_Join_Room			m_cAlreadyJoinRoom;		// 이미 참가 되어있는 다른 클라이언트 통보 Send 패킷,	Send[16]

	My_Chat						m_cMyChat;				// 채팅 Recv 패킷,								Recv[8]
	Another_Chat				m_cAnotherchat;			// 채팅 Send 패킷,								Send[17]

	MakeDump_Error				m_cMakeDumpError;		// 에러 발생 유도 Recv 패킷,						Recv[9]
	Program_Shutdown			m_cProgramShutdown;		// 서버 다운 시 클라이언트들에게 종료통보 Send 패킷,	Send[18]

	Check_Alive_Request			m_cCheckAliveRequest;	// 상대 클라이언트 연결상태 확인 요청 Recv 패킷,		Recv[10]
	Check_Alive					m_cCheckAlive;			// 클라이언트 연결상태 확인 Send 패킷,				Send[19]
	Still_Alive					m_cStillAlive;			// 클라이언트가 연결되어있음을 알려주는 Recv 패킷,		Recv[11]
	Program_Quit				m_cProgramQuit;			// 클라이언트 종료시 서버로 종료통보 Recv 패킷,		Recv[12]

	Echo_Send					m_cEchoSend;			// 접속 테스트용 Recv 패킷
	Echo_Recv					m_cEchoRecv;			// 접속 테스트용 Send 패킷

};



// WSAOVERLAPPED 구조체를 포함하는 사용자 정의 구조체
struct stOverLapped_Item
{
	WSAOVERLAPPED		m_WSAOverlapped;					// Overlapped I/O용 구조체
	Operation			m_eOperation;						// Recv, Send 구분용 공용체
	WSABUF				m_WSABuff;							// WSABUF
	char				m_IOCPBuff[MAX_PACKET_SIZE];		// IOCP Send, Recv용 버퍼
	char				m_ParshingBuff[MAX_PACKET_SIZE];	// Recv받은 패킷을 조립할 버퍼
	char				m_DeCryptionBuff[MAX_PACKET_SIZE];	// XOR Decryption 용으로 사용할 버퍼
	char				m_EnCryptionBuff[MAX_PACKET_SIZE];  // XOR Encryption 용으로 사용할 버퍼
	int					m_nSuccessToSend;					// Send로 보내야 하는 패킷 크기 및 현재 보낸 크기
	int					m_nReceived;						// 현재 받은 총 데이터 크기 저장
};


// 클라이언트 정보를 담기 위한 사용자 정의 구조체
struct stClient_Item
{
	SOCKET				m_ClientSocket;			// 연결된 클라이언트 소켓
	DWORD				m_ClientID;				// 연결된 클라이언트 ID
	DWORD				m_dwRoomNumber;			// 접속한 방 번호
	std::string			m_IDString;				// 로그인 ID
	stOverLapped_Item	m_stRecvOverlapped;		// Recv 용 Overlapped I/O를 위한 구조체
	stOverLapped_Item	m_stSendOverlapped;		// Send 용 Overlapped I/O를 위한 구조체
	stClientPacketInfo  m_stClientPacketInfo;	// 패킷 정보를 모아둔 구조체

	// 초기화 실행
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
	std::list<std::vector<stClient_Item>> RoomList;		// 전체 Room 관리용 list
	std::vector<stClient_Item> Room;					// 개별적인 Room 관리용 vector


	std::vector<stClient_Item> ClientList;		// 접속된 모든 클라이언트 관리용 vector
	stClient_Item* m_pstClientItem;				// ClientList의 원소를 가리킬 포인터

	
	char m_PacketBuff[BUFFSIZE];				// 패킷 조립용 버퍼
	int m_nConnectAllClientCount;				// 현재까지 누적된 접속한 클라이언트 개수
	int m_nNowConnectClientCount;				// 지금 접속중인 클라이언트 개수
	int m_nAllRoomCount;						// 생성된 Room의 총 개수	
	bool m_bWaitRoom;							// 참가가능한 방의 존재유무 Flag값

public:
	cClientSession();
	~cClientSession();

	int GetEmptyClientList();	// vector에 추가할 클라이언트의 Index를 얻어오는 함수
	void CryptionPacket(char* Original, char* Cryption, const char* Key, int Size);	 // XOR En, Decryption 처리용 함수
};