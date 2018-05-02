#pragma once

#include <iostream>
#include <Windows.h>

template<class T, int ALLOC_BLOCK_SIZE = 50>
class cMemoryPool
{
public:
	static char* m_pFree;

public:

	static void Allocate()
	{
		m_pFree = new char[sizeof(T)* ALLOC_BLOCK_SIZE];

		char** ppCurrent = reinterpret_cast<char**>(m_pFree);
		char* pNext = m_pFree;

		for (int i = 0; i < ALLOC_BLOCK_SIZE; i++)
		{
			pNext += sizeof(T);
			*ppCurrent = pNext;
			ppCurrent = reinterpret_cast<char**>(pNext);
		}

		*ppCurrent = 0;
	}

public:

	static void* operator new(std::size_t AllocSize)
	{
		if (!m_pFree)
		{
			Allocate();
		}

		char* pReturn = m_pFree;

		m_pFree = *reinterpret_cast<char**>(pReturn);

		return pReturn;
	}

	static void operator delete(void* pDelete)
	{
		*reinterpret_cast<char**>(pDelete) = m_pFree;

		m_pFree = static_cast<char*>(pDelete);
	}

public:
	~cMemoryPool()
	{

	}
};

template<class T, int ALLOC_BLOCK_SIZE>
char* cMemoryPool<T, ALLOC_BLOCK_SIZE>::m_pFree;