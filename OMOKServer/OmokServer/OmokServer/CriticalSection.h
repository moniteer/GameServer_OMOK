#pragma once

#include <WinSock2.h>

class cCriticalSection
{
public:
	CRITICAL_SECTION m_cs;
	
	cCriticalSection();
	~cCriticalSection();

	void Lock();
	void UnLock();
};