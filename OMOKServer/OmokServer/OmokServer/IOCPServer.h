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

#define MAX_THREAD_COUNT 5 // WorkerThread ���� �ִ� ����

class cIOCPServer : public cClientSession
{
public:
	cLog m_cLog;							// Log�� ����� ���� Log Ŭ���� ��ü
	cSystemStatus m_cSystemStatus;			// �ý��� �ڿ� ��� ��Ȳ Log�� ����� ���� ��ü
	cDataBaseSession m_cDataBaseSession;	// MySQL RDBMS ������ ���� ��ü
	cCriticalSection m_cCriticalSection;	// CriticalSection �� ����ϱ� ���� CriticalSection Ŭ���� ��ü
	cNetworkSession* m_pCNetworkSession;	// ���� �ʱ�ȭ �� listen���� ������ ���� NetworkSession Ŭ���� ������ ��ü

	
	HANDLE m_hIOCP;								// IOCP�� �����ϱ� ���� HANDLE	
	HANDLE m_hAccepterThread;					// AcceptThread ������ HANDLE
	HANDLE m_hSystemStatusThread;				// SystemStatusThread ������ HANDLE
	HANDLE m_hWorkerThread[MAX_THREAD_COUNT];	// WorkerThread ������ HANDLE �迭
	

	DWORD m_dwAcceptThreadID;					// AcceptThread ThreadID �����
	DWORD m_dwSystemStatusThreadID;				// SystemStatusThread ThreadID �����
	DWORD m_dwWorkerThreadID[MAX_THREAD_COUNT];	// WorkerThread ThreadID �����
	


	bool m_AcceptRunning;		// AcceptThread �۵� while�� �÷���
	bool m_WorkerRunning;		// WorkerThread �۵� while�� �÷���
	bool m_SystemStatusRunning;	// SystemStatusThread �۵� while�� �÷���

public:
	cIOCPServer();
	~cIOCPServer();

	void InitServer();	// Server �ʱ�ȭ �Լ�( ���� ���̺귯��, listen ���� ���� )
	bool StartServer(); // Server ���� �Լ�( IOCP ��ü ���� �� AcceptThread, WorkerThread ����)
	void CloseServer(); // Server ���� �Լ�( ������ ����, IOCP ����, ���� ���� )

		
	bool CreateWorkerThread();			// WorkerThread ���� �Լ�
	bool CreateAcceptThread();			// AcceptThread ���� �Լ�
	bool CreateSystemStatusThread();	// SystemStatusThread �����Լ�


	void DestroyThread();									// Thread ���� �Լ�
	void DeleteSocket(stClient_Item* pstClientItem);		// Socket ���� �Լ�
	bool DisconnectProcess(stClient_Item* pstClientItem);	// Client ���� ���� �Լ�	

		
	bool PacketstatisticsProcess(cIOCPServer* pcIOCPServer, stClient_Item* pstClientItem, PacketType Type, bool Result);	// ��Ŷ ���ó�� �ϴ� �Լ�
	bool PacketProcess(cIOCPServer* pcIOCPServer, stClient_Item* pstClientItem, const char* pBuff);							// Packet Parshing �ϴ� �Լ�
	bool ConnectIOCP(cIOCPServer* pcIOCPServer, stClient_Item* pstClientItem);												// IOCP�� ClientItem �����Ű�� �Լ� 
	bool IOCPRecv(cIOCPServer* pcIOCPServer, stClient_Item* pstClientItem, int Received);									// WSARecv()
	bool IOCPSend(cIOCPServer* pcIOCPServer, stClient_Item* pstClientItem);													// WSASend()	
};