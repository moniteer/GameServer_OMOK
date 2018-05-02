#pragma once

#include <WinSock2.h>
#include "NetworkSession.h"
#include "ClientSession.h"
#include "CriticalSection.h"
#include "Log.h"
#include "DataBaseSession.h"
#include "Exception.h"
#include "SystemStatus.h"
//#include "AutoDetectMemoryLeak.h"

#define MAX_THREAD_COUNT 5 // WorkerThread 생성 최대 개수

class cIOCPServer : public cClientSession
{
public:
	cLog m_cLog;							// Log를 남기기 위한 Log 클래스 객체
	cSystemStatus m_cSystemStatus;			// 시스템 자원 사용 현황 Log를 남기기 위한 객체
	cDataBaseSession m_cDataBaseSession;	// MySQL RDBMS 연동을 위한 객체
	cCriticalSection m_cCriticalSection;	// CriticalSection 을 사용하기 위한 CriticalSection 클래스 객체
	cNetworkSession* m_pCNetworkSession;	// 윈속 초기화 및 listen소켓 생성을 위한 NetworkSession 클래스 포인터 객체

	
	HANDLE m_hIOCP;								// IOCP를 생성하기 위한 HANDLE	
	HANDLE m_hAccepterThread;					// AcceptThread 생성용 HANDLE
	HANDLE m_hSystemStatusThread;				// SystemStatusThread 생성용 HANDLE
	HANDLE m_hWorkerThread[MAX_THREAD_COUNT];	// WorkerThread 생성용 HANDLE 배열
	

	DWORD m_dwAcceptThreadID;					// AcceptThread ThreadID 저장용
	DWORD m_dwSystemStatusThreadID;				// SystemStatusThread ThreadID 저장용
	DWORD m_dwWorkerThreadID[MAX_THREAD_COUNT];	// WorkerThread ThreadID 저장용
	


	bool m_AcceptRunning;		// AcceptThread 작동 while문 플래그
	bool m_WorkerRunning;		// WorkerThread 작동 while문 플래그
	bool m_SystemStatusRunning;	// SystemStatusThread 작동 while문 플래그

public:
	cIOCPServer();
	~cIOCPServer();

	void InitServer();	// Server 초기화 함수( 윈속 라이브러리, listen 소켓 셋팅 )
	bool StartServer(); // Server 시작 함수( IOCP 객체 생성 및 AcceptThread, WorkerThread 생성)
	void CloseServer(); // Server 종료 함수( 쓰레드 종료, IOCP 종료, 소켓 종료 )

		
	bool CreateWorkerThread();			// WorkerThread 생성 함수
	bool CreateAcceptThread();			// AcceptThread 생성 함수
	bool CreateSystemStatusThread();	// SystemStatusThread 생성함수


	void DestroyThread();									// Thread 삭제 함수
	void DeleteSocket(stClient_Item* pstClientItem);		// Socket 삭제 함수
	bool DisconnectProcess(stClient_Item* pstClientItem);	// Client 접속 끊는 함수	

		
	bool PacketstatisticsProcess(cIOCPServer* pcIOCPServer, stClient_Item* pstClientItem, PacketType Type, bool Result);	// 패킷 통계처리 하는 함수
	bool PacketProcess(cIOCPServer* pcIOCPServer, stClient_Item* pstClientItem, const char* pBuff);							// Packet Parshing 하는 함수
	bool ConnectIOCP(cIOCPServer* pcIOCPServer, stClient_Item* pstClientItem);												// IOCP에 ClientItem 연결시키는 함수 
	bool IOCPRecv(cIOCPServer* pcIOCPServer, stClient_Item* pstClientItem, int Received);									// WSARecv()
	bool IOCPSend(cIOCPServer* pcIOCPServer, stClient_Item* pstClientItem);													// WSASend()	
};