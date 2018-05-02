#pragma once

#include <Windows.h>
#include <iostream>
#include <vector>


#define LOG_BUFF_SIZE		512		// 파일 출력용 버퍼   
#define MAX_SEND_STATISTICS	30		// Send 패킷 통계용 버퍼 사이즈
#define MAX_RECV_STATISTICS	20		// Recv 패킷 통계용 버퍼 사이즈


struct stPacketStatistics
{
	DWORD ConnectedClientID;
	unsigned short Send[MAX_SEND_STATISTICS][2];	// Send 패킷 통계 복사용 배열
	unsigned short Recv[MAX_RECV_STATISTICS][2];	// Recv 패킷 통계 복사용 배열
};

class cLog
{
public:
	std::vector<stPacketStatistics> PacketStatisticsList;	// 패킷 통계 저장할 vector
	stPacketStatistics PacketStatistics;					// stPacketStatistics 구조체 맴버변수
	SYSTEMTIME m_SystemTime;								// 시스템 시간 얻어올 변수
	wchar_t Buff[128];										// 파일 이름 저장할 버퍼
	char szTime[64];										// 얻어온 시간을 형식에 맞춰 저장할 버퍼
	WORD m_wLastDay;										// 하루단위로 파일 갱신할 변수
	HANDLE hFile;											// LogFile용 File Handle

public:
	cLog();
	~cLog();

	bool CreateLogFile();						// LogFile 생성하는 함수
	void GetTime(char* szTime);					// 시스템 시간 얻어오는 함수	
	void WriteLogFile(char* szMessage);			// Log 남기는 함수


	bool WritePacketStatisticsFile();			// 패킷 통계 파일 생성하는 함수
	int GetEmptyPacketStatisticsList();			// StatisticsList에서 빈 Index 얻어오는 함수
};