#include "amlog.h"
#include <time.h>
#include "strsafe.h"

CAmLog g_AmLog;

CAmLog::CAmLog()
{
	m_FileName[0] = '\0';
	m_LogLevel = CAmLoglevelNothing;
	m_File = NULL;
	InitializeCriticalSection(&m_cs);
}

CAmLog::~CAmLog()
{
	if (m_File != NULL)
	{
		fclose(m_File);
	}

	DeleteCriticalSection( &m_cs );
}

bool CAmLog::SetFileName(TCHAR* FileName)
{
	if ( m_File != NULL )
	{
		fclose( m_File );
	}

	StringCbCopy( m_FileName, sizeof( m_FileName ), FileName );

	//
	// Empty the logfile
	//
	
	m_File = fopen( m_FileName, "w" );
	if ( m_File )
	{
		fclose( m_File );
	}

	//
	// Open the logfile
	//

	m_File = fopen( m_FileName, "ab" );
	if ( m_File == NULL )
	{
		return false;
	}

	return true;
}

void CAmLog::SetSourceFileName(char *filename)
{
	//strip the path from the filename...
	char *mid = filename + strlen(filename);
	while (mid > filename)
	{
		if (*(--mid) == '\\')
		{
			mid++;
			break;
		}
	}

	StringCbCopy( m_SourceFile, sizeof( m_SourceFile ), mid );
}

void CAmLog::SetLogLevel(CAmLogLevels LogLevel)
{
	m_LogLevel = LogLevel;
}

CAmLogLevels CAmLog::GetLogLevel()
{
	return m_LogLevel;
}

void CAmLog::LogNow(TCHAR *LoglevelName, TCHAR *LogString)
{
	if ( m_File == NULL )
	{
		return;
	}

	//get the current date and time, and format it to the format we wanna use...
	time_t now;
	time(&now);
	struct tm *tmnow = localtime(&now);
	char strnow[25];
	strftime(strnow, 24, "%Y-%m-%d %H:%M:%S", tmnow);

#ifdef _UNICODE
	if (m_LogLevel == CAmLoglevelDeveloperInfo)
	{
		fprintf(m_File, "%s\t%S\t%s, %d\t%S\r\n", strnow, LoglevelName, m_SourceFile, m_LineNumber, LogString);
	}
	else
	{
		fprintf(m_File, "%s\t%S\t%S\r\n", strnow, LoglevelName, LogString);
	}
#else
	if ( m_LogLevel == CAmLoglevelDebug )
	{
		fprintf( m_File, "%s\t%s\t%s, %d\t0x%X\t%s\r\n", strnow, LoglevelName, m_SourceFile, m_LineNumber, ::GetCurrentThreadId(), LogString );
	}
	else
	{
		fprintf( m_File, "%s\t%s\t0x%X\t%s\r\n", strnow, LoglevelName, ::GetCurrentThreadId(),  LogString );
	}
#endif

	fflush( m_File );

#ifdef _DEBUG
	TCHAR mid[1025] = {0};
	StringCbPrintf( mid, sizeof( mid ), "%s", LogString );
	OutputDebugString( mid );
#endif
}

void CAmLog::ReplaceCRLF(TCHAR *s)
{
	TCHAR *mid = s;

	while ( *mid != '\0' )
	{
		switch (*mid)
		{
			case '\r':
				*mid = '|';
				break;
			case '\n':
				*mid = '|';
				break;
		}
		
		mid++;
	}
}

void CAmLog::LogInfo(TCHAR *format, ...)
{
	if ( m_LogLevel == CAmLoglevelDebug || m_LogLevel == CAmLoglevelInfo )
	{
		//never corrupt the last error value...
		DWORD LastError = GetLastError();
		//do the actual logging...
		TCHAR mid[1025] = {0}; //the wvsprintf function never puts more than 1024 bytes in a string...
		va_list args;
		va_start(args, format);
		StringCbVPrintf( mid, sizeof( mid ), format, args);
		ReplaceCRLF(mid);
		LogNow( "Info", mid );
		va_end(args);
		SetLastError(LastError);
	}

	LeaveCriticalSection(&m_cs);
}

void CAmLog::LogError(TCHAR *format, ...)
{
	if ( m_LogLevel == CAmLoglevelDebug || m_LogLevel == CAmLoglevelInfo || m_LogLevel == CAmLoglevelError )
	{
		//never corrupt the last error value...
		DWORD LastError = GetLastError();
		//do the actual logging...
		TCHAR mid[1025] = {0}; //the wvsprintf function never puts more than 1024 bytes in a string...
		va_list args;
		va_start(args, format);
		StringCbVPrintf( mid, sizeof( mid ), format, args);
		ReplaceCRLF(mid);
		LogNow( "Error", mid );
		va_end(args);
		SetLastError(LastError);
	}

	LeaveCriticalSection(&m_cs);
}

void CAmLog::LogDebug(TCHAR *format, ...)
{
	if ( m_LogLevel == CAmLoglevelDebug )
	{
		//never corrupt the last error value...
		DWORD LastError = GetLastError();
		//do the actual logging...
		TCHAR mid[1025] = {0}; //the wvsprintf function never puts more than 1024 bytes in a string...
		va_list args;
		va_start(args, format);
		StringCbVPrintf( mid, sizeof( mid ), format, args );
		ReplaceCRLF(mid);
		LogNow( "Debug", mid );
		va_end(args);
		SetLastError(LastError);
	}

	LeaveCriticalSection( &m_cs );
}

