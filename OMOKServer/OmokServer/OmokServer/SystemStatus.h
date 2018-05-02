#pragma once

#include <Windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <Psapi.h>

#pragma comment(lib, "pdh.lib")

#define STATUS_BUFF_SIZE 256

class cSystemStatus
{
public:
	HANDLE m_hFile;
	SYSTEMTIME m_SystemTime;
	char StatusBuff[STATUS_BUFF_SIZE];

	HQUERY m_hQuery;
	size_t m_CPUCoreCount;
	HCOUNTER m_hCounterCPUTotal;
	HCOUNTER* m_phCounterCPUCore;
	MEMORYSTATUSEX m_MemoyInformation;
	PROCESS_MEMORY_COUNTERS_EX m_ProcessMemoryCounters;

public:
	cSystemStatus();
	~cSystemStatus();

	void Init();
	void Update();
	void Terminate();

	inline size_t GetCPUCount() { return m_CPUCoreCount; }
	bool GetCPUStatus(LONG &Total, LONG* ArrCore, size_t ArryCoreSize);
	bool GetMemoryStatus(DWORDLONG* pMemoryStatus);

	bool CreateLogFile(DWORD* pData);
	bool WriteLogFile(LONG& Cpu, LONG* pArrayCore, size_t& CpuCount, DWORDLONG* pMemoryStatus);
};