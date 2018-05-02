#include "Exception.h"
#include <fstream>
#include <iostream>
#include <DbgHelp.h>
#include <TlHelp32.h>
#include <strsafe.h>
#include "StackWalker.h"

// ToolHelp Library 사용 -> Kernel32.dll에 포함되어있는 프로세스, 모듈, 쓰레드 열거 관련 라이브러리.
// 비표준 C Library srtsafe.h 버퍼관련 오버플로우 방지를 보장하는 라이브러리

void CreateMiniDump(EXCEPTION_POINTERS* exception)
{
	wchar_t FileName[BUFF_SIZE] = { 0 };
	
	SYSTEMTIME SystemTime;
	GetSystemTime( &SystemTime );
	
	StringCbPrintf( FileName,
					_countof(FileName),
					L"%s_%4d%02d%02d_%02d%02d%02d.dmp",
					L"DumpFile",
					SystemTime.wYear,
					SystemTime.wMonth,
					SystemTime.wDay,
					SystemTime.wHour,
					SystemTime.wMinute,
					SystemTime.wSecond );

	HANDLE hFile = CreateFile( FileName,
							   GENERIC_WRITE,
							   FILE_SHARE_READ,
							   0,
							   CREATE_ALWAYS,
							   FILE_ATTRIBUTE_NORMAL,
							   0 );

	if ( hFile == INVALID_HANDLE_VALUE )
	{
		std::cout << "[ Server ]  CreateFile() Failed..." << std::endl;
		return;
	}

	MINIDUMP_EXCEPTION_INFORMATION ExceptionInfo;
	ExceptionInfo.ThreadId = GetCurrentThreadId();
	ExceptionInfo.ExceptionPointers = exception;
	ExceptionInfo.ClientPointers = FALSE;

	// 덤프파일 생성하는 함수
	MiniDumpWriteDump( GetCurrentProcess(),
					   GetCurrentProcessId(),
					   hFile,
					   MINIDUMP_TYPE( MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory | MiniDumpWithFullMemory ),
					   exception ? &ExceptionInfo : NULL,
					   NULL,
					   NULL );

	if ( hFile )
	{
		CloseHandle( hFile );
		hFile = NULL;
	}
}

// Exception 발생시 실행되는 콜백함수
LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
{
	if ( IsDebuggerPresent() )
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	THREADENTRY32 ThreadEntry32;

	DWORD MyThreadID = GetCurrentThreadId();
	DWORD MyProcessID = GetCurrentProcessId();

	// Thread SnapShot 생성
	HANDLE hThreadSnapShot = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );
	if ( hThreadSnapShot != INVALID_HANDLE_VALUE )
	{
		ThreadEntry32.dwSize = sizeof(THREADENTRY32);

		if ( Thread32First( hThreadSnapShot, &ThreadEntry32 ) )
		{
			do
			{
				// MyThreadID인 현재 쓰레드만 살아남고 나머지는 SuspendThread()
				if ( ThreadEntry32.th32OwnerProcessID == MyProcessID && ThreadEntry32.th32ThreadID != MyThreadID )
				{
					HANDLE hThread = OpenThread( THREAD_ALL_ACCESS, 
												 FALSE, 
												 ThreadEntry32.th32ThreadID );
					if ( hThread )
					{
						SuspendThread( hThread );
					}
				}
			} while ( Thread32Next( hThreadSnapShot, &ThreadEntry32 ) );
		}
	}

	// 스냅샷 처리한 핸들 닫음
	CloseHandle( hThreadSnapShot );

	std::ofstream ExceptionWrite( "Server_Exception.txt", std::ofstream::out );

	ExceptionWrite << "=============================================" << std::endl;
	ExceptionWrite << "|                                           |" << std::endl;
	ExceptionWrite << "|    Exception ERROR, CALL STACK ADDRESS    |" << std::endl;
	ExceptionWrite << "|                                           |" << std::endl;
	ExceptionWrite << "=============================================" << std::endl;

	StackWalker stackwalker;
	stackwalker.SetOutputStream( &ExceptionWrite );
	stackwalker.ShowCallstack();

	ExceptionWrite.flush();
	ExceptionWrite.close();

	// MiniDumpWriteDump로 덤프파일을 생성하는 함수
	CreateMiniDump( ExceptionInfo );

	std::cout << "[ Server ]  WorkerThreadProc Exception!" << std::endl;
	std::cout << "[ Server ]  Create Dump and System Shutdown" << std::endl;

	// 덤프남기고 콜스택남기고 프로그램 종료
	ExitProcess(1);

	return EXCEPTION_EXECUTE_HANDLER;
}