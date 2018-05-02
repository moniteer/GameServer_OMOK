#pragma once

#include <Windows.h>
#include <iostream>
#include <vector>


#define LOG_BUFF_SIZE		512		// ���� ��¿� ����   
#define MAX_SEND_STATISTICS	30		// Send ��Ŷ ���� ���� ������
#define MAX_RECV_STATISTICS	20		// Recv ��Ŷ ���� ���� ������


struct stPacketStatistics
{
	DWORD ConnectedClientID;
	unsigned short Send[MAX_SEND_STATISTICS][2];	// Send ��Ŷ ��� ����� �迭
	unsigned short Recv[MAX_RECV_STATISTICS][2];	// Recv ��Ŷ ��� ����� �迭
};

class cLog
{
public:
	std::vector<stPacketStatistics> PacketStatisticsList;	// ��Ŷ ��� ������ vector
	stPacketStatistics PacketStatistics;					// stPacketStatistics ����ü �ɹ�����
	SYSTEMTIME m_SystemTime;								// �ý��� �ð� ���� ����
	wchar_t Buff[128];										// ���� �̸� ������ ����
	char szTime[64];										// ���� �ð��� ���Ŀ� ���� ������ ����
	WORD m_wLastDay;										// �Ϸ������ ���� ������ ����
	HANDLE hFile;											// LogFile�� File Handle

public:
	cLog();
	~cLog();

	bool CreateLogFile();						// LogFile �����ϴ� �Լ�
	void GetTime(char* szTime);					// �ý��� �ð� ������ �Լ�	
	void WriteLogFile(char* szMessage);			// Log ����� �Լ�


	bool WritePacketStatisticsFile();			// ��Ŷ ��� ���� �����ϴ� �Լ�
	int GetEmptyPacketStatisticsList();			// StatisticsList���� �� Index ������ �Լ�
};