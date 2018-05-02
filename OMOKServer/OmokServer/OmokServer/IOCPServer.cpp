#include "IOCPServer.h"
#include "Exception.h"
#include <iostream>

DWORD WINAPI SystemStatusThreadProc(PVOID pParam)
{
	cIOCPServer* pcIOCPServer = (cIOCPServer*)pParam;	// 스레드 엔트리 함수 매개변수 받을 cIOCPServer 클래스 포인터
	
	__try
	{
		size_t CpuCount = 0;
		LONG* pArrayCore = nullptr;
		LONG Cpu = 0;

		pcIOCPServer->m_cSystemStatus.Init();

		CpuCount = pcIOCPServer->m_cSystemStatus.GetCPUCount();
		pArrayCore = new LONG[CpuCount];

		DWORD ThreadProcessData[15];
		ThreadProcessData[0] = pcIOCPServer->m_dwWorkerThreadID[0];
		ThreadProcessData[1] = pcIOCPServer->m_dwWorkerThreadID[1];
		ThreadProcessData[2] = pcIOCPServer->m_dwWorkerThreadID[2];
		ThreadProcessData[3] = pcIOCPServer->m_dwWorkerThreadID[3];
		ThreadProcessData[4] = pcIOCPServer->m_dwWorkerThreadID[4];
		ThreadProcessData[5] = pcIOCPServer->m_dwAcceptThreadID;
		ThreadProcessData[6] = pcIOCPServer->m_dwSystemStatusThreadID;
		ThreadProcessData[7] = (DWORD)pcIOCPServer->m_hWorkerThread[0];
		ThreadProcessData[8] = (DWORD)pcIOCPServer->m_hWorkerThread[1];
		ThreadProcessData[9] = (DWORD)pcIOCPServer->m_hWorkerThread[2];
		ThreadProcessData[10] = (DWORD)pcIOCPServer->m_hWorkerThread[3];
		ThreadProcessData[11] = (DWORD)pcIOCPServer->m_hWorkerThread[4];
		ThreadProcessData[12] = (DWORD)pcIOCPServer->m_hAccepterThread;
		ThreadProcessData[13] = (DWORD)pcIOCPServer->m_hSystemStatusThread;
		ThreadProcessData[14] = (DWORD)pcIOCPServer->m_hIOCP;

		DWORDLONG MemoryStatus[7];
		ZeroMemory(MemoryStatus, sizeof(DWORDLONG) * 7);

		pcIOCPServer->m_cSystemStatus.CreateLogFile(ThreadProcessData);

		while (pcIOCPServer->m_SystemStatusRunning)
		{
			Sleep(2000);

			pcIOCPServer->m_cSystemStatus.Update();
			pcIOCPServer->m_cSystemStatus.GetMemoryStatus(MemoryStatus);
			pcIOCPServer->m_cSystemStatus.GetCPUStatus(Cpu, pArrayCore, CpuCount);			
			pcIOCPServer->m_cSystemStatus.WriteLogFile(Cpu, pArrayCore, CpuCount, MemoryStatus);
		}

		pcIOCPServer->m_cSystemStatus.Terminate();

		delete[] pArrayCore;

		pArrayCore = nullptr;

	}
	__except (ExceptionFilter(GetExceptionInformation()))
	{
		std::cout << "[ Server ]  AcceptThreadProc Exception!" << std::endl;
		std::cout << "[ Server ]  Create Dump and System Shutdown" << std::endl;
	}	

	return 0;
}

DWORD WINAPI WorkerThreadProc(PVOID pParam)
{
	cIOCPServer* pcIOCPServer = (cIOCPServer*)pParam;	// 스레드 엔트리 함수 매개변수 받을 cIOCPServer 클래스 포인터
	stClient_Item* pstClientItem = nullptr;				// Completion Key 값 받을 pstClientItem 포인터
	BOOL GQCSSuccess = TRUE;							// GetQueuedCompletionStatus 리턴 값 받을 변수
	DWORD dwbyteSize = 0;								// Overlapped I/O로 전송 혹은 수신된 바이트 크기
	LPOVERLAPPED lpOverLapped = NULL;					// I/O 작업용으로 넘긴 Overlapped 구조체 받을 포인터

	__try
	{
		while (pcIOCPServer->m_WorkerRunning)
		{

			GQCSSuccess = GetQueuedCompletionStatus( pcIOCPServer->m_hIOCP,
													 &dwbyteSize,
													 (LPDWORD)&pstClientItem,
													 &lpOverLapped,
													 INFINITE );

			
			if ( GQCSSuccess == FALSE && dwbyteSize == 0  && pstClientItem != NULL ) // GetQueuedCompletionStatus 실패( 클라이언트 접속 해제 )
			{
				std::cout << "[ Server ]  GetQueueCompletionStatus return FALSE, Error = " << GetLastError() << std::endl;

				pcIOCPServer->m_cCriticalSection.Lock();
				pcIOCPServer->m_cLog.WriteLogFile( "GetQueueCompletionStatus return FALSE" );
				pcIOCPServer->m_cCriticalSection.UnLock();

				//  GQCS Error 리턴이 되서 추가해봄... -> 스레드 종료나 응용 프로그램 요청때문에 I/O 연산이 취소되었습니다. 
				if ((int)GetLastError() == 995)
				{
					continue;
				}

				if (GetLastError() == WSAECONNABORTED)
				{
					pcIOCPServer->DisconnectProcess(pstClientItem);
					continue;
				}				
			}

			if ( GQCSSuccess == TRUE && dwbyteSize == 0 && lpOverLapped == NULL ) // DestroyThread 호출로 인한 쓰레드 종료 
			{
				pcIOCPServer->m_WorkerRunning = false;
				continue;
			}

			stOverLapped_Item* pstOverLappedItem = (stOverLapped_Item*)lpOverLapped;

			if ( pstOverLappedItem->m_eOperation == OP_RECV ) // Overlapped I/O 작업에서 Recv인 경우
			{
				pcIOCPServer->CryptionPacket(pstOverLappedItem->m_IOCPBuff, pstOverLappedItem->m_DeCryptionBuff, Key, dwbyteSize);

				char* pRecvBuff = pstOverLappedItem->m_DeCryptionBuff;
				int nRecvedDataSize = dwbyteSize;
				int PacketSize = 0;

				if ( nRecvedDataSize != 0 )
				{
					PacketSize = (int)pRecvBuff[0]; // 수신되야할 패킷 사이즈
				}

				if ( nRecvedDataSize == 0 ) // 데이터가 전혀 Recv 되지 않았을때
				{
					bool NotRet = pcIOCPServer->IOCPRecv(pcIOCPServer, pstClientItem, NULL);
				}
				else if ( nRecvedDataSize + pstOverLappedItem->m_nReceived >= PacketSize ) // 데이터가 다 Recv 되었을때
				{
					CopyMemory( pstOverLappedItem->m_ParshingBuff, pRecvBuff, PacketSize );

					pstOverLappedItem->m_nReceived = 0; // 지금까지 받은 데이터 크기 초기화

					bool PackRet = pcIOCPServer->PacketProcess( pcIOCPServer, pstClientItem, pstOverLappedItem->m_ParshingBuff ); // PacketProcess 실행
					if ( PackRet == false )
					{
						std::cout << "[ Server ]  PacketProcess Failed..." << std::endl;

						pcIOCPServer->m_cCriticalSection.Lock();
						pcIOCPServer->m_cLog.WriteLogFile("PacketProcess Failed...");
						pcIOCPServer->m_cCriticalSection.UnLock();
					}

					bool RecvRet = pcIOCPServer->IOCPRecv(pcIOCPServer, pstClientItem, NULL);
				}
				else // 데이터가 일부만 Recv 되었을때
				{
					DWORD dwFlags = 0;
					pstOverLappedItem->m_nReceived += nRecvedDataSize;													// 받은 총 데이터 크기 저장
					bool RecvRet = pcIOCPServer->IOCPRecv(pcIOCPServer, pstClientItem, pstOverLappedItem->m_nReceived); // WSARecv 재호출
				}
			}

			else if ( pstOverLappedItem->m_eOperation == OP_SEND ) // Overlapped I/O 에서 Send 작업인 경우
			{
				int PacketSize = 0;

				pstOverLappedItem->m_nSuccessToSend -= dwbyteSize; // 원래 패킷 사이즈에서 실제 송신한 바이트 차이

				if ( pstOverLappedItem->m_nSuccessToSend > 0 ) // 일부 송신 완료
				{
					pstOverLappedItem->m_WSABuff.buf += dwbyteSize;
					pstOverLappedItem->m_WSABuff.len = pstOverLappedItem->m_nSuccessToSend;
					bool Ret = pcIOCPServer->IOCPSend(pcIOCPServer, pstClientItem);
				}
			}
			else
			{
				std::cout << "[ Server ]  None Define Packet..." << std::endl;

				pcIOCPServer->m_cCriticalSection.Lock();
				pcIOCPServer->m_cLog.WriteLogFile("None Define Packet...");
				pcIOCPServer->m_cCriticalSection.UnLock();

				continue;
			}
		}
	}
	__except ( ExceptionFilter( GetExceptionInformation() ) )
	{
		std::cout << "[ Server ]  WorkerThreadProc Exception!" << std::endl;
		std::cout << "[ Server ]  Create Dump and System Shutdown" << std::endl;
	}	

	return 0;
}

DWORD WINAPI AcceptThreadProc(PVOID pParam)
{
	/*__try
	{*/
		cIOCPServer* pcIOCPServer = (cIOCPServer*)pParam;

		SOCKADDR_IN ClientAddr;
		int nAddrLen = sizeof( SOCKADDR_IN );

		while ( pcIOCPServer->m_AcceptRunning )
		{
			int Index = pcIOCPServer->GetEmptyClientList();

			stClient_Item ClientItem;
			ClientItem.m_ClientSocket = accept( pcIOCPServer->m_pCNetworkSession->GetListenSocket(),
												(PSOCKADDR)&ClientAddr,
												&nAddrLen );

			if ( ClientItem.m_ClientSocket == INVALID_SOCKET )
			{
				std::cout << "[ Server ]  Accpet Failed, Error Code = " << GetLastError() << std::endl;

				pcIOCPServer->m_cCriticalSection.Lock();
				pcIOCPServer->m_cLog.WriteLogFile("Accpet Failed");
				pcIOCPServer->m_cCriticalSection.UnLock();

				continue;
			}

			if ( pcIOCPServer->m_nNowConnectClientCount >= 50 )
			{				
				std::cout << "[ Server ]  Client Connect is Limits, IP : " << inet_ntoa( ClientAddr.sin_addr ) << ", Port : " << ntohs( ClientAddr.sin_port ) << std::endl;

				pcIOCPServer->m_cCriticalSection.Lock();
				pcIOCPServer->m_cLog.WriteLogFile("Client Connect is Limits");
				pcIOCPServer->m_cCriticalSection.UnLock();

				pcIOCPServer->DeleteSocket( &ClientItem );
				continue;
			}
			else
			{
				std::cout << "[ Server ]  Client Connect Success, IP : " << inet_ntoa( ClientAddr.sin_addr ) << ", Port : " << ntohs( ClientAddr.sin_port ) << std::endl;
				
				std::string Message;
				Message = "Client Connect Success, IP : 127.0.0.1, Port : " + std::to_string(ntohs(ClientAddr.sin_port));

				pcIOCPServer->m_cCriticalSection.Lock();
				pcIOCPServer->m_cLog.WriteLogFile(const_cast<char*>(Message.c_str()));
				pcIOCPServer->m_cCriticalSection.UnLock();
				
				ClientItem.m_ClientID = pcIOCPServer->m_nConnectAllClientCount;
				pcIOCPServer->m_cLog.PacketStatistics.ConnectedClientID = pcIOCPServer->m_nConnectAllClientCount;

				pcIOCPServer->m_nConnectAllClientCount++;
				pcIOCPServer->m_nNowConnectClientCount++;

				pcIOCPServer->ClientList.push_back( ClientItem );
				pcIOCPServer->m_cLog.PacketStatisticsList.push_back( pcIOCPServer->m_cLog.PacketStatistics );
				
				for (auto iter = pcIOCPServer->ClientList.begin(); iter != pcIOCPServer->ClientList.end(); ++iter)
				{
					if (ClientItem.m_ClientID == iter->m_ClientID)
					{
						pcIOCPServer->m_pstClientItem = &(*iter);
						
						break;
					}
				}

				// pcIOCPServer->m_pstClientItem = &(pcIOCPServer->ClientList[Index]);

				bool bRet = pcIOCPServer->ConnectIOCP( pcIOCPServer, pcIOCPServer->m_pstClientItem );
				if ( bRet == false )
				{
					std::cout << "[ Server ]  ConnectIOCP() Failed... " << std::endl;
					std::cout << "[ Server ]  AcceptThreadProc Terminated... " << std::endl;
					return 0;
				}
			}
		}

	/*}
	__except ( ExceptionFilter( GetExceptionInformation() ) )
	{
		std::cout << "[ Server ]  AcceptThreadProc Exception!" << std::endl;
		std::cout << "[ Server ]  Create Dump and System Shutdown" << std::endl;
	}*/

	return 0;
}

cIOCPServer::cIOCPServer() // 맴버 변수들 초기화
{
	m_AcceptRunning = true;
	m_WorkerRunning = true;
	m_SystemStatusRunning = true;

	m_hAccepterThread = NULL;
	m_dwAcceptThreadID = 0;

	m_hSystemStatusThread = NULL;	
	m_dwSystemStatusThreadID = 0;

	for ( int i = 0; i < MAX_THREAD_COUNT; i++ )
	{
		m_hWorkerThread[i] = NULL;
		m_dwWorkerThreadID[i] = 0;
	}						
}

cIOCPServer::~cIOCPServer()
{
	if ( !ClientList.empty() )
	{
		ClientList.clear();
	}

	if ( !RoomList.empty() )
	{
		RoomList.clear();
	}

	delete m_pCNetworkSession;
}

void cIOCPServer::InitServer()
{
	m_pCNetworkSession = new cNetworkSession();

	bool bRet = m_pCNetworkSession->WSAStart();
	if ( bRet == false )
	{
		int Error = GetLastError();

		std::string ErrorString = "WSAStart() Failed, Error Code = " + std::to_string(Error);
		std::cout << "[ Server ]  " << ErrorString << std::endl;
		std::cout << "[ Server ]  Server Shutdown..." << std::endl;

		m_cCriticalSection.Lock();
		m_cLog.WriteLogFile(const_cast<char*>(ErrorString.c_str()));
		m_cCriticalSection.UnLock();

		CloseServer();
		return;
	}

	bRet = m_pCNetworkSession->SetListenSocket();
	if ( bRet == false )
	{
		int Error = GetLastError();

		std::string ErrorString = "SetListenSocket() Failed, Error Code = " + std::to_string(Error);
		std::cout << "[ Server ]  " << ErrorString << std::endl;
		std::cout << "[ Server ]  Server Shutdown..." << std::endl;

		m_cCriticalSection.Lock();
		m_cLog.WriteLogFile(const_cast<char*>(ErrorString.c_str()));
		m_cCriticalSection.UnLock();

		closesocket( m_pCNetworkSession->GetListenSocket() );
		CloseServer();
		return;
	}

	bRet = StartServer();
	if ( bRet == false )
	{
		int Error = GetLastError();

		std::string ErrorString = "StartServer() Failed, Error Code = " + std::to_string(Error);
		std::cout << "[ Server ]   " << ErrorString << std::endl;
		std::cout << "[ Server ]  Server Shutdown..." << std::endl;

		m_cCriticalSection.Lock();
		m_cLog.WriteLogFile(const_cast<char*>(ErrorString.c_str()));
		m_cCriticalSection.UnLock();

		closesocket( m_pCNetworkSession->GetListenSocket() );
		CloseServer();
		return;
	}

	m_cCriticalSection.Lock();
	m_cLog.WriteLogFile("Server Start Complete");
	m_cCriticalSection.UnLock();
}

bool cIOCPServer::StartServer()
{
	m_hIOCP = CreateIoCompletionPort( INVALID_HANDLE_VALUE,
									  NULL,
									  NULL,
									  0 );
	if ( m_hIOCP == NULL )
	{
		return false;
	}

	bool bRet = CreateWorkerThread();
	if ( bRet == false )
	{
		return false;
	}

	bRet = CreateAcceptThread();
	if ( bRet == false )
	{
		return false;
	}

	bRet = m_cDataBaseSession.StartMySQL();
	if ( bRet == false )
	{
		return false;
	}

	bRet = m_cLog.CreateLogFile();
	if (bRet == false)
	{
		return false;
	}

	bRet = CreateSystemStatusThread();
	if (bRet == false)
	{
		return false;
	}

	return true;
}

void cIOCPServer::CloseServer()
{
	DestroyThread(); // 쓰레드 종료

	int Size = ClientList.size();

	for ( int i = 0; i < Size; i++ ) // 접속한 모든 클라이언트 초기화
	{
		if ( ClientList[i].m_ClientSocket != INVALID_SOCKET )
		{
			DeleteSocket( &ClientList[i] );
		}
	}

	// RoomList에 있는 모든 클라이언트 초기화
	for (auto iter = RoomList.begin(); iter != RoomList.end(); ++iter)
	{
		for (auto it = iter->begin(); it != iter->end(); ++it)
		{
			if (it->m_ClientSocket != INVALID_SOCKET)
			{
				DeleteSocket(&(*it));
			}			
		}
	}

	m_pCNetworkSession->WSAClear(); // 윈속 라이브러리 종료

	m_cCriticalSection.Lock();
	m_cLog.WriteLogFile("Server Close Complete");
	m_cCriticalSection.UnLock();
}

bool cIOCPServer::ConnectIOCP(cIOCPServer* pcIOCPServer, stClient_Item* pstClientItem)
{
	HANDLE RetIOCP = CreateIoCompletionPort( (HANDLE)pcIOCPServer->ClientList.back().m_ClientSocket,
											 m_hIOCP,
											 reinterpret_cast<ULONG_PTR>(pstClientItem),
											 0 );

	if ( RetIOCP != m_hIOCP || RetIOCP == NULL )
	{
		int Error = GetLastError();

		std::string ErrorString = "IOCP Connect Failed, Error Code = " + std::to_string(Error);

		std::cout << "[ Server ]  " << ErrorString << std::endl;		

		m_cCriticalSection.Lock();
		m_cLog.WriteLogFile( const_cast<char*>(ErrorString.c_str()) );
		m_cCriticalSection.UnLock();

		return false;
	}

	m_cCriticalSection.Lock();
	m_cLog.WriteLogFile("Client Connect IOCP Complete");
	m_cCriticalSection.UnLock();

	bool Ret = IOCPRecv( pcIOCPServer, pstClientItem, NULL );
	if ( Ret == false )
	{
		return false;
	}

	m_cCriticalSection.Lock();
	m_cLog.WriteLogFile("Asynchronous Overlapped I/O Start Complete");
	m_cCriticalSection.UnLock();

	return true;
}

bool cIOCPServer::IOCPRecv(cIOCPServer* pcIOCPServer, stClient_Item* pstClientItem, int Received)
{
	DWORD dwFlags = 0;

	if ( pstClientItem->m_ClientSocket == INVALID_SOCKET || pcIOCPServer == NULL )
	{
		int Error = GetLastError();

		std::string ErrorString = "Client Socket Use Failed, Error code = " + std::to_string(Error);
		std::cout << "[ Server ]  " << ErrorString << std::endl;

		m_cCriticalSection.Lock();
		m_cLog.WriteLogFile(const_cast<char*>(ErrorString.c_str()));
		m_cCriticalSection.UnLock();

		return false;
	}

	pstClientItem->m_stRecvOverlapped.m_WSABuff.buf = (char*)pstClientItem->m_stRecvOverlapped.m_IOCPBuff + Received;
	pstClientItem->m_stRecvOverlapped.m_WSABuff.len = MAX_PACKET_SIZE - Received;

	ZeroMemory( &pstClientItem->m_stRecvOverlapped.m_WSAOverlapped, sizeof( WSAOVERLAPPED ) );

	int RetRecv = WSARecv( pstClientItem->m_ClientSocket,
						   &pstClientItem->m_stRecvOverlapped.m_WSABuff,
						   1,
						   NULL,
						   &dwFlags,
						   (LPOVERLAPPED)&pstClientItem->m_stRecvOverlapped,
						   NULL );

	if ( RetRecv == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING )
	{
		int Error = WSAGetLastError();

		std::string ErrorString = "WSARecv Failed, Error code = " + std::to_string(Error);
		std::cout << "[ Server ]  " << ErrorString << std::endl;

		m_cCriticalSection.Lock();
		m_cLog.WriteLogFile(const_cast<char*>(ErrorString.c_str()));
		m_cCriticalSection.UnLock();

		// 이미 닫은 소켓에 대해 에러가 리턴될때 처리
		if (Error == WSAENOTSOCK)
		{
			goto $Label15;
		}
		// 클라이언트 끊길 경우 처리( Room에 있는 다른 클라이언트에게 종료 패킷 발송 Disconnect 처리, 끊긴 클라이언트도 Disconnect 처리 )
		// RoomList에서 해당 Room 제거, ClientList에서 제거
		else if (Error == WSAECONNABORTED)
		{
			
			for (auto iter = RoomList.begin(); iter != RoomList.end(); ++iter)
			{
				for (auto it = iter->begin(); it != iter->end(); ++it)
				{
					if ((it->m_dwRoomNumber == pstClientItem->m_dwRoomNumber) && (it->m_ClientID != pstClientItem->m_ClientID))
					{
						it->m_stClientPacketInfo.m_cProgramShutdown = Program_Shutdown(DISCONNECT_ANOTHER_CODE);

						//  m_EnCryptionBuff에 1차적으로 Copy
						CopyMemory(it->m_stSendOverlapped.m_EnCryptionBuff, &it->m_stClientPacketInfo.m_cProgramShutdown, sizeof(it->m_stClientPacketInfo.m_cProgramShutdown));

						//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
						CryptionPacket(it->m_stSendOverlapped.m_EnCryptionBuff, it->m_stSendOverlapped.m_IOCPBuff, Key, sizeof(it->m_stClientPacketInfo.m_cProgramShutdown));

						it->m_stSendOverlapped.m_nSuccessToSend = sizeof(it->m_stClientPacketInfo.m_cProgramShutdown);
						it->m_stSendOverlapped.m_WSABuff.buf = (char*)it->m_stSendOverlapped.m_IOCPBuff;
						it->m_stSendOverlapped.m_WSABuff.len = sizeof(it->m_stClientPacketInfo.m_cProgramShutdown);

						bool Ret = IOCPSend(pcIOCPServer, &(*it));
						if (Ret == false)
						{
							int Error = GetLastError();

							std::string ErrorString = "Send 'Program Shutdown' Packet Failed" + std::to_string(Error);
							std::cout << "[ Server ]  " << ErrorString << std::endl;

							m_cCriticalSection.Lock();
							m_cLog.WriteLogFile(const_cast<char*>(ErrorString.c_str()));
							m_cCriticalSection.UnLock();

							bool Counting = PacketstatisticsProcess(pcIOCPServer, &(*it), PROGRAM_SHUTDOWN, FALSE);
							if (Counting == false)
							{
								std::cout << "[ Server ]  Did Not Counting 'PROGRAM_SHUTDOWN' Packet" << std::endl;
							}
						}
						else
						{
							std::cout << "[ Server ]  Send 'Program Shutdown' Packet Complete" << std::endl;

							m_cCriticalSection.Lock();
							m_cLog.WriteLogFile("Program Shutdown Packet WSASend");
							m_cCriticalSection.UnLock();

							bool Counting = PacketstatisticsProcess(pcIOCPServer, &(*it), PROGRAM_SHUTDOWN, TRUE);
							if (Counting == false)
							{
								std::cout << "[ Server ]  Did Not Counting 'PROGRAM_SHUTDOWN' Packet" << std::endl;
							}
						}

						//  접속 끊긴 클라이언트 및 같은 방에 있던 클라이언트 RoomList, ClientLst 에서 제거
						pcIOCPServer->DisconnectProcess(pstClientItem); 
						pcIOCPServer->DisconnectProcess(&(*it));

						goto $Label10;
					}
				}
			}

			$Label10:			

			return true;
		}

		return false;
	}

	$Label15:

	return true;
}

bool cIOCPServer::IOCPSend(cIOCPServer* pcIOCPServer, stClient_Item* pstClientItem)
{
	if ( pstClientItem == NULL ||
		 pstClientItem->m_ClientSocket == INVALID_SOCKET ||
		 pstClientItem->m_stSendOverlapped.m_WSABuff.buf == NULL ||
		 pstClientItem->m_stSendOverlapped.m_WSABuff.len <= 0 )
	{
		return false;
	}

	ZeroMemory( &pstClientItem->m_stSendOverlapped.m_WSAOverlapped, sizeof( WSAOVERLAPPED ) );

	int RetSend = WSASend( pstClientItem->m_ClientSocket,
						   &pstClientItem->m_stSendOverlapped.m_WSABuff,
						   1,
						   NULL,
						   0,
						   (LPWSAOVERLAPPED)&pstClientItem->m_stSendOverlapped.m_WSAOverlapped,
						   NULL );

	if ( RetSend == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING )
	{
		int Error = WSAGetLastError();

		std::string ErrorString = "WSASend Failed, Error code = " + std::to_string(Error);
		std::cout << "[ Server ]  " << ErrorString << std::endl;

		m_cCriticalSection.Lock();
		m_cLog.WriteLogFile(const_cast<char*>(ErrorString.c_str()));
		m_cCriticalSection.UnLock();

		return false;
	}

	return true;
}

bool cIOCPServer::CreateSystemStatusThread()
{
	m_hSystemStatusThread = CreateThread( NULL,
										  0,
										  SystemStatusThreadProc,
										  this,
										  CREATE_SUSPENDED,
										  &m_dwSystemStatusThreadID );

	if ( m_hSystemStatusThread == NULL )
	{
		int Error = GetLastError();

		std::string ErrorString = "SystemStatusThread Create Failed, Error Code = " + std::to_string(Error);
		std::cout << "[ Server ]  " << ErrorString << std::endl;

		m_cCriticalSection.Lock();
		m_cLog.WriteLogFile( const_cast<char*>(ErrorString.c_str()) );
		m_cCriticalSection.UnLock();

		return false;
	}
	else
	{
		std::cout << "[ Server ]  SystemStatusThread Create Success! " << std::endl;
		ResumeThread(m_hSystemStatusThread);
	}

	m_cCriticalSection.Lock();
	m_cLog.WriteLogFile("SystemStatus Thread Create complete");
	m_cCriticalSection.UnLock();

	return true;
}

bool cIOCPServer::CreateAcceptThread()
{
	m_hAccepterThread = CreateThread( NULL, 
									  0,
									  AcceptThreadProc,
									  this,
									  CREATE_SUSPENDED,
									  &m_dwAcceptThreadID );
	if ( m_hAccepterThread == NULL )
	{
		int Error = GetLastError();
		
		std::string ErrorString = "AcceptThread Create Failed, Error Code = " + std::to_string(Error);
		std::cout << "[ Server ]  " << ErrorString << std::endl;

		m_cCriticalSection.Lock();
		m_cLog.WriteLogFile(const_cast<char*>(ErrorString.c_str()));
		m_cCriticalSection.UnLock();

		return false;
	}
	else
	{
		std::cout << "[ Server ]  AcceptThread Create Success! " << std::endl;
		ResumeThread( m_hAccepterThread );
	}

	m_cCriticalSection.Lock();
	m_cLog.WriteLogFile("Accept Thread Create complete");
	m_cCriticalSection.UnLock();

	return true;
}

bool cIOCPServer::CreateWorkerThread()
{
	for ( int i = 0; i < MAX_THREAD_COUNT; i++ )
	{
		m_hWorkerThread[i] = CreateThread( NULL,
										   0,
										   WorkerThreadProc,
										   this,
										   CREATE_SUSPENDED, 
										   &m_dwWorkerThreadID[i] );
		if ( m_hWorkerThread[i] == NULL )
		{	
			int Error = GetLastError();

			std::string ErrorString = " WorkerThread Create Failed, Error Code = " + std::to_string(Error);
			std::cout << "[ Server ] " << ErrorString << std::endl;

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile(const_cast<char*>(ErrorString.c_str()));
			m_cCriticalSection.UnLock();

			return false;
		}
		else
		{
			std::string SuccessString = "N0." + std::to_string(i + 1) + " WorkerThread Create Success!";
			std::cout << "[ Server ]  " << SuccessString << std::endl;

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile(const_cast<char*>(SuccessString.c_str()));
			m_cCriticalSection.UnLock();

			ResumeThread( m_hWorkerThread[i] );
		}
	}

	m_cCriticalSection.Lock();
	m_cLog.WriteLogFile("All Worker Thread Create complete");
	m_cCriticalSection.UnLock();

	return true;
}

bool cIOCPServer::DisconnectProcess(stClient_Item* pstClientItem)
{
	// Accept만 처리되고 RoomList엔 처리되지 않은 클라이언트( ClientList에서 해당 클라이언트만 제거 )
	if (pstClientItem->m_dwRoomNumber == 0)
	{
		stClient_Item DisconnectClient;
		SOCKADDR_IN DisconnectClientAddr;
		int DisconnectClientAddrLen = sizeof(SOCKADDR_IN);

		DisconnectClient = *pstClientItem;

		getpeername(DisconnectClient.m_ClientSocket, (PSOCKADDR)&DisconnectClientAddr, &DisconnectClientAddrLen);
		
		for (auto iter = ClientList.begin(); iter != ClientList.end(); ++iter)
		{
			if (iter->m_ClientID == DisconnectClient.m_ClientID)
			{				
				ClientList.erase(iter);				

				break;
			}
		}

		m_nNowConnectClientCount--;
		
		DeleteSocket(&DisconnectClient);
		
		std::cout << "[ Server ]  Client Disconnect, IP : " << inet_ntoa(DisconnectClientAddr.sin_addr) << ", Port : " << ntohs(DisconnectClientAddr.sin_port) << std::endl;
		std::cout << "[ Server ]  ClientList in Disconnected No." << DisconnectClient.m_ClientID << " Client Erase complet" << std::endl;

		std::string Message;
		Message = "Client Disconnect, IP : 127.0.0.1, Port : " + std::to_string(ntohs(DisconnectClientAddr.sin_port));
		std::string Message1;
		Message1 = "ClientList in Disconnected No." + std::to_string(DisconnectClient.m_ClientID) + " Client Erase complete";

		m_cCriticalSection.Lock();
		m_cLog.WriteLogFile(const_cast<char*>(Message.c_str()));
		m_cLog.WriteLogFile(const_cast<char*>(Message1.c_str()));
		m_cCriticalSection.UnLock();
	}
	// RoomList에 추가되어있는 클라이언트( ClientList, RoomList에서 해당 클라이언트 제거 - Room에 혼자 있었으면 Room도 삭제 )
	else
	{
		SOCKADDR_IN DisconnectClientAddr;

		stClient_Item DisconnectClient;

		bool bErase = false;

		DisconnectClient = *pstClientItem;

		int DisconnectClientAddrLen = sizeof(SOCKADDR_IN);	
		
		for (auto iter = RoomList.begin(); iter != RoomList.end(); ++iter)
		{
			for (auto it = iter->begin(); it != iter->end(); ++it)
			{
				if ((it->m_dwRoomNumber == DisconnectClient.m_dwRoomNumber) && (it->m_ClientID == DisconnectClient.m_ClientID))
				{
					if (iter->size() == 2)
					{
						iter->erase(it);
					}
					else
					{
						bErase = true;
						iter->clear();
					}					

					goto $Label11;
				}
			}
		}

		$Label11:

		std::cout << "[ Server ]  RoomList in Disconnected No." << DisconnectClient.m_ClientID << " Client Erase complete" << std::endl;

		std::string Message2;
		Message2 = "RoomList in Disconnected No." + std::to_string(DisconnectClient.m_ClientID) + " Client Erase complete";

		m_cCriticalSection.Lock();
		m_cLog.WriteLogFile(const_cast<char*>(Message2.c_str()));
		m_cCriticalSection.UnLock();

		if (bErase == true)
		{
			std::cout << "[ Server ]  RoomList in Disconnected No." << DisconnectClient.m_dwRoomNumber <<  " Room Erase complete" << std::endl;

			std::string Message3;
			Message3 = "RoomList in Disconnected No." + std::to_string(DisconnectClient.m_dwRoomNumber) + " Room Erase complete";

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile(const_cast<char*>(Message3.c_str()));
			m_cCriticalSection.UnLock();

			bErase = false;
		}		

		for (auto iter = ClientList.begin(); iter != ClientList.end(); ++iter)
		{
			if (iter->m_ClientID == DisconnectClient.m_ClientID)
			{
				ClientList.erase(iter);
				m_nNowConnectClientCount--;

				break;
			}
		}

		getpeername(DisconnectClient.m_ClientSocket, (PSOCKADDR)&DisconnectClientAddr, &DisconnectClientAddrLen);

		DeleteSocket(&DisconnectClient);

		std::cout << "[ Server ]  Client Disconnect, IP : " << inet_ntoa(DisconnectClientAddr.sin_addr) << ", Port : " << ntohs(DisconnectClientAddr.sin_port) << std::endl;
		std::cout << "[ Server ]  ClientList in Disconnected No." << DisconnectClient .m_ClientID << " Client Erase complete" << std::endl;

		std::string Message4;
		Message4 = "Client Disconnect, IP : 127.0.0.1, Port : " + std::to_string(ntohs(DisconnectClientAddr.sin_port));
		std::string Message5;
		Message5 = "ClientList in Disconnected No." + std::to_string(DisconnectClient.m_ClientID) + " Client Erase complete";

		m_cCriticalSection.Lock();
		m_cLog.WriteLogFile(const_cast<char*>(Message4.c_str()));
		m_cLog.WriteLogFile(const_cast<char*>(Message5.c_str()));
		m_cCriticalSection.UnLock();

	}
	
	return true;
}

bool cIOCPServer::PacketProcess(cIOCPServer* pcIOCPServer, stClient_Item* pstClientItem, const char* pBuff)
{
	// parshingBuff를 가르키는 pBuff( IOCP버퍼로 수신된 데이터를 처리를 위해 parching버퍼로 복사해둠 )
	int PacketSize = (int)pBuff[0];

	CopyMemory( m_PacketBuff, pBuff, PacketSize );
	CopyMemory( &pstClientItem->m_stClientPacketInfo.m_cPacketHead, m_PacketBuff, sizeof( pstClientItem->m_stClientPacketInfo.m_cPacketHead ) );
	
	PacketType RecvPacketType = pstClientItem->m_stClientPacketInfo.m_cPacketHead.Get_PacketHeader();

	switch ( RecvPacketType )
	{
		case CREATE_USER_REQUEST:
		{
			std::cout << "[ Server ]  N0." << pstClientItem->m_ClientID << " Client Send 'Create User Request' Packet" << std::endl;

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile("'Create User Request' Packet WSARecv");
			m_cCriticalSection.UnLock();

			bool Counting = PacketstatisticsProcess( pcIOCPServer, pstClientItem, CREATE_USER_REQUEST, TRUE );
			if (Counting == false)
			{
				std::cout << "[ Server ]  Did Not Counting 'CREATE_USER_REQUEST' Packet" << std::endl;
			}

			CopyMemory( &pstClientItem->m_stClientPacketInfo.m_cCreateUserRequest, m_PacketBuff, sizeof( pstClientItem->m_stClientPacketInfo.m_cCreateUserRequest ) );

			bool CreateCheck = m_cDataBaseSession.CreateUser( pstClientItem->m_stClientPacketInfo.m_cCreateUserRequest.ID_value, pstClientItem->m_stClientPacketInfo.m_cCreateUserRequest.PW_value );
		
			if ( CreateCheck == true )
			{
				// 요청한 클라이언트에게 CreateSuccess 세팅 후 WSASend
				pstClientItem->m_stClientPacketInfo.m_cCreateUserSuccess = Create_User_Success( CREATE_USER_SUCCESS_CODE );

				//  m_EnCryptionBuff에 1차적으로 Copy
				CopyMemory( pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, &pstClientItem->m_stClientPacketInfo.m_cCreateUserSuccess, sizeof( pstClientItem->m_stClientPacketInfo.m_cCreateUserSuccess ) );

				//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
				CryptionPacket( pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, pstClientItem->m_stSendOverlapped.m_IOCPBuff, Key, sizeof( pstClientItem->m_stClientPacketInfo.m_cCreateUserSuccess ) );

				pstClientItem->m_stSendOverlapped.m_nSuccessToSend = sizeof( pstClientItem->m_stClientPacketInfo.m_cCreateUserSuccess );
				pstClientItem->m_stSendOverlapped.m_WSABuff.buf = (char*)pstClientItem->m_stSendOverlapped.m_IOCPBuff;
				pstClientItem->m_stSendOverlapped.m_WSABuff.len = sizeof( pstClientItem->m_stClientPacketInfo.m_cCreateUserSuccess );

				bool Ret = IOCPSend( pcIOCPServer, pstClientItem );
				if ( Ret == false )
				{
					std::cout << "[ Server ]  Send 'Create User Success' Packet Failed" << std::endl;

					m_cCriticalSection.Lock();
					m_cLog.WriteLogFile("'Create User Success' Packet Failed");
					m_cCriticalSection.UnLock();

					bool Counting = PacketstatisticsProcess( pcIOCPServer, pstClientItem, CREATE_USER_SUCCESS, FALSE );
					if ( Counting == false )
					{
						std::cout << "[ Server ]  Did Not Counting 'CREATE_USER_SUCCESS' Packet" << std::endl;
					}
				}
				else
				{
					std::cout << "[ Server ]  Send 'Create User Success' Packet Complete" << std::endl;

					m_cCriticalSection.Lock();
					m_cLog.WriteLogFile("'Create User Success' Packet WSASend");
					m_cCriticalSection.UnLock();

					bool Counting = PacketstatisticsProcess( pcIOCPServer, pstClientItem, CREATE_USER_SUCCESS, TRUE );
					if ( Counting == false )
					{
						std::cout << "[ Server ]  Did Not Counting 'CREATE_USER_SUCCESS' Packet" << std::endl;
					}
				}
			}
			else
			{
				// 요청한 클라이언트에게 CreateUserFail 세팅 후 WSASend
				pstClientItem->m_stClientPacketInfo.m_cCreateUserFail = Create_User_Fail( CREATE_USER_FAIL_CODE );

				//  m_EnCryptionBuff에 1차적으로 Copy
				CopyMemory( pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, &pstClientItem->m_stClientPacketInfo.m_cCreateUserFail, sizeof( pstClientItem->m_stClientPacketInfo.m_cCreateUserFail ) );

				//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
				CryptionPacket( pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, pstClientItem->m_stSendOverlapped.m_IOCPBuff, Key, sizeof( pstClientItem->m_stClientPacketInfo.m_cCreateUserFail ) );

				pstClientItem->m_stSendOverlapped.m_nSuccessToSend = sizeof( pstClientItem->m_stClientPacketInfo.m_cCreateUserFail );
				pstClientItem->m_stSendOverlapped.m_WSABuff.buf = (char*)pstClientItem->m_stSendOverlapped.m_IOCPBuff;
				pstClientItem->m_stSendOverlapped.m_WSABuff.len = sizeof( pstClientItem->m_stClientPacketInfo.m_cCreateUserFail );

				bool Ret = IOCPSend( pcIOCPServer, pstClientItem );
				if ( Ret == false )
				{
					std::cout << "[ Server ]  Send 'Create User Fail' Packet Failed" << std::endl;

					m_cCriticalSection.Lock();
					m_cLog.WriteLogFile("'Create User Fail' Packet Failed");
					m_cCriticalSection.UnLock();

					bool Counting = PacketstatisticsProcess( pcIOCPServer, pstClientItem, CREATE_USER_FAIL, FALSE );
					if ( Counting == false )
					{
						std::cout << "[ Server ]  Did Not Counting 'CREATE_USER_FAIL' Packet" << std::endl;
					}
				}
				else
				{
					std::cout << "[ Server ]  Send 'Create User Fail' Packet Complete" << std::endl;

					m_cCriticalSection.Lock();
					m_cLog.WriteLogFile("'Create User Fail' Packet WSASend");
					m_cCriticalSection.UnLock();

					bool Counting = PacketstatisticsProcess( pcIOCPServer, pstClientItem, CREATE_USER_FAIL, TRUE );
					if ( Counting == false )
					{
						std::cout << "[ Server ]  Did Not Counting 'CREATE_USER_FAIL' Packet" << std::endl;
					}
				}
			}
		
			ZeroMemory( m_PacketBuff, sizeof( BUFFSIZE ) );

			break;				
		}

		case LOGIN_REQUEST:
		{
			std::cout << "[ Server ]  N0." << pstClientItem->m_ClientID << " Client Send 'Login Request' Packet" << std::endl;

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile("'Login Request' Packet WSARecv");
			m_cCriticalSection.UnLock();

			bool Counting = PacketstatisticsProcess( pcIOCPServer, pstClientItem, LOGIN_REQUEST, TRUE );
			if (Counting == false)
			{
				std::cout << "[ Server ]  Did Not Counting 'LOGIN_REQUEST' Packet" << std::endl;
			}

			CopyMemory( &pstClientItem->m_stClientPacketInfo.m_cLoginRequest, m_PacketBuff, sizeof( pstClientItem->m_stClientPacketInfo.m_cLoginRequest ) );

			bool LoginCheck = m_cDataBaseSession.CheckUser( pstClientItem->m_stClientPacketInfo.m_cLoginRequest.ID_value, pstClientItem->m_stClientPacketInfo.m_cLoginRequest.PW_value );
		
			if ( LoginCheck == true )
			{
				// 클라이언트 로그인 ID 저장
				pstClientItem->m_IDString = pstClientItem->m_stClientPacketInfo.m_cLoginRequest.ID_value;

				// 요청한 클라이언트에게 LoginSuccess 세팅 후 WSASend				
				pstClientItem->m_stClientPacketInfo.m_cLoginSuccess = Login_Success( LOGIN_SUCCESS_CODE );								

				//  m_EnCryptionBuff에 1차적으로 Copy
				CopyMemory( pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, &pstClientItem->m_stClientPacketInfo.m_cLoginSuccess, sizeof( pstClientItem->m_stClientPacketInfo.m_cLoginSuccess ) );

				//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
				CryptionPacket( pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, pstClientItem->m_stSendOverlapped.m_IOCPBuff, Key, sizeof( pstClientItem->m_stClientPacketInfo.m_cLoginSuccess ) );

				pstClientItem->m_stSendOverlapped.m_nSuccessToSend = sizeof( pstClientItem->m_stClientPacketInfo.m_cLoginSuccess );
				pstClientItem->m_stSendOverlapped.m_WSABuff.buf = (char*)pstClientItem->m_stSendOverlapped.m_IOCPBuff;
				pstClientItem->m_stSendOverlapped.m_WSABuff.len = sizeof( pstClientItem->m_stClientPacketInfo.m_cLoginSuccess );
				
				bool Ret = IOCPSend( pcIOCPServer, pstClientItem );
				if ( Ret == false )
				{
					std::cout << "[ Server ]  Send 'Login Success' Packet Failed" << std::endl;

					m_cCriticalSection.Lock();
					m_cLog.WriteLogFile("'Login Success' Packet Failed");
					m_cCriticalSection.UnLock();

					bool Counting = PacketstatisticsProcess( pcIOCPServer, pstClientItem, LOGIN_SUCCESS, FALSE );
					if ( Counting == false )
					{
						std::cout << "[ Server ]  Did Not Counting 'LOGIN_SUCCESS' Packet" << std::endl;
					}
				}
				else
				{
					std::cout << "[ Server ]  Send 'Login Success' Packet Complete" << std::endl;

					m_cCriticalSection.Lock();
					m_cLog.WriteLogFile("'Login Success' Packet WSASend");
					m_cCriticalSection.UnLock();

					bool Counting = PacketstatisticsProcess( pcIOCPServer, pstClientItem, LOGIN_SUCCESS, TRUE );
					if ( Counting == false )
					{
						std::cout << "[ Server ]  Did Not Counting 'LOGIN_SUCCESS' Packet" << std::endl;
					}
				}
				
				// 로비화면으로 접속할 때 참가 대기중인 방이 있는지 확인 후 있으면 패킷 발송
				/*
				if (RoomList.empty() == false)
				{
					int Num = 0;
					std::list<std::vector<stClient_Item>>::iterator iter;

					for (iter = RoomList.begin(); iter != RoomList.end(); ++iter)
					{
						if (iter->size() < 2)
						{
							Num = iter->at(0).m_dwRoomNumber;
							break;
						}
					}

					if (Num != 0)
					{
						// 현재 참가 가능한 방 번호를 세팅
						pstClientItem->m_stClientPacketInfo.m_cAnotherMakeRoom = Another_Make_Room(Num);

						//  m_EnCryptionBuff에 1차적으로 Copy
						CopyMemory(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, &pstClientItem->m_stClientPacketInfo.m_cAnotherMakeRoom, sizeof(pstClientItem->m_stClientPacketInfo.m_cAnotherMakeRoom));

						//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
						CryptionPacket(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, pstClientItem->m_stSendOverlapped.m_IOCPBuff, Key, sizeof(pstClientItem->m_stClientPacketInfo.m_cAnotherMakeRoom));

						pstClientItem->m_stSendOverlapped.m_nSuccessToSend = sizeof(pstClientItem->m_stClientPacketInfo.m_cAnotherMakeRoom);
						pstClientItem->m_stSendOverlapped.m_WSABuff.buf = (char*)pstClientItem->m_stSendOverlapped.m_IOCPBuff;
						pstClientItem->m_stSendOverlapped.m_WSABuff.len = sizeof(pstClientItem->m_stClientPacketInfo.m_cAnotherMakeRoom);

						bool Ret = IOCPSend(pcIOCPServer, pstClientItem);
						if (Ret == false)
						{
							std::cout << "[ Server ]  Send 'Another Make Room' Packet Failed" << std::endl;

							m_cCriticalSection.Lock();
							m_cLog.WriteLogFile("'Another Make Room' Packet Failed");
							m_cCriticalSection.UnLock();

							bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, ANOTHER_MAKE_ROOM, FALSE);
							if (Counting == false)
							{
								std::cout << "[ Server ]  Did Not Counting 'ANOTHER_MAKE_ROOM' Packet" << std::endl;
							}
						}
						else
						{
							std::cout << "[ Server ]  Send 'Another Make Room' Packet Complete" << std::endl;

							m_cCriticalSection.Lock();
							m_cLog.WriteLogFile("'Another Make Room' Packet WSASend");
							m_cCriticalSection.UnLock();

							bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, ANOTHER_MAKE_ROOM, TRUE);
							if (Counting == false)
							{
								std::cout << "[ Server ]  Did Not Counting 'ANOTHER_MAKE_ROOM' Packet" << std::endl;
							}
						}
					}					
				}
				*/
			}
			else
			{
				// 요청한 클라이언트에게 LoginFail 세팅 후 WSASend
				pstClientItem->m_stClientPacketInfo.m_cLoginFail = Login_Fail( LOGIN_FAIL_CODE );

				//  m_EnCryptionBuff에 1차적으로 Copy
				CopyMemory( pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, &pstClientItem->m_stClientPacketInfo.m_cLoginFail, sizeof( pstClientItem->m_stClientPacketInfo.m_cLoginFail ) );

				//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
				CryptionPacket( pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, pstClientItem->m_stSendOverlapped.m_IOCPBuff, Key, sizeof( pstClientItem->m_stClientPacketInfo.m_cLoginFail ) );

				pstClientItem->m_stSendOverlapped.m_nSuccessToSend = sizeof( pstClientItem->m_stClientPacketInfo.m_cLoginFail );
				pstClientItem->m_stSendOverlapped.m_WSABuff.buf = (char*)pstClientItem->m_stSendOverlapped.m_IOCPBuff;
				pstClientItem->m_stSendOverlapped.m_WSABuff.len = sizeof( pstClientItem->m_stClientPacketInfo.m_cLoginFail );

				bool Ret = IOCPSend( pcIOCPServer, pstClientItem );
				if ( Ret == false )
				{
					std::cout << "[ Server ]  Send 'Login Fail' Packet Failed" << std::endl;

					m_cCriticalSection.Lock();
					m_cLog.WriteLogFile("'Login Fail' Packet Failed");
					m_cCriticalSection.UnLock();

					bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, LOGIN_FAIL, FALSE);
					if (Counting == false)
					{
						std::cout << "[ Server ]  Did Not Counting 'LOGIN_FAIL' Packet" << std::endl;
					}
				}
				else
				{
					std::cout << "[ Server ]  Send 'Login Fail' Packet Complete" << std::endl;

					m_cCriticalSection.Lock();
					m_cLog.WriteLogFile("'Login Fail' Packet WSASend");
					m_cCriticalSection.UnLock();

					bool Counting = PacketstatisticsProcess( pcIOCPServer, pstClientItem, LOGIN_FAIL, TRUE );
					if ( Counting == false )
					{
						std::cout << "[ Server ]  Did Not Counting 'LOGIN_FAIL' Packet" << std::endl;
					}
				}
			}

			ZeroMemory( m_PacketBuff, sizeof( BUFFSIZE ) );

			break;
		}		

		case MAKE_ROOM_REQUEST:
		{
			std::cout << "[ Server ]  N0." << pstClientItem->m_ClientID << " Client Send 'Make Room Request' Packet" << std::endl;

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile("'Make Room Request' Packet WSASRecv");
			m_cCriticalSection.UnLock();

			bool Counting = PacketstatisticsProcess( pcIOCPServer, pstClientItem, MAKE_ROOM_REQUEST, TRUE );
			if (Counting == false)
			{
				std::cout << "[ Server ]  Did Not Counting 'LOGIN_FAIL' Packet" << std::endl;
			}

			// 방을 완전히 새로 만들수 있는 상태
			if ((pstClientItem->m_dwRoomNumber == 0) && (m_bWaitRoom == false))
			{
				// 방 총 갯수 증가
				m_nAllRoomCount += 1;
				
				// 해당 클라이언트에 RoomNumber 세팅
				pstClientItem->m_dwRoomNumber = m_nAllRoomCount;

				// Room에 해당 클라이언트 추가
				Room.push_back(*pstClientItem);

				// RoomList에 Room을 추가
				RoomList.push_back(Room);

				// Room 재사용을 초기화
				Room.clear();

				// ClientList에 있는 해당 클라이언트에 RoomNumber 세팅
				for (auto iter = ClientList.begin(); iter != ClientList.end(); ++iter)
				{
					if (iter->m_ClientID == pstClientItem->m_ClientID)
					{
						iter->m_dwRoomNumber = m_nAllRoomCount;

						break;
					}
				}

				// 방 생성 성공 세팅 후 요청한 클라이언트에게 WSASend
				pstClientItem->m_stClientPacketInfo.m_cMakeRoomSuccess = Make_Room_Success(pstClientItem->m_dwRoomNumber);

				//  m_EnCryptionBuff에 1차적으로 Copy
				CopyMemory(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, &pstClientItem->m_stClientPacketInfo.m_cMakeRoomSuccess, sizeof(pstClientItem->m_stClientPacketInfo.m_cMakeRoomSuccess));

				//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
				CryptionPacket(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, pstClientItem->m_stSendOverlapped.m_IOCPBuff, Key, sizeof(pstClientItem->m_stClientPacketInfo.m_cMakeRoomSuccess));

				pstClientItem->m_stSendOverlapped.m_nSuccessToSend = sizeof(pstClientItem->m_stClientPacketInfo.m_cMakeRoomSuccess);
				pstClientItem->m_stSendOverlapped.m_WSABuff.buf = (char*)pstClientItem->m_stSendOverlapped.m_IOCPBuff;
				pstClientItem->m_stSendOverlapped.m_WSABuff.len = sizeof(pstClientItem->m_stClientPacketInfo.m_cMakeRoomSuccess);

				bool Ret = IOCPSend(pcIOCPServer, pstClientItem);
				if (Ret == false)
				{
					std::cout << "[ Server ]  Send 'Make Room Success' Packet Failed" << std::endl;

					m_cCriticalSection.Lock();
					m_cLog.WriteLogFile("'Make Room Success' Packet Failed");
					m_cCriticalSection.UnLock();

					bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, MAKE_ROOM_SUCCESS, FALSE);
					if (Counting == false)
					{
						std::cout << "[ Server ]  Did Not Counting 'MAKE_ROOM_SUCCESS' Packet" << std::endl;
					}
				}
				else
				{
					std::cout << "[ Server ]  Send 'Make Room Success' Packet Complete" << std::endl;

					m_cCriticalSection.Lock();
					m_cLog.WriteLogFile("'Make Room Success' Packet WSASend");
					m_cCriticalSection.UnLock();

					bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, MAKE_ROOM_SUCCESS, TRUE);
					if (Counting == false)
					{
						std::cout << "[ Server ]  Did Not Counting 'MAKE_ROOM_SUCCESS' Packet" << std::endl;
					}
				}

				// Room 생성 완료 후 true 세팅
				m_bWaitRoom = true;
			}
			// Room 생성이 안되는 경우
			else
			{
				//  입장 대기중인 Room이 있어 생성실패, 해당 RoomNumber를 요청한 클라이언테에게 알려줘야함
				if ((pstClientItem->m_dwRoomNumber == 0) && (m_bWaitRoom == true))
				{
					DWORD RoomNum = 0;
					
					// 현재 대기중인 RoomNumber 세팅
					for (auto iter = RoomList.begin(); iter != RoomList.end(); ++iter)
					{
						if (iter->size() == 1)
						{
							RoomNum = iter->at(0).m_dwRoomNumber;

							break;
						}
					}

					//  해당 클라이언트에 RoomNumber 세팅 후 WSASend
					pstClientItem->m_stClientPacketInfo.m_cAnotherMakeRoom = Another_Make_Room((int)RoomNum);

					//  m_EnCryptionBuff에 1차적으로 Copy
					CopyMemory(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, &pstClientItem->m_stClientPacketInfo.m_cAnotherMakeRoom, sizeof(pstClientItem->m_stClientPacketInfo.m_cAnotherMakeRoom));

					//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
					CryptionPacket(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, pstClientItem->m_stSendOverlapped.m_IOCPBuff, Key, sizeof(pstClientItem->m_stClientPacketInfo.m_cAnotherMakeRoom));

					pstClientItem->m_stSendOverlapped.m_nSuccessToSend = sizeof(pstClientItem->m_stClientPacketInfo.m_cAnotherMakeRoom);
					pstClientItem->m_stSendOverlapped.m_WSABuff.buf = (char*)pstClientItem->m_stSendOverlapped.m_IOCPBuff;
					pstClientItem->m_stSendOverlapped.m_WSABuff.len = sizeof(pstClientItem->m_stClientPacketInfo.m_cAnotherMakeRoom);

					bool Ret = IOCPSend(pcIOCPServer, pstClientItem);
					if (Ret == false)
					{
						std::cout << "[ Server ]  Send 'Another Make Room' Packet Failed" << std::endl;

						m_cCriticalSection.Lock();
						m_cLog.WriteLogFile("'Another Make Room' Packet Failed");
						m_cCriticalSection.UnLock();

						bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, ANOTHER_MAKE_ROOM, FALSE);
						if (Counting == false)
						{
							std::cout << "[ Server ]  Did Not Counting 'ANOTHER_MAKE_ROOM' Packet" << std::endl;
						}
					}
					else
					{
						std::cout << "[ Server ]  Send 'Another Make Room' Packet Complete" << std::endl;

						m_cCriticalSection.Lock();
						m_cLog.WriteLogFile("'Another Make Room' Packet WSASend");
						m_cCriticalSection.UnLock();

						bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, ANOTHER_MAKE_ROOM, TRUE);
						if (Counting == false)
						{
							std::cout << "[ Server ]  Did Not Counting 'ANOTHER_MAKE_ROOM' Packet" << std::endl;
						}
					}
				}
				// 나머지 케이스로 Room 생성 실패 상태
				else
				{
					pstClientItem->m_stClientPacketInfo.m_cMakeRoomFail = Make_Room_Fail(MAKE_ROOM_FAIL_CODE);

					//  m_EnCryptionBuff에 1차적으로 Copy
					CopyMemory(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, &pstClientItem->m_stClientPacketInfo.m_cMakeRoomFail, sizeof(pstClientItem->m_stClientPacketInfo.m_cMakeRoomFail));

					//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
					CryptionPacket(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, pstClientItem->m_stSendOverlapped.m_IOCPBuff, Key, sizeof(pstClientItem->m_stClientPacketInfo.m_cMakeRoomFail));

					pstClientItem->m_stSendOverlapped.m_nSuccessToSend = sizeof(pstClientItem->m_stClientPacketInfo.m_cMakeRoomFail);
					pstClientItem->m_stSendOverlapped.m_WSABuff.buf = (char*)pstClientItem->m_stSendOverlapped.m_IOCPBuff;
					pstClientItem->m_stSendOverlapped.m_WSABuff.len = sizeof(pstClientItem->m_stClientPacketInfo.m_cMakeRoomFail);

					bool Ret = IOCPSend(pcIOCPServer, pstClientItem);
					if (Ret == false)
					{
						std::cout << "[ Server ]  Send 'Make Room Fail' Packet Failed" << std::endl;

						m_cCriticalSection.Lock();
						m_cLog.WriteLogFile("'Make Room Fail' Packet Failed");
						m_cCriticalSection.UnLock();

						bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, MAKE_ROOM_FAIL, FALSE);
						if (Counting == false)
						{
							std::cout << "[ Server ]  Did Not Counting 'MAKE_ROOM_FAIL' Packet" << std::endl;
						}
					}
					else
					{
						std::cout << "[ Server ]  Send 'Make Room Fail' Packet Complete" << std::endl;

						m_cCriticalSection.Lock();
						m_cLog.WriteLogFile("'Make Room Fail' Packet WSASend");
						m_cCriticalSection.UnLock();

						bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, MAKE_ROOM_FAIL, TRUE);
						if (Counting == false)
						{
							std::cout << "[ Server ]  Did Not Counting 'MAKE_ROOM_FAIL' Packet" << std::endl;
						}
					}
				}
			}

			ZeroMemory( m_PacketBuff, sizeof( BUFFSIZE ) );

			break;
		}

		case JOIN_ROOM_REQUEST:
		{

			std::cout << "[ Server ]  No." << pstClientItem->m_ClientID << " Client Send 'Join Room Request' Packet" << std::endl;

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile("'Join Room Request' Packet WSARecv");
			m_cCriticalSection.UnLock();

			bool Counting = PacketstatisticsProcess( pcIOCPServer, pstClientItem, JOIN_ROOM_REQUEST, TRUE );
			if ( Counting == false )
			{
				std::cout << "[ Server ]  Did Not Counting 'JOIN_ROOM_REQUEST' Packet" << std::endl;
			}

			// Room 참여가 가능한 상태( Make_Room_Request 로 방목록을 받았던 클라이언트 or 그냥 Join_Room_Request를 보낸 클라이언트든 가능함)
			if ((pstClientItem->m_dwRoomNumber == 0) && (m_bWaitRoom == true))
			{
				DWORD RoomNum = 0;

				for (auto iter = RoomList.begin(); iter != RoomList.end(); ++iter)
				{
					if (iter->size() == 1)
					{
						// 클라이언트에 RoomNumber 세팅
						pstClientItem->m_dwRoomNumber = iter->at(0).m_dwRoomNumber;
						RoomNum = iter->at(0).m_dwRoomNumber;

						// Room에 클라이언트 추가
						iter->push_back(*pstClientItem);

						// TEST COUT
						std::cout << "No." << RoomNum << " Room에 클라이언트 추가, Room 사이즈 : " << iter->size() << std::endl;

						break;
					}
				}


				for (auto iter = ClientList.begin(); iter != ClientList.end(); ++iter)
				{
					if (iter->m_ClientID == pstClientItem->m_ClientID)
					{
						//  방에 참여하는 클라이언트이므로 ClientList에서도 RoomNumber 세팅
						iter->m_dwRoomNumber = RoomNum;
						
						break;
					}					
				}


				// 대기중인 Room 없는 상태로 세팅
				m_bWaitRoom = false;

				// 요청한 클라이언트에게 JoinRoomSuccess 세팅 후 WSASend
				pstClientItem->m_stClientPacketInfo.m_cJoinRoomSuccess = Join_Room_Success(JOIN_ROOM_SUCCESS_CODE);

				//  m_EnCryptionBuff에 1차적으로 Copy
				CopyMemory(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, &pstClientItem->m_stClientPacketInfo.m_cJoinRoomSuccess, sizeof(pstClientItem->m_stClientPacketInfo.m_cJoinRoomSuccess));

				//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
				CryptionPacket(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, pstClientItem->m_stSendOverlapped.m_IOCPBuff, Key, sizeof(pstClientItem->m_stClientPacketInfo.m_cJoinRoomSuccess));

				pstClientItem->m_stSendOverlapped.m_nSuccessToSend = sizeof(pstClientItem->m_stClientPacketInfo.m_cJoinRoomSuccess);
				pstClientItem->m_stSendOverlapped.m_WSABuff.buf = (char*)pstClientItem->m_stSendOverlapped.m_IOCPBuff;
				pstClientItem->m_stSendOverlapped.m_WSABuff.len = sizeof(pstClientItem->m_stClientPacketInfo.m_cJoinRoomSuccess);

				bool Ret = IOCPSend(pcIOCPServer, pstClientItem);
				if (Ret == false)
				{
					std::cout << "[ Server ]  Send 'Join Room Success' Packet Failed" << std::endl;

					m_cCriticalSection.Lock();
					m_cLog.WriteLogFile("'Join Room Success' Packet Failed");
					m_cCriticalSection.UnLock();

					bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, JOIN_ROOM_SUCCESS, FALSE);
					if (Counting == false)
					{
						std::cout << "[ Server ]  Did Not Counting 'JOIN_ROOM_SUCCESS' Packet" << std::endl;
					}
				}
				else
				{
					std::cout << "[ Server ]  Send 'Join Room Success' Packet Complete" << std::endl;

					m_cCriticalSection.Lock();
					m_cLog.WriteLogFile("'Join Room Success' Packet WSASend");
					m_cCriticalSection.UnLock();

					bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, JOIN_ROOM_SUCCESS, TRUE);
					if (Counting == false)
					{
						std::cout << "[ Server ]  Did Not Counting 'JOIN_ROOM_SUCCESS' Packet" << std::endl;
					}
				}


				for (auto iter = RoomList.begin(); iter != RoomList.end(); ++iter)
				{
					if ((iter->size() == 2) && (iter->at(0).m_dwRoomNumber == RoomNum))
					{
						// 이미 Room에 있는 클라이언트에게 새로 접속한 클라이언트 정보 보내려고 세팅 후 WSASend
						iter->at(0).m_stClientPacketInfo.m_cAnotherJoinRoom = Another_Join_Room(iter->at(1).m_IDString);

						//  m_EnCryptionBuff에 1차적으로 Copy
						CopyMemory(iter->at(0).m_stSendOverlapped.m_EnCryptionBuff, &iter->at(0).m_stClientPacketInfo.m_cAnotherJoinRoom, sizeof(iter->at(0).m_stClientPacketInfo.m_cAnotherJoinRoom));
												
						//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
						CryptionPacket(iter->at(0).m_stSendOverlapped.m_EnCryptionBuff, iter->at(0).m_stSendOverlapped.m_IOCPBuff, Key, sizeof(iter->at(0).m_stClientPacketInfo.m_cAnotherJoinRoom));

						iter->at(0).m_stSendOverlapped.m_nSuccessToSend = sizeof(iter->at(0).m_stClientPacketInfo.m_cAnotherJoinRoom);
						iter->at(0).m_stSendOverlapped.m_WSABuff.buf = (char*)iter->at(0).m_stSendOverlapped.m_IOCPBuff;
						iter->at(0).m_stSendOverlapped.m_WSABuff.len = sizeof(iter->at(0).m_stClientPacketInfo.m_cAnotherJoinRoom);

						bool Ret = IOCPSend(pcIOCPServer, &(iter->at(0)));
						if (Ret == false)
						{
							std::cout << "[ Server ]  Send 'Another Join Room' Packet Failed" << std::endl;

							m_cCriticalSection.Lock();
							m_cLog.WriteLogFile("'Another Join Room' Packet Failed");
							m_cCriticalSection.UnLock();

							bool Counting = PacketstatisticsProcess(pcIOCPServer, &(iter->at(0)), ANOTHER_JOIN_ROOM, FALSE);
							if (Counting == false)
							{
								std::cout << "[ Server ]  Did Not Counting 'ANOTHER_JOIN_ROOM' Packet" << std::endl;
							}
						}
						else
						{
							std::cout << "[ Server ]  Send 'Another Join Room' Packet Complete" << std::endl;

							m_cCriticalSection.Lock();
							m_cLog.WriteLogFile("'Another Join Room' Packet WSASend");
							m_cCriticalSection.UnLock();

							bool Counting = PacketstatisticsProcess(pcIOCPServer, &(iter->at(0)), ANOTHER_JOIN_ROOM, TRUE);
							if (Counting == false)
							{
								std::cout << "[ Server ]  Did Not Counting 'ANOTHER_JOIN_ROOM' Packet" << std::endl;
							}
						}

						//  이미 Room에 있는 클라이언트의 정보를 새로 접속한 클라이언트에게 보내려고 세팅 후 WSASend
						iter->at(1).m_stClientPacketInfo.m_cAlreadyJoinRoom = Already_Join_Room(iter->at(0).m_IDString);

						//  m_EnCryptionBuff에 1차적으로 Copy
						CopyMemory(iter->at(1).m_stSendOverlapped.m_EnCryptionBuff, &iter->at(1).m_stClientPacketInfo.m_cAlreadyJoinRoom, sizeof(iter->at(1).m_stClientPacketInfo.m_cAlreadyJoinRoom));

						//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
						CryptionPacket(iter->at(1).m_stSendOverlapped.m_EnCryptionBuff, iter->at(1).m_stSendOverlapped.m_IOCPBuff, Key, sizeof(iter->at(1).m_stClientPacketInfo.m_cAlreadyJoinRoom));

						iter->at(1).m_stSendOverlapped.m_nSuccessToSend = sizeof(iter->at(1).m_stClientPacketInfo.m_cAlreadyJoinRoom);
						iter->at(1).m_stSendOverlapped.m_WSABuff.buf = (char*)iter->at(1).m_stSendOverlapped.m_IOCPBuff;
						iter->at(1).m_stSendOverlapped.m_WSABuff.len = sizeof(iter->at(1).m_stClientPacketInfo.m_cAlreadyJoinRoom);

						Ret = IOCPSend(pcIOCPServer, &(iter->at(1)));
						if (Ret == false)
						{
							std::cout << "[ Server ]  Send 'Already Join Room' Packet Failed" << std::endl;

							m_cCriticalSection.Lock();
							m_cLog.WriteLogFile("'Already Join Room' Packet Failed");
							m_cCriticalSection.UnLock();

							bool Counting = PacketstatisticsProcess(pcIOCPServer, &(iter->at(1)), ALREADY_JOIN_ROOM, FALSE);
							if (Counting == false)
							{
								std::cout << "[ Server ]  Did Not Counting 'ALREADY_JOIN_ROOM' Packet" << std::endl;
							}
						}
						else
						{
							std::cout << "[ Server ]  Send 'Already Join Room' Packet Complete" << std::endl;

							m_cCriticalSection.Lock();
							m_cLog.WriteLogFile("'Already Join Room' Packet WSASend");
							m_cCriticalSection.UnLock();

							bool Counting = PacketstatisticsProcess(pcIOCPServer, &(iter->at(1)), ALREADY_JOIN_ROOM, TRUE);
							if (Counting == false)
							{
								std::cout << "[ Server ]  Did Not Counting 'ALREADY_JOIN_ROOM' Packet" << std::endl;
							}
						}

						break;
					}					
				}
			}
			// 정상적인 Room 참여가 불가능한 상태
			else
			{
				// 접속 가능한 Room이 존재하지 않아 새로 생성해야 함을 알려야 하는 상태
				if ((pstClientItem->m_dwRoomNumber == 0) && (m_bWaitRoom == false))
				{
					// 요청한 클라이언트에게 JoinRoomFail, NeedCreateRoomCode 세팅 후 WSASend
					pstClientItem->m_stClientPacketInfo.m_cJoinRoomFail = Join_Room_Fail(NEED_CREATE_ROOM_CODE);

					//  m_EnCryptionBuff에 1차적으로 Copy
					CopyMemory(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, &pstClientItem->m_stClientPacketInfo.m_cJoinRoomFail, sizeof(pstClientItem->m_stClientPacketInfo.m_cJoinRoomFail));

					//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
					CryptionPacket(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, pstClientItem->m_stSendOverlapped.m_IOCPBuff, Key, sizeof(pstClientItem->m_stClientPacketInfo.m_cJoinRoomFail));

					pstClientItem->m_stSendOverlapped.m_nSuccessToSend = sizeof(pstClientItem->m_stClientPacketInfo.m_cJoinRoomFail);
					pstClientItem->m_stSendOverlapped.m_WSABuff.buf = (char*)pstClientItem->m_stSendOverlapped.m_IOCPBuff;
					pstClientItem->m_stSendOverlapped.m_WSABuff.len = sizeof(pstClientItem->m_stClientPacketInfo.m_cJoinRoomFail);

					bool Ret = IOCPSend(pcIOCPServer, pstClientItem);
					if (Ret == false)
					{
						std::cout << "[ Server ]  Send 'Join Room Fail' Packet Failed" << std::endl;

						m_cCriticalSection.Lock();
						m_cLog.WriteLogFile("'Join Room Fail' Packet Failed");
						m_cCriticalSection.UnLock();

						bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, JOIN_ROOM_FAIL, FALSE);
						if (Counting == false)
						{
							std::cout << "[ Server ]  Did Not Counting 'JOIN_ROOM_FAIL' Packet" << std::endl;
						}
					}
					else
					{
						std::cout << "[ Server ]  Send 'Join Room Fail' Packet Complete" << std::endl;

						m_cCriticalSection.Lock();
						m_cLog.WriteLogFile("'Join Room Fail' Packet WSASend");
						m_cCriticalSection.UnLock();

						bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, JOIN_ROOM_FAIL, TRUE);
						if (Counting == false)
						{
							std::cout << "[ Server ]  Did Not Counting 'JOIN_ROOM_FAIL' Packet" << std::endl;
						}
					}
				}
				// 나머지 상태(이미 접속한 상태에서 Join_Room_Request를 보냈다던지 하는 케이스)
				else
				{
					// 요청한 클라이언트에게 JoinRoomFail 세팅 후 WSASend
					pstClientItem->m_stClientPacketInfo.m_cJoinRoomFail = Join_Room_Fail(JOIN_ROOM_FAIL_CODE);

					//  m_EnCryptionBuff에 1차적으로 Copy
					CopyMemory(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, &pstClientItem->m_stClientPacketInfo.m_cJoinRoomFail, sizeof(pstClientItem->m_stClientPacketInfo.m_cJoinRoomFail));

					//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
					CryptionPacket(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, pstClientItem->m_stSendOverlapped.m_IOCPBuff, Key, sizeof(pstClientItem->m_stClientPacketInfo.m_cJoinRoomFail));

					pstClientItem->m_stSendOverlapped.m_nSuccessToSend = sizeof(pstClientItem->m_stClientPacketInfo.m_cJoinRoomFail);
					pstClientItem->m_stSendOverlapped.m_WSABuff.buf = (char*)pstClientItem->m_stSendOverlapped.m_IOCPBuff;
					pstClientItem->m_stSendOverlapped.m_WSABuff.len = sizeof(pstClientItem->m_stClientPacketInfo.m_cJoinRoomFail);

					bool Ret = IOCPSend(pcIOCPServer, pstClientItem);
					if (Ret == false)
					{
						std::cout << "[ Server ]  Send 'Join Room Fail' Packet Failed" << std::endl;

						m_cCriticalSection.Lock();
						m_cLog.WriteLogFile("'Join Room Fail' Packet Failed");
						m_cCriticalSection.UnLock();

						bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, JOIN_ROOM_FAIL, FALSE);
						if (Counting == false)
						{
							std::cout << "[ Server ]  Did Not Counting 'JOIN_ROOM_FAIL' Packet" << std::endl;
						}
					}
					else
					{
						std::cout << "[ Server ]  Send 'Join Room Fail' Packet Complete" << std::endl;

						m_cCriticalSection.Lock();
						m_cLog.WriteLogFile("'Join Room Fail' Packet WSASend");
						m_cCriticalSection.UnLock();

						bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, JOIN_ROOM_FAIL, TRUE);
						if (Counting == false)
						{
							std::cout << "[ Server ]  Did Not Counting 'JOIN_ROOM_FAIL' Packet" << std::endl;
						}
					}
				}				
			}

			ZeroMemory(m_PacketBuff, sizeof(BUFFSIZE));

			break;
		}

		case MY_CHAT:
		{
			std::cout << "[ Server ]  N0." << pstClientItem->m_ClientID << " Client Send 'Chatting' Packet" << std::endl;

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile("'Chatting' Packet WSARecv");
			m_cCriticalSection.UnLock();

			bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, MY_CHAT, TRUE);
			if (Counting == false)
			{
				std::cout << "[ Server ]  Did Not Counting 'MY_CHAT' Packet" << std::endl;
			}

			CopyMemory( &pstClientItem->m_stClientPacketInfo.m_cMyChat, m_PacketBuff, sizeof( pstClientItem->m_stClientPacketInfo.m_cMyChat ) );

			//  Room에 참가하지 않은 클라이언트 일 경우 전체채팅으로 채팅내용 세팅 후 WSASend
			if (pstClientItem->m_dwRoomNumber == 0)
			{
				for (auto iter = ClientList.begin(); iter != ClientList.end(); ++iter)
				{
					if (pstClientItem->m_ClientID != iter->m_ClientID)
					{
						std::string AnotherString = pstClientItem->m_stClientPacketInfo.m_cMyChat.Chatting;
						iter->m_stClientPacketInfo.m_cAnotherchat = Another_Chat(AnotherString);

						//  m_EnCryptionBuff에 1차적으로 Copy
						CopyMemory(iter->m_stSendOverlapped.m_EnCryptionBuff, &(iter->m_stClientPacketInfo.m_cAnotherchat), sizeof(iter->m_stClientPacketInfo.m_cAnotherchat));

						//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
						CryptionPacket(iter->m_stSendOverlapped.m_EnCryptionBuff, iter->m_stSendOverlapped.m_IOCPBuff, Key, sizeof(iter->m_stClientPacketInfo.m_cAnotherchat));

						iter->m_stSendOverlapped.m_nSuccessToSend = sizeof(iter->m_stClientPacketInfo.m_cAnotherchat);
						iter->m_stSendOverlapped.m_WSABuff.buf = (char*)iter->m_stSendOverlapped.m_IOCPBuff;
						iter->m_stSendOverlapped.m_WSABuff.len = sizeof(iter->m_stClientPacketInfo.m_cAnotherchat);

						bool Ret = IOCPSend(pcIOCPServer, &(*iter));
						if (Ret == false)
						{
							std::cout << "[ Server ]  Send 'Chatting' Packet Failed" << std::endl;

							m_cCriticalSection.Lock();
							m_cLog.WriteLogFile("'Chatting' Packet Failed");
							m_cCriticalSection.UnLock();

							bool Counting = PacketstatisticsProcess(pcIOCPServer, &(*iter), ANOTHER_CHAT, FALSE);
							if (Counting == false)
							{
								std::cout << "[ Server ]  Did Not Counting 'ANOTHER_CHAT' Packet" << std::endl;
							}
						}
						else
						{
							std::cout << "[ Server ]  Send 'Chatting' Packet Complete" << std::endl;

							m_cCriticalSection.Lock();
							m_cLog.WriteLogFile("'Chatting' Packet WSASend");
							m_cCriticalSection.UnLock();

							bool Counting = PacketstatisticsProcess(pcIOCPServer, &(*iter), ANOTHER_CHAT, TRUE);
							if (Counting == false)
							{
								std::cout << "[ Server ]  Did Not Counting 'ANOTHER_CHAT' Packet" << std::endl;
							}
						}
					}
				}
			}
			//  Room에 참가한 클라이언트일 경우 Room에 있는 다른 클라이언트에게 채팅내용 세팅 후 WSASend
			else
			{
				for (auto iter = RoomList.begin(); iter != RoomList.end(); ++iter)
				{
					for (auto it = iter->begin(); it != iter->end(); ++it)
					{
						if ((it->m_dwRoomNumber == pstClientItem->m_dwRoomNumber) && (it->m_ClientID != pstClientItem->m_ClientID))
						{
							std::string AnotherString = pstClientItem->m_stClientPacketInfo.m_cMyChat.Chatting;
							it->m_stClientPacketInfo.m_cAnotherchat = Another_Chat(AnotherString);

							//  m_EnCryptionBuff에 1차적으로 Copy
							CopyMemory(it->m_stSendOverlapped.m_EnCryptionBuff, &(it->m_stClientPacketInfo.m_cAnotherchat), sizeof(it->m_stClientPacketInfo.m_cAnotherchat));

							//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
							CryptionPacket(it->m_stSendOverlapped.m_EnCryptionBuff, it->m_stSendOverlapped.m_IOCPBuff, Key, sizeof(it->m_stClientPacketInfo.m_cAnotherchat));

							it->m_stSendOverlapped.m_nSuccessToSend = sizeof(it->m_stClientPacketInfo.m_cAnotherchat);
							it->m_stSendOverlapped.m_WSABuff.buf = (char*)it->m_stSendOverlapped.m_IOCPBuff;
							it->m_stSendOverlapped.m_WSABuff.len = sizeof(it->m_stClientPacketInfo.m_cAnotherchat);

							bool Ret = IOCPSend(pcIOCPServer, &(*it));
							if (Ret == false)
							{
								std::cout << "[ Server ]  Send 'Chatting' Packet Failed" << std::endl;

								m_cCriticalSection.Lock();
								m_cLog.WriteLogFile("'Chatting' Packet Failed");
								m_cCriticalSection.UnLock();

								bool Counting = PacketstatisticsProcess(pcIOCPServer, &(*it), ANOTHER_CHAT, FALSE);
								if (Counting == false)
								{
									std::cout << "[ Server ]  Did Not Counting 'ANOTHER_CHAT' Packet" << std::endl;
								}
							}
							else
							{
								std::cout << "[ Server ]  Send 'Chatting' Packet Complete" << std::endl;

								m_cCriticalSection.Lock();
								m_cLog.WriteLogFile("'Chatting' Packet WSASend");
								m_cCriticalSection.UnLock();

								bool Counting = PacketstatisticsProcess(pcIOCPServer, &(*it), ANOTHER_CHAT, TRUE);
								if (Counting == false)
								{
									std::cout << "[ Server ]  Did Not Counting 'ANOTHER_CHAT' Packet" << std::endl;
								}
							}

							goto $Label2;
						}
					}
				}
			}
			

			$Label2:

			ZeroMemory( m_PacketBuff, sizeof( BUFFSIZE ) );

			break;
		}

		case GAME_START_REQUEST:
		{
			std::cout << "[ Server ]  N0." << pstClientItem->m_ClientID << " Client Send 'Game Start Request' Packet" << std::endl;

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile("'Game Start Request' WSARecv");
			m_cCriticalSection.UnLock();

			bool Counting = PacketstatisticsProcess( pcIOCPServer, pstClientItem, GAME_START_REQUEST, TRUE );
			if ( Counting == false )
			{
				std::cout << "[ Server ]  Did Not Counting 'GAME_START_REQUEST' Packet" << std::endl;
			}

			// 방을 만들었거나, 방에 참가중이거나 둘다 해당 안되는 상태
			if (pstClientItem->m_dwRoomNumber == 0)
			{
				//  요청한 클라이언트에게 GameStartFail 세팅 후 WSASend
				pstClientItem->m_stClientPacketInfo.m_cStartFail = Game_Start_Fail(NOT_JOIN_ROOM_STATUS_CODE);

				//  m_EnCryptionBuff에 1차적으로 Copy
				CopyMemory(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, &pstClientItem->m_stClientPacketInfo.m_cStartFail, sizeof(pstClientItem->m_stClientPacketInfo.m_cStartFail));

				//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
				CryptionPacket(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, pstClientItem->m_stSendOverlapped.m_IOCPBuff, Key, sizeof(pstClientItem->m_stClientPacketInfo.m_cStartFail));

				pstClientItem->m_stSendOverlapped.m_nSuccessToSend = sizeof(pstClientItem->m_stClientPacketInfo.m_cStartFail);
				pstClientItem->m_stSendOverlapped.m_WSABuff.buf = (char*)pstClientItem->m_stSendOverlapped.m_IOCPBuff;
				pstClientItem->m_stSendOverlapped.m_WSABuff.len = sizeof(pstClientItem->m_stClientPacketInfo.m_cStartFail);

				bool Ret = IOCPSend(pcIOCPServer, pstClientItem);
				if (Ret == false)
				{
					std::cout << "[ Server ]  Send 'Game Start Fail' Packet Failed" << std::endl;

					m_cCriticalSection.Lock();
					m_cLog.WriteLogFile("'Game Start Fail' Packet Failed");
					m_cCriticalSection.UnLock();

					bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, GAME_START_FAIL, FALSE);
					if (Counting == false)
					{
						std::cout << "[ Server ]  Did Not Counting 'GAME_START_FAIL' Packet" << std::endl;
					}
				}
				else
				{
					std::cout << "[ Server ]  Send 'Game Start Fail' Packet Complete" << std::endl;

					m_cCriticalSection.Lock();
					m_cLog.WriteLogFile("'Game Start Fail' Packet WSASend");
					m_cCriticalSection.UnLock();

					bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, GAME_START_FAIL, TRUE);
					if (Counting == false)
					{
						std::cout << "[ Server ]  Did Not Counting 'GAME_START_FAIL' Packet" << std::endl;
					}
				}
			}
			// 방장이거나 방에 참여중인 상태
			else
			{
				bool bStartAvailable = false;

				for (auto iter = RoomList.begin(); iter != RoomList.end(); ++iter)
				{
					for (auto it = iter->begin(); it != iter->end(); ++it)
					{
						if (it->m_dwRoomNumber == pstClientItem->m_dwRoomNumber)
						{
							if (iter->size() == 2)
							{
								bStartAvailable = true;
								goto $Label3;
							}
							else if (iter->size() == 1)
							{
								bStartAvailable = false;
								goto $Label3;
							}
						}						
					}
				}

				$Label3:

				// 게임 시작 가능한 상태
				if (bStartAvailable)
				{
					for (auto iter = RoomList.begin(); iter != RoomList.end(); ++iter)
					{
						for (auto it = iter->begin(); it != iter->end(); ++it)
						{
							if (it->m_dwRoomNumber == pstClientItem->m_dwRoomNumber)
							{
								//  Room에 있는 첫번째 클라이언트에게 GameStartSucc, FIRST_CLIENT_CODE 세팅 후 WSASend
								iter->at(0).m_stClientPacketInfo.m_cStartSuccess = Game_Start_Success(FIRST_CLIENT_CODE);

								//  m_EnCryptionBuff에 1차적으로 Copy
								CopyMemory(iter->at(0).m_stSendOverlapped.m_EnCryptionBuff, &(iter->at(0).m_stClientPacketInfo.m_cStartSuccess), sizeof(iter->at(0).m_stClientPacketInfo.m_cStartSuccess));

								//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
								CryptionPacket(iter->at(0).m_stSendOverlapped.m_EnCryptionBuff, iter->at(0).m_stSendOverlapped.m_IOCPBuff, Key, sizeof(iter->at(0).m_stClientPacketInfo.m_cStartFail));

								iter->at(0).m_stSendOverlapped.m_nSuccessToSend = sizeof(iter->at(0).m_stClientPacketInfo.m_cStartFail);
								iter->at(0).m_stSendOverlapped.m_WSABuff.buf = (char*)iter->at(0).m_stSendOverlapped.m_IOCPBuff;
								iter->at(0).m_stSendOverlapped.m_WSABuff.len = sizeof(iter->at(0).m_stClientPacketInfo.m_cStartFail);

								bool bRet = IOCPSend(pcIOCPServer, &(iter->at(0)));
								if (bRet == false)
								{
									std::cout << "[ Server ]  Send 'Game Start Success' Packet Failed" << std::endl;

									m_cCriticalSection.Lock();
									m_cLog.WriteLogFile("'Game Start Success' Packet Failed");
									m_cCriticalSection.UnLock();

									bool Counting = PacketstatisticsProcess(pcIOCPServer, &(iter->at(0)), GAME_START_SUCCESS, FALSE);
									if (Counting == false)
									{
										std::cout << "[ Server ]  Did Not Counting 'GAME_START_SUCCESS' Packet" << std::endl;
									}
								}
								else
								{
									std::cout << "[ Server ]  Send 'Game Start Success' Packet Complete" << std::endl;

									m_cCriticalSection.Lock();
									m_cLog.WriteLogFile("'Game Start Success' Packet WSASend");
									m_cCriticalSection.UnLock();

									bool Counting = PacketstatisticsProcess(pcIOCPServer, &(iter->at(0)), GAME_START_SUCCESS, TRUE);
									if (Counting == false)
									{
										std::cout << "[ Server ]  Did Not Counting 'GAME_START_SUCCESS' Packet" << std::endl;
									}
								}


								//  Room에 있는 두번째 클라이언트에게 GameStartSucc, SECOND_CLIENT_CODE 세팅 후 WSASend
								iter->at(1).m_stClientPacketInfo.m_cStartSuccess = Game_Start_Success(SECOND_CLIENT_CODE);

								//  m_EnCryptionBuff에 1차적으로 Copy
								CopyMemory(iter->at(1).m_stSendOverlapped.m_EnCryptionBuff, &(iter->at(1).m_stClientPacketInfo.m_cStartSuccess), sizeof(iter->at(1).m_stClientPacketInfo.m_cStartSuccess));

								//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
								CryptionPacket(iter->at(1).m_stSendOverlapped.m_EnCryptionBuff, iter->at(1).m_stSendOverlapped.m_IOCPBuff, Key, sizeof(iter->at(1).m_stClientPacketInfo.m_cStartFail));

								iter->at(1).m_stSendOverlapped.m_nSuccessToSend = sizeof(iter->at(1).m_stClientPacketInfo.m_cStartFail);
								iter->at(1).m_stSendOverlapped.m_WSABuff.buf = (char*)iter->at(1).m_stSendOverlapped.m_IOCPBuff;
								iter->at(1).m_stSendOverlapped.m_WSABuff.len = sizeof(iter->at(1).m_stClientPacketInfo.m_cStartFail);

								bool Ret = IOCPSend(pcIOCPServer, &(iter->at(1)));
								if (Ret == false)
								{
									std::cout << "[ Server ]  Send 'Game Start Success' Packet Failed" << std::endl;

									m_cCriticalSection.Lock();
									m_cLog.WriteLogFile("'Game Start Success' Packet Failed");
									m_cCriticalSection.UnLock();

									bool Counting = PacketstatisticsProcess(pcIOCPServer, &(iter->at(1)), GAME_START_SUCCESS, FALSE);
									if (Counting == false)
									{
										std::cout << "[ Server ]  Did Not Counting 'GAME_START_SUCCESS' Packet" << std::endl;
									}
								}
								else
								{
									std::cout << "[ Server ]  Send 'Game Start Success' Packet Complete" << std::endl;

									m_cCriticalSection.Lock();
									m_cLog.WriteLogFile("'Game Start Success' Packet WSASend");
									m_cCriticalSection.UnLock();

									bool Counting = PacketstatisticsProcess(pcIOCPServer, &(iter->at(1)), GAME_START_SUCCESS, TRUE);
									if (Counting == false)
									{
										std::cout << "[ Server ]  Did Not Counting 'GAME_START_SUCCESS' Packet" << std::endl;
									}
								}

								goto $Label4;
							}
						}
					}					
				}
				// 인원수 부족으로 게임 시작 불가능한 상태
				else
				{
					//  요청한 클라이언트에게 GameStartFail 세팅 후 WSASend
					pstClientItem->m_stClientPacketInfo.m_cStartFail = Game_Start_Fail(NEED_ONE_MORE_USER_CODE);

					//  m_EnCryptionBuff에 1차적으로 Copy
					CopyMemory(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, &pstClientItem->m_stClientPacketInfo.m_cStartFail, sizeof(pstClientItem->m_stClientPacketInfo.m_cStartFail));

					//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
					CryptionPacket(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, pstClientItem->m_stSendOverlapped.m_IOCPBuff, Key, sizeof(pstClientItem->m_stClientPacketInfo.m_cStartFail));

					pstClientItem->m_stSendOverlapped.m_nSuccessToSend = sizeof(pstClientItem->m_stClientPacketInfo.m_cStartFail);
					pstClientItem->m_stSendOverlapped.m_WSABuff.buf = (char*)pstClientItem->m_stSendOverlapped.m_IOCPBuff;
					pstClientItem->m_stSendOverlapped.m_WSABuff.len = sizeof(pstClientItem->m_stClientPacketInfo.m_cStartFail);

					bool Ret = IOCPSend(pcIOCPServer, pstClientItem);
					if (Ret == false)
					{
						std::cout << "[ Server ]  Send 'Game Start Fail' Packet Failed" << std::endl;

						m_cCriticalSection.Lock();
						m_cLog.WriteLogFile("'Game Start Fail' Packet Failed");
						m_cCriticalSection.UnLock();

						bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, GAME_START_FAIL, FALSE);
						if (Counting == false)
						{
							std::cout << "[ Server ]  Did Not Counting 'GAME_START_FAIL' Packet" << std::endl;
						}
					}
					else
					{
						std::cout << "[ Server ]  Send 'Game Start Fail' Packet Complete" << std::endl;

						m_cCriticalSection.Lock();
						m_cLog.WriteLogFile("'Game Start Fail' Packet WSASend");
						m_cCriticalSection.UnLock();

						bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, GAME_START_FAIL, TRUE);
						if (Counting == false)
						{
							std::cout << "[ Server ]  Did Not Counting 'GAME_START_FAIL' Packet" << std::endl;
						}
					}
				}
			}

			$Label4:

			ZeroMemory(m_PacketBuff, sizeof(BUFFSIZE));

			break;
		}

		case MY_STONE_PUT:
		{
			std::cout << "[ Server ]  N0." << pstClientItem->m_ClientID << " Client Send 'My Stone Put' Packet" << std::endl;

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile("My Stone Put Packet WSARecv");
			m_cCriticalSection.UnLock();

			bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, MY_STONE_PUT, TRUE);
			if (Counting == false)
			{
				std::cout << "[ Server ]  Did Not Counting 'MY_STONE_PUT' Packet" << std::endl;
			}

			CopyMemory(&pstClientItem->m_stClientPacketInfo.m_cMyStonePut, m_PacketBuff, sizeof(pstClientItem->m_stClientPacketInfo.m_cMyStonePut));


			for (auto iter = RoomList.begin(); iter != RoomList.end(); ++iter)
			{
				for (auto it = iter->begin(); it != iter->end(); ++it)
				{
					if ((it->m_dwRoomNumber == pstClientItem->m_dwRoomNumber) && (it->m_ClientID != pstClientItem->m_ClientID))
					{
						//  Room에 있는 다른 클라이언트에게 좌표정보 세팅 후 WSASend
						it->m_stClientPacketInfo.m_cAnotherStonePut = Game_Another_Stone_Put(pstClientItem->m_stClientPacketInfo.m_cMyStonePut.Index_X, pstClientItem->m_stClientPacketInfo.m_cMyStonePut.Index_Y);

						//  m_EnCryptionBuff에 1차적으로 Copy
						CopyMemory(it->m_stSendOverlapped.m_EnCryptionBuff, &it->m_stClientPacketInfo.m_cAnotherStonePut, sizeof(it->m_stClientPacketInfo.m_cAnotherStonePut));

						//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
						CryptionPacket(it->m_stSendOverlapped.m_EnCryptionBuff, it->m_stSendOverlapped.m_IOCPBuff, Key, sizeof(it->m_stClientPacketInfo.m_cAnotherStonePut));

						it->m_stSendOverlapped.m_nSuccessToSend = sizeof(it->m_stClientPacketInfo.m_cAnotherStonePut);
						it->m_stSendOverlapped.m_WSABuff.buf = (char*)it->m_stSendOverlapped.m_IOCPBuff;
						it->m_stSendOverlapped.m_WSABuff.len = sizeof(it->m_stClientPacketInfo.m_cAnotherStonePut);

						bool Ret = IOCPSend(pcIOCPServer, &(*it));
						if (Ret == false)
						{
							std::cout << "[ Server ]  Send 'Another Stone Put' Packet Failed" << std::endl;

							m_cCriticalSection.Lock();
							m_cLog.WriteLogFile("'Another Stone Put' Packet Failed");
							m_cCriticalSection.UnLock();

							bool Counting = PacketstatisticsProcess(pcIOCPServer, &(*it), ANOTHER_STONE_PUT, FALSE);
							if (Counting == false)
							{
								std::cout << "[ Server ]  Did Not Counting 'ANOTHER_STONE_PUT' Packet" << std::endl;
							}
						}
						else
						{
							std::cout << "[ Server ]  Send 'Another Stone Put' Packet Complete" << std::endl;

							m_cCriticalSection.Lock();
							m_cLog.WriteLogFile("'Another Stone Put' Packet WSASend");
							m_cCriticalSection.UnLock();

							bool Counting = PacketstatisticsProcess(pcIOCPServer, &(*it), ANOTHER_STONE_PUT, TRUE);
							if (Counting == false)
							{
								std::cout << "[ Server ]  Did Not Counting 'ANOTHER_STONE_PUT' Packet" << std::endl;
							}
						}

						goto $Label5;
					}
				}
			}

			$Label5:

			ZeroMemory(m_PacketBuff, sizeof(BUFFSIZE));

			break;
		}

		case MY_GAME_TIME_OUT:
		{
			std::cout << "[ Server ]  N0." << pstClientItem->m_ClientID << " Client Send 'My Game Time Out' Packet" << std::endl;

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile("'My Stone Put' Packet WSARecv");
			m_cCriticalSection.UnLock();

			bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, MY_GAME_TIME_OUT, TRUE);
			if (Counting == false)
			{
				std::cout << "[ Server ]  Did Not Counting 'MY_GAME_TIME_OUT' Packet" << std::endl;
			}


			for (auto iter = RoomList.begin(); iter != RoomList.end(); ++iter)
			{
				for (auto it = iter->begin(); it != iter->end(); ++it)
				{
					if ((it->m_dwRoomNumber == pstClientItem->m_dwRoomNumber) && (it->m_ClientID != pstClientItem->m_ClientID))
					{
						//  Room에 있는 상대 클라이언트에게 AnotherGameTimeOut 세팅 후 WSASend
						it->m_stClientPacketInfo.m_cAnotherTimeOut = Another_Game_Time_Out(true);

						//  m_EnCryptionBuff에 1차적으로 Copy
						CopyMemory(it->m_stSendOverlapped.m_EnCryptionBuff, &it->m_stClientPacketInfo.m_cAnotherTimeOut, sizeof(it->m_stClientPacketInfo.m_cAnotherTimeOut));

						//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
						CryptionPacket(it->m_stSendOverlapped.m_EnCryptionBuff, it->m_stSendOverlapped.m_IOCPBuff, Key, sizeof(it->m_stClientPacketInfo.m_cAnotherTimeOut));

						it->m_stSendOverlapped.m_nSuccessToSend = sizeof(it->m_stClientPacketInfo.m_cAnotherTimeOut);
						it->m_stSendOverlapped.m_WSABuff.buf = (char*)it->m_stSendOverlapped.m_IOCPBuff;
						it->m_stSendOverlapped.m_WSABuff.len = sizeof(it->m_stClientPacketInfo.m_cAnotherTimeOut);

						bool Ret = IOCPSend(pcIOCPServer, &(*it));
						if (Ret == false)
						{
							std::cout << "[ Server ]  Send 'Another Time Out' Packet Failed" << std::endl;

							m_cCriticalSection.Lock();
							m_cLog.WriteLogFile("'Another Time Out' Packet Failed");
							m_cCriticalSection.UnLock();

							bool Counting = PacketstatisticsProcess(pcIOCPServer, &(*it), ANOTHER_GAME_TIME_OUT, FALSE);
							if (Counting == false)
							{
								std::cout << "[ Server ]  Did Not Counting 'ANOTHER_GAME_TIME_OUT' Packet" << std::endl;
							}
						}
						else
						{
							std::cout << "[ Server ]  Send 'Another Time Out' Packet Complete" << std::endl;

							m_cCriticalSection.Lock();
							m_cLog.WriteLogFile("'Another Time Out' Packet WSASend");
							m_cCriticalSection.UnLock();

							bool Counting = PacketstatisticsProcess(pcIOCPServer, &(*it), ANOTHER_GAME_TIME_OUT, TRUE);
							if (Counting == false)
							{
								std::cout << "[ Server ]  Did Not Counting 'ANOTHER_GAME_TIME_OUT' Packet" << std::endl;
							}
						}

						goto $Label6;
					}
				}
			}

			$Label6:

			ZeroMemory( m_PacketBuff, sizeof( BUFFSIZE ) );

			break;
		}

		case GAME_END:
		{
			std::cout << "[ Server ]  N0." << pstClientItem->m_ClientID << " Client Send 'Game End' Packet" << std::endl;

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile("'Game End' Packet WSARecv");
			m_cCriticalSection.UnLock();

			bool Counting = PacketstatisticsProcess( pcIOCPServer, pstClientItem, GAME_END, TRUE );
			if ( Counting == false )
			{
				std::cout << "[ Server ]  Did Not Counting 'GAME_END' Packet" << std::endl;
			}

			// 패킷 통계 txt 출력
			pcIOCPServer->m_cLog.WritePacketStatisticsFile();

			break;
		}

		case CHECK_ALIVE_REQUEST:
		{
			std::cout << "[ Server ]  N0." << pstClientItem->m_ClientID << " Client Send 'Check Alive Request' Packet" << std::endl;

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile("'Check Alive Request' Packet WSARecv");
			m_cCriticalSection.UnLock();

			bool Counting = PacketstatisticsProcess( pcIOCPServer, pstClientItem, CHECK_ALIVE_REQUEST, TRUE );
			if ( Counting == false )
			{
				std::cout << "[ Server ]  Did Not Counting 'CHECK_ALIVE_REQUEST' Packet" << std::endl;
			}

			// Room에 있는 다른 클라이언트의 접속상태 확인
			for (auto iter = RoomList.begin(); iter != RoomList.end(); ++iter)
			{
				for (auto it = iter->begin(); it != iter->end(); ++it)
				{
					if ((it->m_dwRoomNumber == pstClientItem->m_dwRoomNumber) && (it->m_ClientID != pstClientItem->m_ClientID))
					{
						it->m_stClientPacketInfo.m_cCheckAlive = Check_Alive(CHECK_ALIVE_CODE);

						//  m_EnCryptionBuff에 1차적으로 Copy
						CopyMemory(it->m_stSendOverlapped.m_EnCryptionBuff, &it->m_stClientPacketInfo.m_cCheckAlive, sizeof(it->m_stClientPacketInfo.m_cCheckAlive));

						//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
						CryptionPacket(it->m_stSendOverlapped.m_EnCryptionBuff, it->m_stSendOverlapped.m_IOCPBuff, Key, sizeof(it->m_stClientPacketInfo.m_cCheckAlive));

						it->m_stSendOverlapped.m_nSuccessToSend = sizeof(it->m_stClientPacketInfo.m_cCheckAlive);
						it->m_stSendOverlapped.m_WSABuff.buf = (char*)it->m_stSendOverlapped.m_IOCPBuff;
						it->m_stSendOverlapped.m_WSABuff.len = sizeof(it->m_stClientPacketInfo.m_cCheckAlive);

						bool Ret = IOCPSend(pcIOCPServer, &(*it));
						if (Ret == false)
						{
							std::cout << "[ Server ]  Send 'Check Alive' Packet Failed" << std::endl;

							m_cCriticalSection.Lock();
							m_cLog.WriteLogFile("'Check Alive' Packet Failed");
							m_cCriticalSection.UnLock();

							bool Counting = PacketstatisticsProcess(pcIOCPServer, &(*it), CHECK_ALIVE, FALSE);
							if (Counting == false)
							{
								std::cout << "[ Server ]  Did Not Counting 'CHECK_ALIVE' Packet" << std::endl;
							}
						}
						else
						{
							std::cout << "[ Server ]  Send 'Check Alive' Packet Complete" << std::endl;

							m_cCriticalSection.Lock();
							m_cLog.WriteLogFile("'Check Alive' Packet WSASend");
							m_cCriticalSection.UnLock();

							bool Counting = PacketstatisticsProcess(pcIOCPServer, &(*it), CHECK_ALIVE, TRUE);
							if (Counting == false)
							{
								std::cout << "[ Server ]  Did Not Counting 'CHECK_ALIVE' Packet" << std::endl;
							}
						}

						goto $Label7;
					}
				}
			}

			$Label7:

			ZeroMemory(m_PacketBuff, sizeof(BUFFSIZE));

			break;
		}

		case STILL_ALIVE:
		{
			std::cout << "[ Server ]  N0." << pstClientItem->m_ClientID << " Client Send 'Still Alive' Packet" << std::endl;

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile("'Still Alive' Packet WSARecv");
			m_cCriticalSection.UnLock();

			bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, STILL_ALIVE, TRUE);
			if (Counting == false)
			{
				std::cout << "[ Server ]  Did Not Counting 'STILL_ALIVE' Packet" << std::endl;
			}

			// TODO : ...

			break;
		}

		case ECHO_SEND:
		{
			std::cout << "[ Server ]  N0." << pstClientItem->m_ClientID << " Client Send 'Echo Send' Packet" << std::endl;

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile("'Echo Send' Packet WSARecv");
			m_cCriticalSection.UnLock();

			pstClientItem->m_stClientPacketInfo.m_cEchoRecv = Echo_Recv(0);

			//  m_EnCryptionBuff에 1차적으로 Copy
			CopyMemory(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, &pstClientItem->m_stClientPacketInfo.m_cEchoRecv, sizeof(pstClientItem->m_stClientPacketInfo.m_cEchoRecv));

			//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
			CryptionPacket(pstClientItem->m_stSendOverlapped.m_EnCryptionBuff, pstClientItem->m_stSendOverlapped.m_IOCPBuff, Key, sizeof(pstClientItem->m_stClientPacketInfo.m_cEchoRecv));

			pstClientItem->m_stSendOverlapped.m_nSuccessToSend = sizeof(pstClientItem->m_stClientPacketInfo.m_cEchoRecv);
			pstClientItem->m_stSendOverlapped.m_WSABuff.buf = (char*)pstClientItem->m_stSendOverlapped.m_IOCPBuff;
			pstClientItem->m_stSendOverlapped.m_WSABuff.len = sizeof(pstClientItem->m_stClientPacketInfo.m_cEchoRecv);

			bool Ret = IOCPSend(pcIOCPServer, pstClientItem);
			if (Ret == false)
			{
				std::cout << "[ Server ]  Send 'Echo Recv' Packet Failed" << std::endl;

				m_cCriticalSection.Lock();
				m_cLog.WriteLogFile("'Echo Recv' Packet Failed");
				m_cCriticalSection.UnLock();	

			}
			else
			{
				std::cout << "[ Server ]  Send 'Echo Recv' Packet Complete" << std::endl;

				m_cCriticalSection.Lock();
				m_cLog.WriteLogFile("'Echo Recv' Packet WSASend");
				m_cCriticalSection.UnLock();
			}

			break;
		}

		case MAKE_ERROR:
		{
			// Error 패킷 의도적으로 메모리 접근 오류 발생시켜 덤프파일 생성 및 서버 종료 유도
			std::cout << "[ Server ]  N0." << pstClientItem->m_ClientID << " Client Send 'Make Error' Packet" << std::endl;

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile("'Make Error' Packet WSARecv");
			m_cCriticalSection.UnLock();

			bool Counting = PacketstatisticsProcess( pcIOCPServer, pstClientItem, MAKE_ERROR, TRUE );
			if ( Counting == false )
			{
				std::cout << "[ Server ]  Did Not Counting 'MAKE_ERROR' Packet" << std::endl;
			}

			
			for ( auto iter = ClientList.begin(); iter != ClientList.end(); ++iter )
			{
				if ( iter->m_ClientID != pstClientItem->m_ClientID )
				{
					iter->m_stClientPacketInfo.m_cProgramShutdown = Program_Shutdown( PROGRAM_SHUTDOWN_CODE );

					//  m_EnCryptionBuff에 1차적으로 Copy
					CopyMemory( iter->m_stSendOverlapped.m_EnCryptionBuff, &iter->m_stClientPacketInfo.m_cProgramShutdown, sizeof( iter->m_stClientPacketInfo.m_cProgramShutdown ) );

					//  m_IOCPBuff에 XOR Encryption 처리한 데이터 Copy
					CryptionPacket( iter->m_stSendOverlapped.m_EnCryptionBuff, iter->m_stSendOverlapped.m_IOCPBuff, Key, sizeof( iter->m_stClientPacketInfo.m_cProgramShutdown ) );

					iter->m_stSendOverlapped.m_nSuccessToSend = sizeof( iter->m_stClientPacketInfo.m_cProgramShutdown );
					iter->m_stSendOverlapped.m_WSABuff.buf = (char*)iter->m_stSendOverlapped.m_IOCPBuff;
					iter->m_stSendOverlapped.m_WSABuff.len = sizeof( iter->m_stClientPacketInfo.m_cProgramShutdown );

					bool Ret = IOCPSend( pcIOCPServer, &(*iter) );
					if ( Ret == false )
					{
						std::cout << "[ Server ]  Send 'Program Shutdown' Packet Failed" << std::endl;

						m_cCriticalSection.Lock();
						m_cLog.WriteLogFile("'Program Shutdown' Packet Failed");
						m_cCriticalSection.UnLock();

						bool Counting = PacketstatisticsProcess( pcIOCPServer, &(*iter), PROGRAM_SHUTDOWN, FALSE );
						if ( Counting == false )


						{
							std::cout << "[ Server ]  Did Not Counting 'PROGRAM_SHUTDOWN' Packet" << std::endl;
						}
					}
					else
					{
						std::cout << "[ Server ]  Send 'Program Shutdown' Packet Complete" << std::endl;

						m_cCriticalSection.Lock();
						m_cLog.WriteLogFile("'Program Shutdown' Packet WSASend");
						m_cCriticalSection.UnLock();

						bool Counting = PacketstatisticsProcess( pcIOCPServer, &(*iter), PROGRAM_SHUTDOWN, TRUE );
						if ( Counting == false )
						{
							std::cout << "[ Server ]  Did Not Counting 'PROGRAM_SHUTDOWN' Packet" << std::endl;
						}
					}
				}
			}

			pcIOCPServer->m_cLog.WritePacketStatisticsFile();

			// Memory Access Error
			char* Error = "MakeError";
			Error[1] = 'a';

			break;
		}

		case MAKE_MEMORY_LEAK:
		{
			// Error 패킷 의도적으로 메모리 할당 후 해체 하지 않음
			std::cout << "[ Server ]  N0." << pstClientItem->m_ClientID << " Client Send 'Make Memory Leak' Packet" << std::endl;

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile("'Make Memory Leak' Packet WSARecv");
			m_cCriticalSection.UnLock();

			// Memory Leak 유도
			int* MemoryLeak = new int(4);

			break;
		}

		case PROGRAM_QUIT:
		{
			// 클라이언트 정상종료시 처리
			std::cout << "[ Server ]  N0." << pstClientItem->m_ClientID << " Client Send 'Program Quit' Packet" << std::endl;

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile("'Program Quit' Packet WSARecv");
			m_cCriticalSection.UnLock();

			bool Counting = PacketstatisticsProcess(pcIOCPServer, pstClientItem, PROGRAM_QUIT, TRUE);
			if (Counting == false)
			{
				std::cout << "[ Server ]  Did Not Counting 'PROGRAM_QUIT' Packet" << std::endl;
			}

			/*SOCKADDR_IN DisconnectClientAddr;
			stClient_Item DisconnectClient;

			DisconnectClient = *pstClientItem;

			int DisconnectClientAddrLen = sizeof(SOCKADDR_IN);

			//  RoomList에서 해당 클라이언트 삭제
			for (auto iter = RoomList.begin(); iter != RoomList.end(); ++iter)
			{
				for (auto it = iter->begin(); it != iter->end(); ++it)
				{
					if (it->m_ClientID == DisconnectClient.m_ClientID)
					{
						int Size = iter->size();
						
						if (Size == 1)
						{
							iter->clear();
							RoomList.erase(iter);
							m_nAllRoomCount--;

							goto $Label12;
						}
						else if (Size == 2)
						{
							iter->erase(it);

							goto $Label12;
						}
					}
				}
			}

			$Label12:

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile("RoomList in Disconnected Room Erase complete");
			m_cCriticalSection.UnLock();

			//  ClientList에서 해당 클라이언트 삭제
			for (auto iter = ClientList.begin(); iter != ClientList.end(); ++iter)
			{
				if (iter->m_ClientID == DisconnectClient.m_ClientID)
				{
					ClientList.erase(iter);
					m_nNowConnectClientCount--;

					break;
				}
			}

			m_cCriticalSection.Lock();
			m_cLog.WriteLogFile("ClientList in Disconnected Client Erase complete");
			m_cCriticalSection.UnLock();

			getpeername(DisconnectClient.m_ClientSocket, (PSOCKADDR)&DisconnectClientAddr, &DisconnectClientAddrLen);

			std::cout << "[ Server ]  Client Disconnect, IP : " << inet_ntoa(DisconnectClientAddr.sin_addr) << ", Port : " << ntohs(DisconnectClientAddr.sin_port) << std::endl;
			
			DeleteSocket(&DisconnectClient);

			*/

			pcIOCPServer->DisconnectProcess(pstClientItem);

			break;
		}

		default:
		{
			std::cout << "[ Server ]  No Identify PacketTpye Received ..." << std::endl;

			bool Counting = PacketstatisticsProcess( pcIOCPServer, pstClientItem, NONE, TRUE );
			if ( Counting == false )
			{
				std::cout << "[ Server ]  Did Not Counting Packet" << std::endl;
			}

			return false;
		}
	}

	return true;
}

bool cIOCPServer::PacketstatisticsProcess(cIOCPServer* pcIOCPServer, stClient_Item* pstClientItem, PacketType Type, bool Result)
{
	int Index = 0;

	for ( int i = 0; i < pcIOCPServer->m_cLog.PacketStatisticsList.size(); ++i )
	{
		if ( pcIOCPServer->m_cLog.PacketStatisticsList[i].ConnectedClientID == pstClientItem->m_ClientID )
		{
			Index = i;
			break;
		}
	}


	switch ( Type )
	{
		case LOGIN_REQUEST:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Recv[0][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSARecv 'LOGIN_REQUEST' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case LOGIN_SUCCESS:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[0][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'LOGIN_SUCCESS' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case LOGIN_FAIL:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[1][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'LOGIN_SUCCESS' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case LOGOUT_ANOTHER:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[2][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'LOGOUT_ANOTHER' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case CREATE_USER_REQUEST:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Recv[1][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSARecv 'CREATE_USER_REQUEST' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case CREATE_USER_SUCCESS:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[3][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'CREATE_USER_SUCCESS' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case CREATE_USER_FAIL:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[4][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'CREATE_USER_FAIL' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case MY_STONE_PUT:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Recv[2][(int)Result] += 1;
			pcIOCPServer->m_cLog.WriteLogFile("WSARecv 'MY_STONE_PUT' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case ANOTHER_STONE_PUT:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[5][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'ANOTHER_STONE_PUT' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case MY_GAME_TIME_OUT:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Recv[3][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSARecv 'MY_GAME_TIME_OUT' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case ANOTHER_GAME_TIME_OUT:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[6][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'ANOTHER_GAME_TIME_OUT' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case GAME_START_REQUEST:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Recv[4][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSARecv 'GAME_START_REQUEST' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case GAME_START_SUCCESS:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[7][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'GAME_START_SUCCESS' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case GAME_START_FAIL:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[8][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'GAME_START_FAIL' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case GAME_END:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Recv[5][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSARecv 'GAME_END' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case MAKE_ROOM_REQUEST:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Recv[6][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSARecv 'MAKE_ROOM_REQUEST' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case MAKE_ROOM_SUCCESS:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[9][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'MAKE_ROOM_SUCCESS' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case MAKE_ROOM_FAIL:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[10][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'MAKE_ROOM_FAIL' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case ALREADY_MAKE_ROOM:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[11][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'ALREADY_MAKE_ROOM' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case ANOTHER_MAKE_ROOM:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[12][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'ANOTHER_MAKE_ROOM' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case JOIN_ROOM_REQUEST:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Recv[7][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSARecv 'JOIN_ROOM_REQUEST' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case JOIN_ROOM_SUCCESS:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[13][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'JOIN_ROOM_SUCCESS' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case JOIN_ROOM_FAIL:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[14][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'JOIN_ROOM_FAIL' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case ANOTHER_JOIN_ROOM:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[15][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'ANOTHER_JOIN_ROOM' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case ALREADY_JOIN_ROOM:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[16][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'ALREADY_JOIN_ROOM' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case MY_CHAT:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Recv[8][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSARecv 'MY_CHAT' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case ANOTHER_CHAT:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[17][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'ANOTHER_CHAT' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case MAKE_ERROR:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Recv[9][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSARecv 'MAKE_ERROR' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();
			
			break;
		}

		case PROGRAM_SHUTDOWN:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[18][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'PROGRAM_SHUTDOWN' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case CHECK_ALIVE_REQUEST:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Recv[10][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSARecv 'CHECK_ALIVE_REQUEST' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case CHECK_ALIVE:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Send[19][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSASend 'CHECK_ALIVE' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case STILL_ALIVE:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Recv[11][(int)Result] += 1;
			pcIOCPServer->m_cLog.WriteLogFile("WSARecv 'STILL_ALIVE' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case PROGRAM_QUIT:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Recv[12][(int)Result] += 1;
			pcIOCPServer->m_cLog.WriteLogFile("WSARecv 'PROGRAM_QUIT' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		case NONE:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.PacketStatisticsList[Index].Recv[13][(int)Result] += 1;			
			pcIOCPServer->m_cLog.WriteLogFile("WSARecv 'NOT INDENTIFY' Packet Add Count Complete");

			pcIOCPServer->m_cCriticalSection.UnLock();

			break;
		}

		default:
		{
			pcIOCPServer->m_cCriticalSection.Lock();

			pcIOCPServer->m_cLog.WriteLogFile("PacketStatisticsProcess() is Failed...");

			pcIOCPServer->m_cCriticalSection.UnLock();

			return false;
		}
	}

	return true;
}

void cIOCPServer::DestroyThread()
{
	for ( int i = 0; i < MAX_THREAD_COUNT; i++ )
	{
		// IOCP에 대기중엔 WorkerThread 종료 요청
		PostQueuedCompletionStatus( m_hIOCP,
									0, 
									0, 
									NULL );	
	}

	for ( int i = 0; i < MAX_THREAD_COUNT; i++ )
	{
		CloseHandle( m_hWorkerThread[i] );						// IOCP에 대기중인 WorkerThread들 Close요청
		WaitForSingleObject( m_hWorkerThread[i], INFINITE );	// 종료처리 완료 이벤트 대기
	}

	m_AcceptRunning = false;								// AcceptThread 종료
	m_SystemStatusRunning = false;							// SystemstatusThread 종료

	closesocket( m_pCNetworkSession->GetListenSocket() );	// Listen소켓 Close요청
	WaitForSingleObject( m_hAccepterThread, INFINITE );		// AcceptThread 종료 완료 이벤트 대기
	WaitForSingleObject( m_hSystemStatusThread, INFINITE );	// SystemstatusThread 종료 완료 이벤트 대기

	std::cout << "[ Server ]  All Thread Destroy Success!" << std::endl;

	m_cCriticalSection.Lock();
	m_cLog.WriteLogFile("All Thread Destroy Success");
	m_cCriticalSection.UnLock();
}

void cIOCPServer::DeleteSocket(stClient_Item* pstClientItem)
{
	struct linger stLinger = { 0, }; // Linger 옵션 생성

	shutdown( pstClientItem->m_ClientSocket, SD_BOTH ); // 소켓 데이터 송신, 수신 모두 중단

	setsockopt( pstClientItem->m_ClientSocket, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof( stLinger ) ); // linger 옵션 설정

	closesocket( pstClientItem->m_ClientSocket ); // 소켓을 닫고 Graceful Shutdown 진행

	pstClientItem->m_ClientSocket = INVALID_SOCKET; // 소켓 핸들 초기화

	m_cCriticalSection.Lock();
	m_cLog.WriteLogFile("DeleteSocket() Complete");
	m_cCriticalSection.UnLock();
}