#include "SystemStatus.h"
#include <string>
#include <tchar.h>
#include <fstream>
#include <sstream>

cSystemStatus::cSystemStatus()
{
	m_hQuery = NULL;
	m_hCounterCPUTotal = NULL;
	m_CPUCoreCount = 0;
	m_phCounterCPUCore = nullptr;
	m_hFile = nullptr;

	ZeroMemory(&m_SystemTime, sizeof(SYSTEMTIME));
	ZeroMemory(&m_MemoyInformation, sizeof(MEMORYSTATUSEX));
	ZeroMemory(&m_ProcessMemoryCounters, sizeof(PROCESS_MEMORY_COUNTERS_EX));
	ZeroMemory(StatusBuff, sizeof(STATUS_BUFF_SIZE));
}

cSystemStatus::~cSystemStatus()
{
	m_phCounterCPUCore = nullptr;
	m_hFile = nullptr;

	CloseHandle(m_hFile);
	m_hFile = nullptr;
}

bool cSystemStatus::CreateLogFile(DWORD* pData)
{
	DWORD ProcessID = GetCurrentProcessId();
	HANDLE ProcessHandle = OpenProcess(MAXIMUM_ALLOWED, TRUE, ProcessID);

	std::ofstream FileOut("SystemStatus.txt", std::ios::app);

	if (FileOut.is_open())
	{
		FileOut << "\n==========================================================================================================\n";
		FileOut << "|                                                                                                        |\n";
		FileOut << "|                                     Server System Resource Status                                      |\n";
		FileOut << "|                                                                                                        |\n";
		FileOut << "==========================================================================================================\n";
		FileOut << "|                                   |                                                                    |\n";

		sprintf(StatusBuff, "|  * Process ID              %05d  | * Process HANDLE              %08X   * IOCP HANDLE  %08X   |\n", 
			ProcessID, (DWORD)ProcessHandle, *(pData + 14));
		FileOut << StatusBuff;
		
		FileOut << "|                                   |                                                                    |\n";

		sprintf(StatusBuff, "|  * WorkerThread[0] ID      %05d  | * WorkerThread[0] HANDLE      %08X                             |\n",
			*pData, *(pData + 7));
		FileOut << StatusBuff;

		sprintf(StatusBuff, "|  * WorkerThread[1] ID      %05d  | * WorkerThread[1] HANDLE      %08X                             |\n",
			*(pData + 1), *(pData + 8));
		FileOut << StatusBuff;

		sprintf(StatusBuff, "|  * WorkerThread[2] ID      %05d  | * WorkerThread[2] HANDLE      %08X                             |\n",
			*(pData + 2), *(pData + 9));
		FileOut << StatusBuff;

		sprintf(StatusBuff, "|  * WorkerThread[3] ID      %05d  | * WorkerThread[3] HANDLE      %08X                             |\n",
			*(pData + 3), *(pData + 10));
		FileOut << StatusBuff;

		sprintf(StatusBuff, "|  * WorkerThread[4] ID      %05d  | * WorkerThread[4] HANDLE      %08X                             |\n",
			*(pData + 4), *(pData + 11));
		FileOut << StatusBuff;

		FileOut << "|                                   |                                                                    |\n";

		sprintf(StatusBuff, "|  * AccepterThread ID       %05d  | * AccepterThread HANDLE       %08X                             |\n",
			*(pData + 5), *(pData + 12));
		FileOut << StatusBuff;

		FileOut << "|                                   |                                                                    |\n";


		sprintf(StatusBuff, "|  * SystemStatusThread ID   %05d  | * SystemStatusThread HANDLE   %08X                             |\n",
			*(pData + 6), *(pData + 13));
		FileOut << StatusBuff;

		FileOut << "|                                   |                                                                    |\n";

		FileOut << "==========================================================================================================\n\n\n";

	}
	else
	{
		return false;
	}

	return true;
}

bool cSystemStatus::WriteLogFile(LONG& Cpu, LONG* pArrayCore, size_t& CpuCount, DWORDLONG* pMemoryStatus)
{
	std::ofstream FileOut("SystemStatus.txt", std::ios::app);

	char Buff[128];
	ZeroMemory(Buff, sizeof(Buff));

	LONG cpu;

	GetSystemTime(&m_SystemTime);

	if (FileOut.is_open())
	{
		FileOut << "----------------------------------------------------------------------------------------------------------\n";

		sprintf(Buff, "|  [TIME] : %04d-%02d-%02d, %02d:%02d:%02d:%03d                                                                     |\n",
			m_SystemTime.wYear, m_SystemTime.wMonth, m_SystemTime.wDay, m_SystemTime.wHour, m_SystemTime.wMinute, m_SystemTime.wSecond, m_SystemTime.wMilliseconds);
		FileOut << Buff;

		FileOut << "----------------------------------------------------------------------------------------------------------\n";
		FileOut << "|        CPU       |                                       Memory                                        |\n";
		FileOut << "----------------------------------------------------------------------------------------------------------\n";

		sprintf(Buff, "|   CPU Use  %02d%%   |              Virtual Memory              |              Physics Memory              |\n", Cpu);
		FileOut << Buff;

		FileOut << "----------------------------------------------------------------------------------------------------------\n";

		sprintf(Buff, "|   Core(1)  %02d%%   |  All Memory                    %6dMB  |  All Memory                     ", 
			*pArrayCore, *pMemoryStatus);
		FileOut << Buff;

		std::string String = std::to_string(*(pMemoryStatus + 4)) + "MB  |\n";
		FileOut << String;

		sprintf(Buff, "|   Core(2)  %02d%%   |  Using Memory Now               %5dMB  |  Using Memory Now                ",
			*(pArrayCore + 1), *(pMemoryStatus + 1));
		FileOut << Buff;

		String = std::to_string(*(pMemoryStatus + 5)) + "MB  |\n";
		FileOut << String;

		sprintf(Buff, "|   Core(3)  %02d%%   |  Using Memory In Server         %5dMB  |  Using Memory In Server            ",
			*(pArrayCore + 2), *(pMemoryStatus + 2));
		FileOut << Buff;

		String = std::to_string(*(pMemoryStatus + 6)) + "MB  |\n";
		FileOut << String;

		sprintf(Buff, "|   Core(4)  %02d%%   |  Available Memory               %5dMB  |  Available Memory               ",
			*(pArrayCore + 3), *pMemoryStatus - *(pMemoryStatus + 1) );
		FileOut << Buff;

		String = std::to_string(*(pMemoryStatus + 4) - *(pMemoryStatus + 5)) + "MB  |\n";
		FileOut << String;

		sprintf(Buff, "|   Core(5)  %02d%%   |                                          |                                          |\n",
			*(pArrayCore + 4));
		FileOut << Buff;

		sprintf(Buff, "|   Core(6)  %02d%%   |                                          |                                          |\n",
			*(pArrayCore + 5));
		FileOut << Buff;

		double Percent1 = ( (double)*(pMemoryStatus + 2) / (double)*pMemoryStatus ) * 100;
		double Percent2 = ( (double)*(pMemoryStatus + 2) / (double)*(pMemoryStatus + 1) ) * 100;

		sprintf(Buff, "|   Core(7)  %02d%%   |  Server Use / All Memory         %5.3f%%  |  Server Use / All Memory         %5.3f%%  |\n",
			*(pArrayCore + 6), Percent1, Percent2);
		FileOut << Buff;

		Percent1 = ((double)*(pMemoryStatus + 6) / (double)*(pMemoryStatus + 4)) * 100;
		Percent2 = ((double)*(pMemoryStatus + 6) / (double)*(pMemoryStatus + 5)) * 100;

		sprintf(Buff, "|   Core(8)  %02d%%   |  Server Use / Using Memory Now   %5.3f%%  |  Server Use / Using Memory Now   %5.3f%%  |\n",
			*(pArrayCore + 7), Percent1, Percent2);
		FileOut << Buff;

		FileOut << "----------------------------------------------------------------------------------------------------------\n\n";

	}
	else
	{
		return false;
	}


	return true;
}

void cSystemStatus::Init()
{
	PdhOpenQuery(NULL, 0, &m_hQuery);

	//CPU Total
	PdhAddCounter(m_hQuery, TEXT("\\Processor(_Total)\\% Processor Time"), 1, &m_hCounterCPUTotal);

	// CPU core 정보
	SYSTEM_INFO SystemInformation = { 0 };
	GetSystemInfo(&SystemInformation);

	if (SystemInformation.dwNumberOfProcessors > 0)
	{
		m_CPUCoreCount = SystemInformation.dwNumberOfProcessors;
		m_phCounterCPUCore = new HCOUNTER[m_CPUCoreCount];

		for (int cnt = 0; cnt < m_CPUCoreCount; cnt++)
		{
			TCHAR szFullCounterPath[1024] = { 0 };

			wsprintf(szFullCounterPath, TEXT("\\Processor(%d)\\%% Processor Time"), cnt);
			PdhAddCounter(m_hQuery, szFullCounterPath, 1, &m_phCounterCPUCore[cnt]);
		}
	}
}

bool cSystemStatus::GetCPUStatus(LONG &Total, LONG* ArrCore, size_t ArryCoreSize)
{
	PDH_FMT_COUNTERVALUE PFC_Value = { 0 };

	PdhGetFormattedCounterValue(m_hCounterCPUTotal, PDH_FMT_LONG, NULL, &PFC_Value);
	Total = PFC_Value.longValue;

	if (ArrCore != nullptr && ArryCoreSize > 0)
	{
		for (size_t i = 0; i < ArryCoreSize; i++)
		{
			PDH_FMT_COUNTERVALUE PFC_Value = { 0 };
			PdhGetFormattedCounterValue(m_phCounterCPUCore[i], PDH_FMT_LONG, NULL, &PFC_Value);
			ArrCore[i] = PFC_Value.longValue;
		}
	}

	return true;
}

bool cSystemStatus::GetMemoryStatus(DWORDLONG* pMemoryStatus)
{
	ZeroMemory(&m_MemoyInformation, sizeof(MEMORYSTATUSEX));
	m_MemoyInformation.dwLength = sizeof(MEMORYSTATUSEX);

	// 전체 메모리 정보 얻어오는 함수
	GlobalMemoryStatusEx(&m_MemoyInformation);

	// 전체 Virtual Memory 
	DWORDLONG TotalVirtualMemory = m_MemoyInformation.ullTotalPageFile / (1024 * 1024);
	*pMemoryStatus = TotalVirtualMemory;

	// 지금 쓰고있는 Virtual Memory
	DWORDLONG VirtualMemoryUsed = (m_MemoyInformation.ullTotalPageFile - m_MemoyInformation.ullAvailPageFile) / (1024 * 1024);
	*(pMemoryStatus + 1) = VirtualMemoryUsed;

	ZeroMemory(&m_ProcessMemoryCounters, sizeof(m_ProcessMemoryCounters));
	m_ProcessMemoryCounters.cb = sizeof(m_ProcessMemoryCounters);

	// K32GetProcessMemoryInfo 원형, 프로세스 메모리 정보 얻어오는 함수
	GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&m_ProcessMemoryCounters, m_ProcessMemoryCounters.cb);

	// 이 프로세스가 쓰고 있는 Virtual Memory
	SIZE_T VirtualMemoryUsedByProcess = m_ProcessMemoryCounters.PrivateUsage / (1024 * 1024);
	*(pMemoryStatus + 2) = VirtualMemoryUsedByProcess;

	// 현재 남은 가상메모리 MB
	DWORDLONG AvailableVirtualMemory = (DWORDLONG)(TotalVirtualMemory - VirtualMemoryUsed) / (1024 * 1024);
	*(pMemoryStatus + 3) = AvailableVirtualMemory;
	
	// 물리메모리 부분 / (1024*1024) 날려봄 - MB

	// 전체 Physical Memory(RAM)
	DWORDLONG TotalPhysicsMemory = m_MemoyInformation.ullTotalPhys / (1024 * 1024);
	*(pMemoryStatus + 4) = TotalPhysicsMemory;

	// 현재 사용되고 있는 전체 Physical Memory
	DWORDLONG PhysicsMemoryUsed = (m_MemoyInformation.ullTotalPhys - m_MemoyInformation.ullAvailPhys) / (1024 * 1024);
	*(pMemoryStatus + 5) = PhysicsMemoryUsed;

	// 이 프로세스가 쓰고 있는  Physical Memory
	SIZE_T PhysicsMemoryUsedByProcess = m_ProcessMemoryCounters.WorkingSetSize / (1024 * 1024);
	*(pMemoryStatus + 6) = PhysicsMemoryUsedByProcess;

	return true;
}

void cSystemStatus::Update()
{
	// 카운트 갱신
	PdhCollectQueryData(m_hQuery);
}

void cSystemStatus::Terminate()
{
	PdhRemoveCounter(m_hCounterCPUTotal);

	for (int i = 0; i < m_CPUCoreCount; i++)
	{
		PdhRemoveCounter(m_phCounterCPUCore[i]);
	}
		
	delete(m_phCounterCPUCore);
}