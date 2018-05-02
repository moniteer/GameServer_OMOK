#include "Log.h"
#include <strsafe.h>
#include <fstream>
#include <sstream>
#include <tchar.h>

cLog::cLog() : hFile(nullptr)
{
	hFile = nullptr;

	ZeroMemory(szTime, sizeof(szTime));
	ZeroMemory(&m_SystemTime, sizeof(SYSTEMTIME));
	ZeroMemory(&PacketStatistics, sizeof(stPacketStatistics));
	ZeroMemory(Buff, sizeof(Buff));
	
	PacketStatisticsList.reserve(50);
}

cLog::~cLog()
{
	CloseHandle(hFile);
	hFile = NULL;

	if ( !PacketStatisticsList.empty() )
	{
		PacketStatisticsList.clear();
	}
}

int cLog::GetEmptyPacketStatisticsList() // [] 연산자로 참조할 Index 반환 함수
{
	if (PacketStatisticsList.empty())
	{
		return 0;
	}
	else
	{
		return PacketStatisticsList.size();
	}
}

void cLog::GetTime(char* szTime)
{
	sprintf( szTime, 
			 "TIME : %04d.%02d.%02d - %02d:%02d:%02d:%03d",
		 	 m_SystemTime.wYear, 
			 m_SystemTime.wMonth, 
			 m_SystemTime.wDay,
			 m_SystemTime.wHour, 
			 m_SystemTime.wMinute, 
			 m_SystemTime.wSecond, 
			 m_SystemTime.wMilliseconds );
}

bool cLog::CreateLogFile()
{
	GetSystemTime(&m_SystemTime);

	m_wLastDay = m_SystemTime.wDay;

	

	StringCbPrintf(Buff, _countof(Buff), L"%s_%04d.%02d.%02d.txt", L"LogFile", m_SystemTime.wYear, m_SystemTime.wMonth, m_SystemTime.wDay);

	hFile = CreateFile( Buff,
						GENERIC_WRITE,
						FILE_SHARE_READ,
						0,
						CREATE_ALWAYS,
						FILE_ATTRIBUTE_NORMAL,
						0 );

	if (hFile == INVALID_HANDLE_VALUE)
	{
		std::cout << "[ Server ]  CreateFile() Failed..." << std::endl;
		return false;
	}

	return true;
}

void cLog::WriteLogFile(char* szMessage)
{
	DWORD Result;
	GetSystemTime(&m_SystemTime);
/*
	if (m_wLastDay != m_SystemTime.wDay)
	{
		CloseHandle(hFile);
		hFile = nullptr;

		bool bCreate = CreateLogFile();
		if (bCreate == false)
		{
			std::cout << "[ Server ]  CreateLogFile Next Day is Fail" << std::endl;
		}
	}*/

	
	GetTime(szTime);
	
	std::string Message = szMessage;
	std::string Time = szTime;
	std::string LogData = "[ Server ]  " + Time + " -> " + Message + "\r\n";

	WriteFile(hFile, LogData.c_str(), strlen(LogData.c_str()), &Result, NULL);	
}

bool cLog::WritePacketStatisticsFile()
{	
	std::ofstream FileOut("PacketStatistics.txt", std::ios::app);

	char Buff[128];
	ZeroMemory(Buff, sizeof(Buff));

	int AllSuccessTotal = 0;
	int AllFailedTotal = 0;

	if (FileOut.is_open() == true)
	{
		for (int i = 0; i < PacketStatisticsList.size(); i++)
		{
			int AllSendSuccessTotal = 0;
			int AllSendFailedTotal = 0;
			int AllRecvSuccessTotal = 0;
			int AllRecvFailedTotal = 0;
			int Index = i;

			for (int i = 0; i < MAX_SEND_STATISTICS; i++)
			{
				AllSendFailedTotal += PacketStatisticsList[Index].Send[i][0];
				AllSendSuccessTotal += PacketStatisticsList[Index].Send[i][1];
			}

			for (int i = 0; i < MAX_RECV_STATISTICS; i++)
			{
				AllRecvFailedTotal += PacketStatisticsList[Index].Recv[i][0];
				AllRecvSuccessTotal += PacketStatisticsList[Index].Recv[i][1];
			}

			AllSuccessTotal += AllSendSuccessTotal;
			AllSuccessTotal += AllRecvSuccessTotal;
			AllFailedTotal += AllSendFailedTotal;
			AllFailedTotal += AllRecvFailedTotal;

			FileOut << "==========================================================================================\n";
			FileOut << "|  Packet Statistics Result                                                              |\n";
			FileOut << "==========================================================================================\n";;

			sprintf(Buff, "|  Client ID : %03d                                                                       |\n", PacketStatisticsList[i].ConnectedClientID);
			FileOut << Buff;

			FileOut << "------------------------------------------------------------------------------------------\n";
			FileOut << "|              Client -> Server             |              Server -> Client              |\n";
			FileOut << "------------------------------------------------------------------------------------------\n";
			FileOut << "|        PacketType        | Success | Fail |        PacketType         | Success | Fail |\n";
			FileOut << "------------------------------------------------------------------------------------------\n";

			sprintf(Buff, "|  1. Login_Request        |   %03d   |  %03d |  1. Login_Success         |   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Recv[0][1], PacketStatisticsList[Index].Recv[0][0], PacketStatisticsList[Index].Send[0][1], PacketStatisticsList[Index].Send[0][0]);
			FileOut << Buff;

			sprintf(Buff, "|  2. Create_User_Request  |   %03d   |  %03d |  2. Login_Fail            |   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Recv[1][1], PacketStatisticsList[Index].Recv[1][0], PacketStatisticsList[Index].Send[1][1], PacketStatisticsList[Index].Send[1][0]);
			FileOut << Buff;

			sprintf(Buff, "|  3. Game_My_Stone_Put	   |   %03d   |  %03d |  3. Logout_Another        |   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Recv[2][1], PacketStatisticsList[Index].Recv[2][0], PacketStatisticsList[Index].Send[2][1], PacketStatisticsList[Index].Send[2][0]);
			FileOut << Buff;

			sprintf(Buff, "|  4. My_Game_Time_Out     |   %03d   |  %03d |  4. Create_User_Success   |   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Recv[3][1], PacketStatisticsList[Index].Recv[3][0], PacketStatisticsList[Index].Send[3][1], PacketStatisticsList[Index].Send[3][0]);
			FileOut << Buff;

			sprintf(Buff, "|  5. Game_Start_Request   |   %03d   |  %03d |  5. Create_User_Fail      |   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Recv[4][1], PacketStatisticsList[Index].Recv[4][0], PacketStatisticsList[Index].Send[4][1], PacketStatisticsList[Index].Send[4][0]);
			FileOut << Buff;

			sprintf(Buff, "|  6. Game_End             |   %03d   |  %03d |  6. Game_Another_Stone_Put|   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Recv[5][1], PacketStatisticsList[Index].Recv[5][0], PacketStatisticsList[Index].Send[5][1], PacketStatisticsList[Index].Send[5][0]);
			FileOut << Buff;

			sprintf(Buff, "|  7. Make_Room_Request    |   %03d   |  %03d |  7. Another_Game_Time_Out |   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Recv[6][1], PacketStatisticsList[Index].Recv[6][0], PacketStatisticsList[Index].Send[6][1], PacketStatisticsList[Index].Send[6][0]);
			FileOut << Buff;

			sprintf(Buff, "|  8. Join_Room_Request    |   %03d   |  %03d |  8. Game_Start_Success    |   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Recv[7][1], PacketStatisticsList[Index].Recv[7][0], PacketStatisticsList[Index].Send[7][1], PacketStatisticsList[Index].Send[7][0]);
			FileOut << Buff;

			sprintf(Buff, "|  9. My_Chat              |   %03d   |  %03d |  9. Game_Start_Fail       |   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Recv[8][1], PacketStatisticsList[Index].Recv[8][0], PacketStatisticsList[Index].Send[8][1], PacketStatisticsList[Index].Send[8][0]);
			FileOut << Buff;

			sprintf(Buff, "| 10. MakeDump_Error       |   %03d   |  %03d | 10. Make_Room_Success     |   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Recv[9][1], PacketStatisticsList[Index].Recv[9][0], PacketStatisticsList[Index].Send[9][1], PacketStatisticsList[Index].Send[9][0]);
			FileOut << Buff;

			sprintf(Buff, "| 11. Check_Alive_Request  |   %03d   |  %03d | 11. Make_Room_Fail        |   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Recv[10][1], PacketStatisticsList[Index].Recv[10][0], PacketStatisticsList[Index].Send[10][1], PacketStatisticsList[Index].Send[10][0]);
			FileOut << Buff;

			sprintf(Buff, "| 12. Still_Alive          |   %03d   |  %03d | 12. Another_Make_Room     |   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Recv[11][1], PacketStatisticsList[Index].Recv[11][0], PacketStatisticsList[Index].Send[11][1], PacketStatisticsList[Index].Send[11][0]);
			FileOut << Buff;

			sprintf(Buff, "| 13. Program_Quit         |   %03d   |  %03d | 13. Already_Make_Room     |   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Recv[12][1], PacketStatisticsList[Index].Recv[12][0], PacketStatisticsList[Index].Send[12][1], PacketStatisticsList[Index].Send[12][0]);
			FileOut << Buff;

			sprintf(Buff, "| 14. None                 |   %03d   |  %03d | 14. Join_Room_Success     |   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Recv[13][1], PacketStatisticsList[Index].Recv[13][0], PacketStatisticsList[Index].Send[13][1], PacketStatisticsList[Index].Send[13][0]);
			FileOut << Buff;

			sprintf(Buff, "|                          |         |      | 15. Join_Room_Fail        |   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Send[14][1], PacketStatisticsList[Index].Send[14][0]);
			FileOut << Buff;

			sprintf(Buff, "|                          |         |      | 16. Another_Join_Room     |   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Send[15][1], PacketStatisticsList[Index].Send[15][0]);
			FileOut << Buff;

			sprintf(Buff, "|                          |         |      | 17. Already_Join_Room     |   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Send[16][1], PacketStatisticsList[Index].Send[16][0]);
			FileOut << Buff;

			sprintf(Buff, "|                          |         |      | 18. Another_Chat          |   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Send[17][1], PacketStatisticsList[Index].Send[17][0]);
			FileOut << Buff;

			sprintf(Buff, "|                          |         |      | 19. Program_Shutdown      |   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Send[18][1], PacketStatisticsList[Index].Send[18][0]);
			FileOut << Buff;

			sprintf(Buff, "|                          |         |      | 20. Check_Alive           |   %03d   |  %03d |\n",
				PacketStatisticsList[Index].Send[19][1], PacketStatisticsList[Index].Send[19][0]);
			FileOut << Buff;

			FileOut << "|                          |         |      |                           |         |      |\n";
			FileOut << "------------------------------------------------------------------------------------------\n";

			sprintf(Buff, "|           Total          |   %03d   |  %03d |           Total           |   %03d   |  %03d |\n", AllRecvSuccessTotal, AllRecvFailedTotal, AllSendSuccessTotal, AllSendFailedTotal);
			FileOut << Buff;

			FileOut << "------------------------------------------------------------------------------------------\n\n";
		}
	}
	else
	{
		return false;
	}

	if (FileOut.is_open() == true)
	{
		FileOut << "\n==========================================================================================\n";
		sprintf(Buff, "|  ALL Tatal Packet : %05d  |  ALL Success Packet : %05d  |  ALL Failed Packet : %05d |\n", AllSuccessTotal + AllFailedTotal, AllSuccessTotal, AllFailedTotal);
		std::string Message = Buff;

		FileOut << Message;
		FileOut << "==========================================================================================\n\n";
	}
	else
	{
		return false;
	}

	return true;
}