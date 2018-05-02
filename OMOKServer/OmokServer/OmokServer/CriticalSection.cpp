#include "CriticalSection.h"

cCriticalSection::cCriticalSection()
{
	InitializeCriticalSection( &m_cs );
}

cCriticalSection::~cCriticalSection()
{
	DeleteCriticalSection( &m_cs );
}

void cCriticalSection::Lock()
{
	EnterCriticalSection( &m_cs );
}

void cCriticalSection::UnLock()
{
	LeaveCriticalSection( &m_cs );
}