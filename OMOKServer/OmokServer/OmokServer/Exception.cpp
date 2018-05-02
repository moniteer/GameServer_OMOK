#include "Exception.h"
#include <fstream>
#include <iostream>
#include <DbgHelp.h>
#include <TlHelp32.h>
#include <strsafe.h>
#include "StackWalker.h"

// ToolHelp Library ��� -> Kernel32.dll�� ���ԵǾ��ִ� ���μ���, ���, ������ ���� ���� ���̺귯��.
// ��ǥ�� C Library srtsafe.h ���۰��� �����÷ο� ������ �����ϴ� ���̺귯��

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

	// �������� �����ϴ� �Լ�
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

// Exception �߻��� ����Ǵ� �ݹ��Լ�
LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
{
	if ( IsDebuggerPresent() )
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	THREADENTRY32 ThreadEntry32;

	DWORD MyThreadID = GetCurrentThreadId();
	DWORD MyProcessID = GetCurrentProcessId();

	// Thread SnapShot ����
	HANDLE hThreadSnapShot = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );
	if ( hThreadSnapShot != INVALID_HANDLE_VALUE )
	{
		ThreadEntry32.dwSize = sizeof(THREADENTRY32);

		if ( Thread32First( hThreadSnapShot, &ThreadEntry32 ) )
		{
			do
			{
				// MyThreadID�� ���� �����常 ��Ƴ��� �������� SuspendThread()
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

	// ������ ó���� �ڵ� ����
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

	// MiniDumpWriteDump�� ���������� �����ϴ� �Լ�
	CreateMiniDump( ExceptionInfo );

	std::cout << "[ Server ]  WorkerThreadProc Exception!" << std::endl;
	std::cout << "[ Server ]  Create Dump and System Shutdown" << std::endl;

	// ��������� �ݽ��ó���� ���α׷� ����
	ExitProcess(1);

	return EXCEPTION_EXECUTE_HANDLER;
}