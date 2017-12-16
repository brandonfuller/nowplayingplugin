// NowPlaying.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "resource.h"

#include <atlbase.h>
CComModule _Module; // dummy required to utilize atlcom.h
#include <atlcom.h>

#include "iTunesVisualAPI.h"
#include "Defines.h"
#include "strsafe.h"
#include "Utilities.h"
#include "AmLog.h"
#include "gen.h"
#include "wa_ipc.h"
#include <io.h>

int init();
void config();
void quit();

winampGeneralPurposePlugin plugin = {GPPHDR_VER, "Now Playing v0.0.0.0", init, config, quit,};

HANDLE g_hThreadCheck = NULL;
HANDLE g_hStopEventLocal = NULL;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}

DWORD WINAPI CheckThreadFunc(LPVOID lpParam) 
{
	int isPlayingLast = 0;
	int isPlaying = 0;
	int currentPos = -1;
	int lastPos = -1;
	DWORD dwSignal = WAIT_TIMEOUT;
	char szTrack[MAX_PATH] = {0};

	USES_CONVERSION;

	while ( dwSignal != WAIT_FAILED && dwSignal != WAIT_OBJECT_0 )
	{
		dwSignal = WaitForSingleObject( g_hStopEventLocal, 1000 );
		if ( dwSignal == WAIT_TIMEOUT )
		{
			isPlaying = SendMessage( plugin.hwndParent, WM_WA_IPC, 0, IPC_ISPLAYING );
			if ( isPlaying == 1 )
			{	
				currentPos = SendMessage( plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTPOS );
				if ( currentPos != lastPos )
				{			
					char *pMp3file = (char *)SendMessage( plugin.hwndParent, WM_WA_IPC, currentPos, IPC_GETPLAYLISTFILE );
					if ( pMp3file )
					{
						StringCbCopy( szTrack, sizeof( szTrack ), pMp3file );

						AMLOGDEBUG( "New song = %s", pMp3file );

						CPlaylistItem x;

						x.m_trackInfo.numTracks = PROP_UNSUPPORTED;
						x.m_trackInfo.discNumber = PROP_UNSUPPORTED;
						x.m_trackInfo.numDiscs = PROP_UNSUPPORTED;

						char szValue[MAX_PATH] = {0};
						static extendedFileInfoStruct xstruct;

						xstruct.filename = &szTrack[0];
						xstruct.ret = szValue;
						xstruct.retlen = sizeof( szValue );

						//
						// Album
						//

						xstruct.metadata = "Album";
						SendMessage( plugin.hwndParent, WM_WA_IPC, (int) &xstruct, IPC_GET_EXTENDED_FILE_INFO );
						StringCbCopyW( (LPWSTR) &x.m_trackInfo.album[1], sizeof( x.m_trackInfo.album ) - 1, A2W( szValue ) );
						x.m_trackInfo.album[0] = strlen( szValue );

						//
						// Title
						//

						xstruct.metadata = "Title";
						SendMessage( plugin.hwndParent, WM_WA_IPC, (int) &xstruct, IPC_GET_EXTENDED_FILE_INFO );
						StringCbCopyW( (LPWSTR) &x.m_trackInfo.name[1], sizeof( x.m_trackInfo.name ) - 1, A2W( szValue ) );
						x.m_trackInfo.name[0] = strlen( szValue );
						
						//
						// Artist
						//

						xstruct.metadata = "Artist";
						SendMessage( plugin.hwndParent, WM_WA_IPC, (int) &xstruct, IPC_GET_EXTENDED_FILE_INFO );
						StringCbCopyW( (LPWSTR) &x.m_trackInfo.artist[1], sizeof( x.m_trackInfo.artist ) - 1, A2W( szValue ) );
						x.m_trackInfo.artist[0] = strlen( szValue );
						
						//
						// Comments
						//

						xstruct.metadata = "Comment";
						SendMessage( plugin.hwndParent, WM_WA_IPC, (int) &xstruct, IPC_GET_EXTENDED_FILE_INFO );
						StringCbCopyW( (LPWSTR) &x.m_trackInfo.comments[1], sizeof( x.m_trackInfo.comments ) - 1, A2W( szValue ) );
						x.m_trackInfo.comments[0] = strlen( szValue );

						//
						// Genre
						//

						xstruct.metadata = "Genre";
						SendMessage( plugin.hwndParent, WM_WA_IPC, (int) &xstruct, IPC_GET_EXTENDED_FILE_INFO );
						StringCbCopyW( (LPWSTR) &x.m_trackInfo.genre[1], sizeof( x.m_trackInfo.genre ) - 1, A2W( szValue ) );
						x.m_trackInfo.genre[0] = strlen( szValue );

						//
						// Composer
						//

						xstruct.metadata = "Composer";
						SendMessage( plugin.hwndParent, WM_WA_IPC, (int) &xstruct, IPC_GET_EXTENDED_FILE_INFO );
						StringCbCopyW( (LPWSTR) &x.m_trackInfo.composer[1], sizeof( x.m_trackInfo.composer ) - 1, A2W( szValue ) );
						x.m_trackInfo.composer[0] = strlen( szValue );

						//
						// Year
						//

						xstruct.metadata = "Year";
						SendMessage( plugin.hwndParent, WM_WA_IPC, (int) &xstruct, IPC_GET_EXTENDED_FILE_INFO );
						x.m_trackInfo.year = atoi( szValue );

						//
						// Bitrate
						//

						xstruct.metadata = "Bitrate";
						SendMessage( plugin.hwndParent, WM_WA_IPC, (int) &xstruct, IPC_GET_EXTENDED_FILE_INFO );
						x.m_trackInfo.bitRate = atoi( szValue );

						//
						// Length
						//

						xstruct.metadata = "Length";
						SendMessage( plugin.hwndParent, WM_WA_IPC, (int) &xstruct, IPC_GET_EXTENDED_FILE_INFO );
						x.m_trackInfo.totalTimeInMS = atoi( szValue );

						//
						// Track
						//

						xstruct.metadata = "Track";
						SendMessage( plugin.hwndParent, WM_WA_IPC, (int) &xstruct, IPC_GET_EXTENDED_FILE_INFO );
						x.m_trackInfo.trackNumber = atoi( szValue );						

						//
						// File
						//
						
						StringCbCopyW( (LPWSTR) &x.m_trackInfo.fileName[1], sizeof( x.m_trackInfo.fileName ) - 1, A2W( pMp3file ) );
						x.m_trackInfo.fileName[0] = strlen( pMp3file );

						//
						// Send it off
						//

						x.m_fSet = true;
						DoPlay( &x );
					}

					lastPos = currentPos;
				}
			}
			else if ( isPlaying == 0 && isPlayingLast == 1 )
			{
				DoStop();
			}

			isPlayingLast = isPlaying;
		}
	}

    return 0; 
}

int init()
{
	static char szDescription[MAX_PATH] = {0};
	char szPath[MAX_PATH] = {0};
	char szPathDefault[MAX_PATH] = {0};
	char szPathIni[MAX_PATH] = {0};

	if ( GetModuleFileName( NULL, &szPath[0], sizeof( szPath ) ) )
	{
		PathRemoveFileSpec( szPath );

		StringCbCopy( szPathDefault, sizeof( szPathDefault ), szPath );
		StringCbCopy( szPathIni, sizeof( szPathIni ), szPath );

		PathAppend( szPathDefault, "Plugins" );
		PathAppend( szPathIni, "winamp.ini" );

		GetPrivateProfileString( "Winamp", "VISDir", szPathDefault, &szPath[0], sizeof( szPath ), szPathIni );

		PathAppend( szPath, "gen_BrandonFullerNowPlaying.dll" );

		if ( DoInit( MEDIA_PLAYER_WINAMP, szPath ) )
		{
			char szVersion[MAX_PATH] = {0};
		
			GetMyVersion( szVersion, sizeof( szVersion ) );

			StringCbPrintf( szDescription, sizeof( szDescription ), "Now Playing v%s", szVersion );

			plugin.description = szDescription;

			//
			// Fire off the check thread
			//

			g_hStopEventLocal = CreateEvent( NULL, TRUE, FALSE, NULL );

   			DWORD dwThreadId = 0;
   			DWORD dwThrdParam = 1;

			g_hThreadCheck = CreateThread( NULL, 0, CheckThreadFunc, &dwThrdParam, 0, &dwThreadId );
			if ( g_hThreadCheck == NULL ) 
			{
				return false;
			}

			return 0;
		}
	}
				
	return 1;
}

void config()
{
	DoPropertySheet();
}

void quit()
{
	DWORD dwExitCode = 0;

	SetEvent( g_hStopEventLocal );

	while ( GetExitCodeThread( g_hThreadCheck, &dwExitCode ) )
	{
		if ( dwExitCode == STILL_ACTIVE )
		{
			AMLOGDEBUG( "Waiting for CheckThreadFunc to exit" );

			Sleep( 500 );		
		}
		else
		{
			break;
		}
	}

	DoCleanup();
}

#ifdef __cplusplus
BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
	__declspec( dllexport ) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin()
	{
		return &plugin;
	}
#ifdef __cplusplus
}
#endif

bool ExportArtwork(LPTSTR lpszFileLocal, size_t cbFileLocal, LPTSTR lpszFileRemote, size_t cbFileRemote)
{
	return false;
}
