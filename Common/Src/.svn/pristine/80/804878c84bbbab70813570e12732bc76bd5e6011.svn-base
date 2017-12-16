#include <windows.h>
#include <wininet.h>
#include <time.h>
#include <shlobj.h>

#import "msxml3.dll" 
using namespace MSXML2;

#import "wodSFTP.dll" no_namespace named_guids

#include <atlbase.h>
extern CComModule _Module; // dummy required to utilize atlcom.h
#include <atlcom.h>

#include "resource.h"
#include "Defines.h"
#include "LocalDefines.h"

#define B64_NO_NAMESPACE
#include "b64.h"
#include "sha2.h"
#include "hmac_sha256.h"

#include "Global.h"
#include "MD5.h"

#include "strsafe.h"
#include "iTunesVisualAPI.h"
#include "AmLog.h"
#include "Encrypt.h"
#include "Utilities.h"
#include "gd.h"
#include "License.h"
#include "oauth.h"

//
// My Globals
//

char g_szGuid[MAX_PATH];
char g_szVersion[MAX_PATH];
char g_szUserAgent[MAX_PATH];
int g_intMediaPlayerType = MEDIA_PLAYER_UNKNOWN;
char g_szThisDllPath[MAX_PATH];
char g_szAppWorkingDir[MAX_PATH];
HBITMAP g_hBitmap = NULL;
char g_szAlbumArtUrl[1024];
char g_szOutputFile[MAX_PATH];
int g_intOnce = 0;
int g_intAppleEnabled = 1;
int g_intAmazonEnabled = 1;
int g_intAmazonUseASIN = 0;
char g_szAmazonLocale[3];
char g_szAmazonAssociate[MAX_PATH];
char g_szAppleAssociate[MAX_PATH];
char g_szEmail[MAX_PATH];
char g_szLicenseKey[MAX_PATH];
char g_szKey[MAX_PATH];
char g_szUploadProtocol[MAX_PATH];
int g_intFTPPassive = 1;
char g_szFTPHost[MAX_PATH];
char g_szFTPUser[MAX_PATH];
char g_szFTPPassword[MAX_PATH];
char g_szFTPPath[MAX_PATH];
char g_szStyleSheet[MAX_PATH];
char g_szTrackBackUrl[MAX_PATH];
char g_szTrackBackPassphrase[MAX_PATH];
char g_szXMLEncoding[MAX_PATH];
BOOL g_fPlayedOnce = FALSE;
BOOL g_fLicensed = FALSE;
int g_intPlaylistLength = DEFAULT_PLAYLIST_LENGTH;
int g_intPublishStop = DEFAULT_PUBLISH_STOP;
int g_intPlaylistClear = DEFAULT_CLEAR_PLAYLIST;
int g_intUseXmlCData = 0;
int g_intNext = 0;
int g_intPlaylistBufferDelay = DEFAULT_SECONDS_DELAY;
int g_intSkipShort = DEFAULT_SKIPSHORT_SECONDS;
HANDLE g_hThreadPublish = NULL; 
HANDLE g_hSignalPlayEvent = NULL;
HANDLE g_hStopEvent = NULL;
CPlaylistItem* g_paPlaylist = NULL;
time_t g_timetChanged = 0;
time_t g_timeLastUpdateCheck = 0;
HWND g_hwndParent = NULL;
int g_intArtworkUpload = 0;
int g_intArtworkWidth = 160;
int g_intTwitterEnabled = DEFAULT_TWITTER_ENABLED;
char g_szTwitterMessage[MAX_PATH];
char g_szTwitterAuthKey[1024] = {0};
char g_szTwitterAuthSecret[1024] = {0};
int g_intTwitterRateLimitMinutes = TWITTER_RATE_LIMIT_MINUTES_DEFAULT;
time_t g_timetTwitterLatest = 0;
int g_intFacebookEnabled = 0;
int g_intFacebookRateLimitMinutes = FACEBOOK_RATE_LIMIT_MINUTES_DEFAULT;
char g_szFacebookMessage[1024] = {NULL};
char g_szFacebookAttachmentDescription[1024] = {NULL};
char g_szFacebookAuthToken[MAX_PATH] = {NULL};
char g_szFacebookSessionKey[MAX_PATH] = {NULL};
time_t g_timetFacebookLatest = 0;
char g_szSkipKinds[MAX_PATH] = {0};

//
// Strings
//

TCHAR g_szStringMsgErrorConnect[] = { TEXT( "Error %d occurred during %s.\n\nDetails: %s" ) };

//
// Define the SFTP events to be handled
//

_ATL_FUNC_INFO ConnectedInfo = {CC_STDCALL, VT_EMPTY, 2, {VT_I2, VT_BSTR}};
_ATL_FUNC_INFO DisconnectedInfo = {CC_STDCALL, VT_EMPTY, 0, NULL};
_ATL_FUNC_INFO ProgressInfo = {CC_STDCALL, VT_EMPTY, 2, {VT_I4, VT_I4}};
_ATL_FUNC_INFO HostFingerprintInfo = {CC_STDCALL, VT_EMPTY, 2, {VT_BSTR, VT_BOOL + VT_BYREF}};

class wFtpEvents : public IDispEventSimpleImpl<1, wFtpEvents, &DIID__IwodSFTPComEvents>
{
public:
    wFtpEvents (IwodSFTPComPtr pWodFtpCom)
    {
        m_pWodFtpCom = pWodFtpCom;
        DispEventAdvise ( (IUnknown*)m_pWodFtpCom);
    }

    virtual ~wFtpEvents ()
    {
        DispEventUnadvise ( (IUnknown*) m_pWodFtpCom );
        m_pWodFtpCom.Release();
    }

	void __stdcall Connected(short ErrorCode, BSTR ErrorText)
	{
		if ( ErrorCode == 0 )
		{
			AMLOGDEBUG( "SFTP Connected" );
		}
		else
		{
			AMLOGDEBUG( "SFTP Connected with Error %d", ErrorCode );
		}
    }

    void __stdcall Disconnected()
    {
        AMLOGDEBUG( "SFTP Disconnected" );
    }

    void __stdcall Progress(long Position, long Total)
    {
        AMLOGDEBUG( "SFTP Progress = %i of %i", Position, Total );
    }

	void __stdcall HostFingerprint(BSTR Fingerprint, VARIANT_BOOL* Accept)
	{
		USES_CONVERSION;

		AMLOGDEBUG( "SFTP Host Fingerprint = %s", OLE2T( Fingerprint ) );
	}

    BEGIN_SINK_MAP( wFtpEvents )
		SINK_ENTRY_INFO( 1, DIID__IwodSFTPComEvents, 1, Connected, &ConnectedInfo )
		SINK_ENTRY_INFO( 1, DIID__IwodSFTPComEvents, 2, Disconnected, &DisconnectedInfo )
		SINK_ENTRY_INFO( 1, DIID__IwodSFTPComEvents, 4, HostFingerprint, &HostFingerprintInfo )
        SINK_ENTRY_INFO( 1, DIID__IwodSFTPComEvents, 5, Progress, &ProgressInfo )
	END_SINK_MAP ()

private:
    IwodSFTPComPtr m_pWodFtpCom;
};

void LaunchURL(LPCTSTR lpszUrl)
{
	AMLOGDEBUG( "Launch URL = %s", lpszUrl );

	ShellExecute( NULL, "open", lpszUrl, NULL, NULL, SW_SHOW ); 
}

void MySetWindowText(HWND hDlg, int nIDDlgItem, LPCTSTR lpszValue)
{
	int nChars = MultiByteToWideChar( CP_UTF8, 0, lpszValue, -1, NULL, 0 );
	int size = nChars * sizeof( WCHAR );

	WCHAR * lpValue = (WCHAR *) malloc( size );
	if ( lpValue )
	{
		ZeroMemory( lpValue, size );

		if ( MultiByteToWideChar( CP_UTF8, 0, lpszValue, -1, lpValue, size ) > 0 )
		{
			SetWindowTextW( GetDlgItem( hDlg, nIDDlgItem ), lpValue );
		}

		free( lpValue );
	}
}

void SavePreference(HKEY hKey, LPCTSTR lpValueName, LPCTSTR lpszValue)
{
	USES_CONVERSION;

	int nChars = MultiByteToWideChar( CP_UTF8, 0, lpszValue, -1, NULL, 0 );
	int size = nChars * sizeof( WCHAR );

	WCHAR * lpValue = (WCHAR *) malloc( size );
	if ( lpValue )
	{
		ZeroMemory( lpValue, size );

		if ( MultiByteToWideChar( CP_UTF8, 0, lpszValue, -1, lpValue, size ) > 0 )
		{
			RegSetValueExW( hKey, A2W( lpValueName ), 0, REG_SZ, (BYTE*) lpValue, size );
		}

		free( lpValue );
	}
}

void SavePreference(LPCTSTR lpszKey, LPCTSTR lpszValue)
{
	HKEY hKey;
	DWORD dwDisposition = 0;

	if ( RegCreateKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition ) == ERROR_SUCCESS )
	{						
		RegSetValueEx( hKey, lpszKey, 0, REG_SZ, (BYTE*) lpszValue, strlen( lpszValue ) + sizeof( char ) );	

		RegCloseKey( hKey );
	}
}

void SavePreference(LPCTSTR lpszKey, time_t time)
{
	HKEY hKey;
	DWORD dwDisposition = 0;

	if ( RegCreateKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition ) == ERROR_SUCCESS )
	{						
		RegSetValueEx( hKey, lpszKey, 0, REG_DWORD, (BYTE*) &time, sizeof( time ) );
		RegCloseKey( hKey );
	}
}

void GetDialogPreference(HWND hDlg, int nIDDlgItem, LPTSTR lpszValue, size_t cbValue)
{
	WCHAR value[1024] = {0};

	GetDlgItemTextW( hDlg, nIDDlgItem, value, sizeof( value ) / sizeof( WCHAR ) );

	WideCharToMultiByte( CP_UTF8, 0, value, -1, lpszValue, cbValue, NULL, NULL );
}

void SetDialogPreference(HWND hDlg, int nIDDlgItem, LPTSTR lpszValue)
{
	int nChars = MultiByteToWideChar( CP_UTF8, 0, lpszValue, -1, NULL, 0 );
	int size = ( nChars + 1 ) * sizeof( WCHAR );

	WCHAR * lpValue = (WCHAR *) malloc( size );
	if ( lpValue )
	{
		ZeroMemory( lpValue, size );

		if ( MultiByteToWideChar( CP_UTF8, 0, lpszValue, -1, lpValue, size ) > 0 )
		{
			SetDlgItemTextW( hDlg, nIDDlgItem, lpValue );
		}

		free( lpValue );
	}
}

int ReadPreference(HKEY hKey, LPCTSTR lpValueName, LPTSTR lpszValue, size_t cbValue)
{
	int size = 0;
	DWORD dwBufLen = 0;

	USES_CONVERSION;

	if ( RegQueryValueExW( hKey, A2W( lpValueName ), NULL, NULL, NULL, &dwBufLen ) == ERROR_SUCCESS )
	{
		WCHAR * value = (WCHAR *) malloc( dwBufLen );
		if ( value )
		{
			ZeroMemory( value, dwBufLen );

			if ( RegQueryValueExW( hKey, A2W( lpValueName ), NULL, NULL, (LPBYTE) value, &dwBufLen ) == ERROR_SUCCESS )
			{
				size = WideCharToMultiByte( CP_UTF8, 0, value, -1, lpszValue, cbValue, NULL, NULL );
			}

			free( value );
		}
	}

	return size;
}

int ReadPreferenceInt(HKEY hKey, LPCTSTR lpValueName)
{
	int value = 0;
	int size = 0;
	DWORD dwBufLen = sizeof(value);

	USES_CONVERSION;

	if ( RegQueryValueExW( hKey, A2W( lpValueName ), NULL, NULL, (LPBYTE) &value, &dwBufLen ) == ERROR_SUCCESS )
	{
		size = value;
	}

	return size;
}

int LoadMyPreferenceString(LPCTSTR lpValueName, LPTSTR lpszValue, size_t cbValue)
{
	HKEY hKey;
	int size = 0;

	if ( RegOpenKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey ) == ERROR_SUCCESS )
	{
		size = ReadPreference( hKey, lpValueName, lpszValue, cbValue );

		RegCloseKey( hKey );
	}

	return size;
}


int LoadMyPreferenceInt(LPCTSTR lpValueName)
{
	HKEY hKey;
	int size = 0;

	if ( RegOpenKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey ) == ERROR_SUCCESS )
	{
		size = ReadPreferenceInt(hKey, lpValueName);

		RegCloseKey( hKey );
	}

	return size;
}

void Dirify(LPTSTR lpszPathOrig)
{
	LPTSTR lpszPath = lpszPathOrig;

	while ( *lpszPath != '\0' )
	{
		if ( *lpszPath == '\\' || *lpszPath == '/' || *lpszPath == ':' || *lpszPath == '*' || *lpszPath == '?' || *lpszPath == '\"' || *lpszPath == '<' || *lpszPath == '>' || *lpszPath == '|' )
		{
			*lpszPath = '_';
		}

		lpszPath++;
	}
}

int FindString(LPTSTR lpszValue, LPCTSTR lpszSub, size_t nStart)
{
	if (nStart > strlen(lpszValue))
		return -1;

	// find first matching substring
	LPTSTR lpsz = _tcsstr(lpszValue + nStart, lpszSub);

	// return -1 for not found, distance from beginning otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - lpszValue);
}

int ReplaceChar(LPTSTR lpsz, TCHAR chOld, TCHAR chNew)
{
	int nCount = 0;

	if ( chOld != chNew )
	{
		LPTSTR lpszEnd = lpsz + strlen( lpsz );
		while ( lpsz < lpszEnd )
		{
			if ( *lpsz == chOld )
			{
				*lpsz = chNew;
				nCount++;
			}
			lpsz = _tcsinc(lpsz);
		}
	}

	return nCount;
}

int ReplaceString(LPTSTR lpszValue, size_t cbValue, LPCTSTR lpszOld, LPCTSTR lpszNew)
{
	int nSourceLen = strlen(lpszOld);
	if (nSourceLen == 0)
		return 0;
	int nReplacementLen = strlen(lpszNew);

	// loop once to figure out the size of the result string
	int nCount = 0;
	LPTSTR lpszStart = lpszValue;
	LPTSTR lpszEnd = lpszValue + strlen( lpszValue );
	LPTSTR lpszTarget;
	while (lpszStart < lpszEnd)
	{
		while ((lpszTarget = _tcsstr(lpszStart, lpszOld)) != NULL)
		{
			nCount++;
			lpszStart = lpszTarget + nSourceLen;
		}
		lpszStart += lstrlen(lpszStart) + 1;
	}

	// if any changes were made, make them
	if (nCount > 0)
	{
		int nOldLength = strlen( lpszValue );
		int nNewLength =  nOldLength + (nReplacementLen-nSourceLen)*nCount;
		//if (GetData()->nAllocLength < nNewLength || GetData()->nRefs > 1)
		//{
		//	CStringData* pOldData = GetData();
		//	LPTSTR pstr = m_pchData;
		//	AllocBuffer(nNewLength);
		//	memcpy(m_pchData, pstr, pOldData->nDataLength*sizeof(TCHAR));
		//	CString::Release(pOldData);
		//}
		lpszStart = lpszValue;
		lpszEnd = lpszValue + strlen( lpszValue );

		// loop again to actually do the work
		while (lpszStart < lpszEnd)
		{
			while ( (lpszTarget = _tcsstr(lpszStart, lpszOld)) != NULL)
			{
				int nBalance = nOldLength - (lpszTarget - lpszValue + nSourceLen);
				memmove(lpszTarget + nReplacementLen, lpszTarget + nSourceLen,
					nBalance * sizeof(TCHAR));
				memcpy(lpszTarget, lpszNew, nReplacementLen*sizeof(TCHAR));
				lpszStart = lpszTarget + nReplacementLen;
				lpszStart[nBalance] = '\0';
				nOldLength += (nReplacementLen - nSourceLen);
			}
			lpszStart += lstrlen(lpszStart) + 1;
		}
	}

	return nCount;
}

void EscapeQuotes(LPTSTR lpszValue, size_t cbValue)
{
	ReplaceString(lpszValue, cbValue, "\"", "\\\"");
}

int DeleteString(LPTSTR lpsz, int nIndex, int nCount)
{
	if (nIndex < 0)
		nIndex = 0;

	int nNewLength = strlen(lpsz);
	if (nCount > 0 && nIndex < nNewLength)
	{
		//CopyBeforeWrite();
		int nBytesToCopy = nNewLength - (nIndex + nCount) + 1;

		memcpy(lpsz + nIndex, lpsz + nIndex + nCount, nBytesToCopy * sizeof(TCHAR));
	}

	return nNewLength;
}

void SubBlock(LPTSTR lpszValue, const size_t cbValue, LPCTSTR lpszTagStart, LPCTSTR lpszTagStop, bool show)
{
	if (lpszValue && lpszTagStart && lpszTagStop)
	{
		bool found = true;

		do
		{
			int nLocationStart = FindString(lpszValue, lpszTagStart, 0);
			if (nLocationStart != -1)
			{
				int nLocationStop = FindString(lpszValue, lpszTagStop, nLocationStart + strlen(lpszTagStart));
				if (nLocationStop != -1)
				{
					if (show)
					{
						DeleteString(lpszValue, nLocationStop, strlen(lpszTagStop));
						DeleteString(lpszValue, nLocationStart, strlen(lpszTagStart));
					}
					else
					{
						DeleteString(lpszValue, nLocationStart, nLocationStop - nLocationStart + strlen(lpszTagStop));
					}
					
					found = true;
				}
				else
				{
					found = false;
				}
			}
			else
			{
				found = false;
			}
		}
		while (found);
	}
}

void PrepareFilename(LPCTSTR lpszSrc, LPTSTR lpszDest, size_t cbDest)
{
	AMLOGDEBUG( "Begin PrepareFilename" );

	time_t now_t;
	struct tm now;
	time( &now_t );
	now = *localtime( &now_t );

	size_t x = strftime( lpszDest, cbDest, lpszSrc, &now );

	AMLOGDEBUG( "File = %s", lpszDest );

	AMLOGDEBUG( "End PrepareFilename" );
}

void Trace(LPCTSTR lpszFormat, ...)
{
	int nBuf = 0;
	TCHAR szBuffer[512];
	va_list args;

	va_start( args, lpszFormat );

	nBuf = StringCchVPrintfA( szBuffer, sizeof(szBuffer), lpszFormat, args );

	OutputDebugString( szBuffer );

	va_end( args );
}

LONG GetTimezoneInMinutes()
{
	TIME_ZONE_INFORMATION tzi;

	switch ( GetTimeZoneInformation( &tzi ) )
	{			
		case TIME_ZONE_ID_STANDARD:
			return tzi.Bias + tzi.StandardBias;

		case TIME_ZONE_ID_DAYLIGHT:
			return tzi.Bias + tzi.DaylightBias;

		default:
			return 0;
	}
}

BOOL CrackUrl(LPCTSTR lpszUrl, LPTSTR lpszValueHostName, size_t nValueHostName, LPTSTR lpszValueUrlPath, size_t nValueUrlPath, int& scheme)
{
	URL_COMPONENTS uc;
	
	memset( &uc, 0, sizeof(uc) );
	uc.dwStructSize = sizeof(URL_COMPONENTS);
	uc.dwHostNameLength = 1;
	uc.dwUrlPathLength = 1;
	
	if ( InternetCrackUrl( lpszUrl, strlen( lpszUrl ), 0, &uc ) )
	{
		StringCbCopyN( lpszValueHostName, nValueHostName, uc.lpszHostName, uc.dwHostNameLength );
		StringCbCopyN( lpszValueUrlPath, nValueUrlPath, uc.lpszUrlPath, uc.dwUrlPathLength );

		scheme = uc.nScheme;

		return TRUE;
	}

	return FALSE;
}

void DoGoogleAnalytics(char * lpszAction)
{
	char szVisitor[MAX_PATH] = {0};
	
	StringCbPrintf(szVisitor, sizeof(szVisitor), "%s%s", g_szGuid, GOOGLE_ANALYTICS_UTMAC);

	MD5_CTX udtMD5Context;
	unsigned char szDigest[16];
	
	MD5Init(&udtMD5Context);
	MD5Update(&udtMD5Context, (unsigned char *) szVisitor, strlen(szVisitor));
	MD5Final(szDigest, &udtMD5Context);
	
	char szDigestHex[MAX_PATH] = {0};
	StringCbPrintf(szDigestHex, sizeof(szDigestHex), "0x%02x%02x%02x%02x%02x%02x%02x%02x", szDigest[0], szDigest[1], szDigest[2], szDigest[3], szDigest[4], szDigest[5], szDigest[6], szDigest[7]);	

	HINTERNET hSession;
	HINTERNET hConnection;
	HINTERNET hData;
	DWORD dwContext = 0;
	char szPath[MAX_URL] = {NULL};
	int nScheme = INTERNET_SCHEME_HTTP;

	hSession = InternetOpen( g_szUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
	if ( hSession )
	{
		hConnection = InternetConnect( hSession, GOOGLE_ANALYTICS_HOSTNAME, nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, dwContext );
		if ( hConnection )
		{
			DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_NO_UI | INTERNET_FLAG_NO_COOKIES;

			if ( nScheme == INTERNET_SCHEME_HTTPS )
			{
				dwFlags |= INTERNET_FLAG_SECURE;
			}

			StringCbPrintf( szPath, sizeof( szPath ), "%s?utmac=%s&utmwv=%s&utmhn=%s&utmp=%s&utmcc=%s&utmn=%lu&utmvid=%s&utmr=-&utmt=event&utme=5(%s*%s*%s)", 
				GOOGLE_ANALYTICS_PATH, 
				GOOGLE_ANALYTICS_UTMAC, 
				GOOGLE_ANALYTICS_UTMWV,
				GOOGLE_ANALYTICS_UTMHN,
				GOOGLE_ANALYTICS_UTMP,
				GOOGLE_ANALYTICS_UTMCC,
				time(NULL),
				szDigestHex,
				g_intMediaPlayerType == MEDIA_PLAYER_ITUNES ? GOOGLE_ANALYTICS_CATEGORY_ITUNES : (g_intMediaPlayerType == MEDIA_PLAYER_WINAMP ? GOOGLE_ANALYTICS_CATEGORY_WINAMP : GOOGLE_ANALYTICS_CATEGORY_WMP),
				lpszAction,
				g_szVersion );

			AMLOGDEBUG("Path: %s", szPath );
		
			hData = HttpOpenRequest( hConnection, "GET", szPath, NULL, NULL, NULL, dwFlags, 0 );
			if ( hData )
			{			
				if ( HttpSendRequest( hData, NULL, 0, NULL, 0 ) )
				{
					DWORD dwStatus = 0;
					DWORD dwStatusSize = sizeof( dwStatus );

					HttpQueryInfo( hData, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, &dwStatus, &dwStatusSize, NULL );

					AMLOGDEBUG( "Response: %ld", dwStatus );

					if (dwStatus == HTTP_STATUS_OK)
					{
						char szResponse[10 * 1024] = {0};

						if ( ReadResponseBody( hData, &szResponse[0], sizeof( szResponse ) ) )
						{
						}
					}
				}
				else
				{				
					AMLOGERROR ( "Ping failed with %ld", ::GetLastError() );
				}

				InternetCloseHandle( hData );
			}
			else
			{
				AMLOGERROR( "HttpOpenRequest failed with %ld", ::GetLastError() );
			}

			InternetCloseHandle( hConnection );
		}
		else
		{
			AMLOGERROR( "InternetConnect failed with %ld", ::GetLastError() );
		}

		InternetCloseHandle( hSession );
	}
	else
	{
		AMLOGERROR( "InternetOpen failed with %ld", ::GetLastError() );
	}
}

HBITMAP LoadPicture(LPCTSTR pszPath)
{
	AMLOGDEBUG( "Begin LoadPicture" );

	HRESULT hr = S_OK;
	WCHAR wpath[MAX_PATH];
	IPicture* pPic = NULL;
	HBITMAP hPic = NULL;
	HBITMAP hPicRet = NULL;
	
	if ( MultiByteToWideChar( CP_ACP, 0, pszPath, -1, wpath, MAX_PATH ) > 0 )
	{
		hr = OleLoadPicturePath( wpath, NULL, NULL, NULL, IID_IPicture, (LPVOID*) &pPic );

		if ( SUCCEEDED( hr ) && pPic )
		{
			pPic->get_Handle( (UINT*) &hPic );

			hPicRet = (HBITMAP) CopyImage( hPic, IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG );

			pPic->Release();			
		}
	}

	AMLOGDEBUG( "End LoadPicture" );

	return hPicRet;
}

BOOL MyPathRemoveFileSpec(LPTSTR lpszPath)
{
	LPTSTR lpszFound = NULL;

	if ( lpszPath )
	{
		lpszFound = strrchr( lpszPath, '/' );
		if ( lpszFound )
		{
			lpszFound++;
			*lpszFound = '\0';
		}
		else
		{
			lpszFound = strrchr( lpszPath, '\\' );
			if ( lpszFound )
			{
				lpszFound++;
				*lpszFound = '\0';
			}
			else
			{
				*lpszPath = '\0';
			}
		}
	}

	return TRUE;
}

BOOL MyCreateDirectory(LPCTSTR lpszFolder)
{
	AMLOGDEBUG( "MyCreateDirectory for %s", lpszFolder );

	if ( !lpszFolder || !lstrlen( lpszFolder ) )
	{
		return FALSE;
	}

	DWORD dwAttrib = GetFileAttributes( lpszFolder );

	//
	// Already exists ?
	//

	if ( dwAttrib != 0xffffffff )
	{
		return (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
	}

	//
	// Recursively create from the top down
	//

	char* szPath = _strdup( lpszFolder );

	char* p = strrchr( szPath, '\\' );

	if ( p )
	{
		// The parent is a dir, not a drive
		*p = '\0';

		// if can't create parent
		if ( ! MyCreateDirectory( szPath ) )
		{
			free( szPath );

			return FALSE;
		}

		free( szPath );

		if ( !::CreateDirectory( lpszFolder, NULL ) ) 
		{
			AMLOGERROR( "Error %d: Failed to create directory \"%s\"", ::GetLastError(), lpszFolder );

			return FALSE;
		}
		else
		{
			AMLOGINFO( "CreateDirectory = %s", lpszFolder );
		}
	}

	return TRUE;
}

bool IsExtension(LPCTSTR lpszString, LPCTSTR lpszEnd)
{
	LPTSTR lpszPos = strrchr( lpszString, '.' );
	if ( lpszPos )
	{
		lpszPos++;
		
		if ( strlen( lpszPos ) > 0 && stricmp( lpszPos, lpszEnd ) == 0 )
		{
			return true;
		}
	}

	return false;
}

bool RemoveParentDirectory(LPCTSTR lpszFile)
{
	TCHAR szDir[MAX_PATH];

	StringCbCopy( szDir, sizeof( szDir ), lpszFile );

	LPTSTR lpszEnd = strrchr( szDir, '\\' );
	if ( lpszEnd )
	{
		*lpszEnd = '\0';
	}

	if ( RemoveDirectory( szDir ) )
	{
		AMLOGINFO( "RemoveDirectory = %s", szDir );

		RemoveParentDirectory( szDir );

		return true;
	}
	else if ( GetLastError() != ERROR_DIR_NOT_EMPTY )
	{
		AMLOGERROR( "RemoveDirectory failed for %s with error %d", szDir, GetLastError() );
	}

	return false;
}

bool CheckParentDirectory(LPCTSTR lpszFile)
{
	HRESULT hr = S_OK;
	TCHAR szDir[MAX_PATH];

	AMLOGDEBUG( "CheckParentDirectory for %s", lpszFile );

	hr = StringCbCopy( szDir, sizeof( szDir ), lpszFile );
	
	PathRemoveFileSpec( szDir );

	if ( MyCreateDirectory( szDir ) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool MyDeleteFile(LPCTSTR lpszFile)
{
	if ( DeleteFile( lpszFile ) )
	{
		AMLOGDEBUG( "Deleted file = %s", lpszFile );
		
		return true;
	}
	else
	{
		AMLOGDEBUG( "Unable to delete file %s with error %d", lpszFile, GetLastError() );

		return false;
	}
}

bool FileExists(LPCTSTR lpszFile)
{
	WIN32_FIND_DATA FindFileData;

	HANDLE hFind = FindFirstFile( lpszFile, &FindFileData );
	if ( hFind == INVALID_HANDLE_VALUE ) 
	{
		AMLOGDEBUG( "File does not exist = %s", lpszFile );
		
		return false;
	}
	else
	{
		FindClose( hFind );
		
		AMLOGDEBUG( "File exists = %s", lpszFile );

		return true;
	}
}

void ResizeArtwork(LPCTSTR lpszFile)
{
	AMLOGDEBUG( "Begin ResizeArtwork" );

	//
	// Read in the file
	//

	gdImagePtr im_in;
	gdImagePtr im_out = NULL;
	FILE* pFile = NULL;

	pFile = fopen( lpszFile, "rb" );
	if ( pFile )
	{
		if ( IsExtension( lpszFile, "jpg" ) )
		{
			im_in = gdImageCreateFromJpeg( pFile );
		}
		else if ( IsExtension( lpszFile, "png" ) )
		{
			im_in = gdImageCreateFromPng( pFile );
		}
		else if ( IsExtension( lpszFile, "bmp" ) )
		{
			im_in = gdImageCreateFromWBMP( pFile );
		}
		else
		{
			AMLOGDEBUG( "Unknown extension for image file at %s", lpszFile );
		}

		fclose( pFile );
		pFile = NULL;

		if ( im_in )
		{
			//
			// Determine if image size is ok
			//

			AMLOGDEBUG( "Original image size is %dx%d", gdImageSX( im_in ), gdImageSY( im_in ) );

			if ( gdImageSX( im_in ) > g_intArtworkWidth && g_intArtworkWidth > 0 )
			{
				MyDeleteFile( lpszFile );
			
				pFile = fopen( lpszFile, "wb" );
				if ( pFile )
				{
					im_out = gdImageCreateTrueColor( g_intArtworkWidth, (int) ( (float) g_intArtworkWidth * (float) im_in->sy / (float) im_in->sx ) );
					if ( im_out )
					{
						AMLOGDEBUG( "New image size is %dx%d", gdImageSX( im_out ), gdImageSY( im_out ) );

						gdImageCopyResampled( im_out, im_in, 0, 0, 0, 0, im_out->sx, im_out->sy, im_in->sx, im_in->sy );
					
						if ( IsExtension( lpszFile, "jpg" ) )
						{
							gdImageJpeg( im_out, pFile, 95 );

							AMLOGDEBUG( "Resized artwork" );
						}
						else if ( IsExtension( lpszFile, "png" ) )
						{
							gdImagePng( im_out, pFile );

							AMLOGDEBUG( "Resized artwork" );
						}
						else if ( IsExtension( lpszFile, "bmp" ) )
						{
							// Can't save as a bitmap because they only do 1 color?
							gdImageJpeg( im_out, pFile, 100 );
							
							AMLOGDEBUG( "Resized artwork" );
						}
						else
						{
							AMLOGDEBUG( "Unknown extension for image file at %s", lpszFile );
						}

						gdImageDestroy( im_out );
						im_out = NULL;
					}

					fclose( pFile );				
					pFile = NULL;
				}
			}

			gdImageDestroy( im_in );
			im_in = NULL;
		}
		else
		{						
			AMLOGDEBUG( "Unable to load image for %s", lpszFile );
		}
	}
	else
	{
		AMLOGDEBUG( "Unable to open image file at %s", lpszFile );
	}

	AMLOGDEBUG( "End ResizeArtwork" );
}

bool RunCommand(LPTSTR lpszCmdLine, DWORD dwMilliseconds)
{
	bool fRetVal = false;
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	DWORD dwStatus = 0;
	DWORD dwRetCode = 0;

	si.cb = sizeof( STARTUPINFO );
	si.lpReserved = NULL;
	si.lpDesktop = NULL;
	si.lpTitle = NULL; 
	si.dwX = NULL; 
	si.dwY = NULL; 
	si.dwXSize = NULL; 
	si.dwYSize = NULL; 
	si.dwXCountChars = NULL; 
	si.dwYCountChars = NULL; 
	si.dwFillAttribute = NULL;
#ifdef _DEBUG
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;
#else
	si.dwFlags = NULL; 
	si.wShowWindow = SW_HIDE;
#endif
	si.cbReserved2 = NULL; 
	si.lpReserved2 = NULL; 
	si.hStdInput = NULL; 
	si.hStdOutput= NULL; 
	si.hStdError = NULL; 

	AMLOGDEBUG( "Run = %s", lpszCmdLine );

	//CREATE_NO_WINDOW
	fRetVal = CreateProcess( NULL, lpszCmdLine, NULL, NULL, TRUE, IDLE_PRIORITY_CLASS, NULL, NULL, &si, &pi ) == TRUE ? true : false;

	if ( !fRetVal || !pi.hProcess )
	{
		return false;
	}

	dwStatus = WaitForSingleObject( pi.hProcess, dwMilliseconds );

	GetExitCodeProcess( pi.hProcess, &dwRetCode );
	if ( dwStatus == WAIT_TIMEOUT ) 
	{
		TerminateProcess( pi.hProcess, 0 );
	}
	else 
	{
		if ( dwRetCode == 0 ) 
		{
			fRetVal = true;
		}
	}

	CloseHandle( pi.hThread );
	CloseHandle( pi.hProcess );

	return fRetVal;
}

bool GetJsonTagContents(LPCTSTR lpszData, LPCTSTR lpszTagName, LPTSTR lpszValue, size_t cbValue)
{
	TCHAR szTagBegin[MAX_PATH] = {0};
	TCHAR szTagEnd[MAX_PATH] = {0};

	StringCbPrintf( szTagBegin, sizeof( szTagBegin ), "\"%s\":\"", lpszTagName );
	StringCbPrintf( szTagEnd, sizeof( szTagEnd ), "\"", lpszTagName );

	LPTSTR lpszStart = strstr( lpszData, szTagBegin );
	if ( lpszStart )
	{
		lpszStart += strlen( szTagBegin );

		LPTSTR lpszEnd = strstr( lpszStart, szTagEnd );
		if ( lpszEnd )
		{
			StringCbCopyN( lpszValue, cbValue, lpszStart, lpszEnd - lpszStart );

			AMLOGDEBUG ( "Tag %s = %s", lpszTagName, lpszValue );
				return true;
		}
	}

	return false;
}


bool GetXMLTagContents(LPCTSTR lpszData, LPCTSTR lpszTagName, LPTSTR lpszValue, size_t cbValue)
{
	TCHAR szTagBegin[MAX_PATH] = {0};
	TCHAR szTagEnd[MAX_PATH] = {0};

	StringCbPrintf( szTagBegin, sizeof( szTagBegin ), "<%s", lpszTagName );
	StringCbPrintf( szTagEnd, sizeof( szTagEnd ), "</%s>", lpszTagName );

	LPTSTR lpszStart = strstr( lpszData, szTagBegin );
	if ( lpszStart )
	{
		lpszStart += strlen( szTagBegin );

		lpszStart = strstr( lpszStart, ">" );
		if ( lpszStart )
		{
			lpszStart++;

			LPTSTR lpszEnd = strstr( lpszStart, szTagEnd );
			if ( lpszEnd )
			{
				StringCbCopyN( lpszValue, cbValue, lpszStart, lpszEnd - lpszStart );

				AMLOGDEBUG ( "Tag %s = %s", lpszTagName, lpszValue );

				return true;
			}
		}
	}

	return false;
}

HRESULT UrlEncodeAndConcat(LPTSTR lpszDest, size_t cbDest, LPCTSTR lpszSrc)
{
	HRESULT hr = S_OK;
	static char safechars[] = URL_ENCODE_SAFE_CHARS_RFC3986;
	size_t cbRemaining = cbDest;

	//
	// Advance to the end of the existing string
	//

	lpszDest += strlen( lpszDest );

    while ( *lpszSrc != '\0' && cbRemaining > 1 )
	{
		if ( strchr( safechars, *lpszSrc ) != NULL ) 
		{
			*lpszDest = *lpszSrc;
			cbDest++;
        } 
		else 
		{
			hr = StringCchPrintfA( lpszDest, cbRemaining, "%%%02X", (BYTE) *lpszSrc );
			if ( hr == STRSAFE_E_INSUFFICIENT_BUFFER )
			{
				return hr;
			}
			else
			{
				lpszDest += 2;
				cbDest += 2;
				cbRemaining -= 2;
			}
        }
        
		++lpszSrc;
		++lpszDest;
		--cbRemaining;
	}	

	*lpszDest = '\0';

	return hr;
}

//
// Encodes complete URLs that we do not have control over (eg. got from an XML doc)
//

LPTSTR UrlEncodeSpecial(LPCTSTR lpszSrc)
{
	HRESULT hr = S_OK;
	static char badchars[] = " ";
	size_t cbSize = 1;	// Start with 1 for the null
	LPCTSTR lpszSrc1 = lpszSrc;

	//
	// Figure out how much space we need
	//

	while ( *lpszSrc1 != '\0' )
	{
		if ( strchr( badchars, *lpszSrc1 ) != NULL ) 
		{
			cbSize += 2;
		}
       
		lpszSrc1++;
		cbSize++;
	}

	//
	// Allocate the string
	//

	LPTSTR lpszReturn = (LPTSTR) malloc( cbSize );
	if ( !lpszReturn )
	{
		return NULL;
	}

	//
	// Copy and encode
	//

	size_t cbRemaining = cbSize;
	LPTSTR lpszDest = lpszReturn;

	while ( *lpszSrc != '\0' && cbRemaining > 1 )
	{
		if ( strchr( badchars, *lpszSrc ) == NULL ) 
		{
			*lpszDest = *lpszSrc;
		} 
		else 
		{
			hr = StringCbPrintf( lpszDest, cbRemaining, "%%%02X", (int) *lpszSrc );
			if ( hr == STRSAFE_E_INSUFFICIENT_BUFFER )
			{
				return NULL;
			}
			else
			{
				lpszDest += 2;
				cbRemaining -= 2;
			}
		}
    
		++lpszSrc;
		++lpszDest;
		--cbRemaining;
	}	

	*lpszDest = '\0';

	return lpszReturn;
}

const char HEX2DEC[256] = 

{

    /*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */

    /* 0 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* 1 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* 2 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* 3 */  0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,

    

    /* 4 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* 5 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* 6 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* 7 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    

    /* 8 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* 9 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* A */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* B */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    

    /* C */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* D */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* E */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* F */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1

};

  

LPTSTR UrlDecode(LPCTSTR lpszSrc)
{
    // Note from RFC1630:  "Sequences which start with a percent sign
    // but are not followed by two hexadecimal characters (0-9, A-F) are reserved
    // for future extension"
    
    const char * pSrc = lpszSrc;
	const int SRC_LEN = strlen(lpszSrc);
    const char * const SRC_END = pSrc + SRC_LEN;
    const char * const SRC_LAST_DEC = SRC_END - 2;   // last decodable '%' 

    char * const pStart = (char *) malloc(SRC_LEN);
    char * pEnd = pStart;

    while (pSrc < SRC_LAST_DEC)
	{
		if (*pSrc == '%')
        {
            char dec1, dec2;

            if (-1 != (dec1 = HEX2DEC[*(pSrc + 1)]) && -1 != (dec2 = HEX2DEC[*(pSrc + 2)]))
            {
                *pEnd++ = (dec1 << 4) + dec2;
                pSrc += 3;
                continue;
            }
        }

        *pEnd++ = *pSrc++;
	}

    // the last 2- chars
    while (pSrc < SRC_END)
	{
        *pEnd++ = *pSrc++;
	}

	*pEnd = '\0';

	return pStart;
}

typedef struct DLGTEMPLATEEX
{
    WORD dlgVer;
    WORD signature;
    DWORD helpID;
    DWORD exStyle;
    DWORD style;
    WORD cDlgItems;
    short x;
    short y;
    short cx;
    short cy;
} DLGTEMPLATEEX, *LPDLGTEMPLATEEX;

int CALLBACK RemoveContextHelpProc(HWND hwnd, UINT message, LPARAM lParam)
{
    switch ( message ) 
	{
		case PSCB_PRECREATE:
		{
			//
			// Remove the DS_CONTEXTHELP style from the dialog box template
			//

			if (((LPDLGTEMPLATEEX)lParam)->signature == 0xFFFF)
			{
				((LPDLGTEMPLATEEX)lParam)->style &= ~DS_CONTEXTHELP;
			}
			else 
			{
				((LPDLGTEMPLATE)lParam)->style &= ~DS_CONTEXTHELP;
			}
        
			return TRUE;
		}
	}

	return TRUE;
}

typedef struct LANGANDCODEPAGE 
{
	WORD wLanguage;
	WORD wCodePage;
}
LANGANDCODEPAGE;

void GetMyVersion(LPTSTR lpszDest, size_t cchDest)
{
	DWORD dwSize = 0;
	char* rcData = 0; 
	LANGANDCODEPAGE* lpTranslate = NULL;
	UINT wSize;
	char szSubBlock[100];
	LPVOID lpBuffer;
	unsigned i = 0;
	LPDWORD handle1 = 0;
	DWORD dwHandle = 0;

	dwSize = GetFileVersionInfoSize( g_szThisDllPath, &dwHandle );
	if ( dwSize > 0 )
	{
		rcData = (char *) malloc( dwSize );

		if ( rcData )
		{
			if ( GetFileVersionInfo( g_szThisDllPath, 0, dwSize, (LPVOID) rcData ) )
			{
				if ( VerQueryValue( rcData, TEXT("\\VarFileInfo\\Translation" ), (LPVOID*)&lpTranslate, &wSize ) )
				{
					for ( i = 0; i < (wSize / sizeof(struct LANGANDCODEPAGE)); i++ )
					{
						StringCchPrintf( szSubBlock, sizeof( szSubBlock ), TEXT("\\StringFileInfo\\%04x%04x\\FileVersion"), lpTranslate[i].wLanguage,lpTranslate[i].wCodePage );

						if ( VerQueryValue( rcData, szSubBlock, (LPVOID*) &lpBuffer, &wSize ) )
						{
							StringCbCopy( lpszDest, cchDest, (LPCSTR) lpBuffer );
							
							free( rcData );
							
							return;
						}
					}
				}
			}

			free( rcData );
		}
	}

	*lpszDest = '\0';
}

void PrintTrackInfoNumberToFile(IXMLDOMDocumentPtr& pXMLDOMDoc, IXMLDOMElementPtr& pElementSong, LPCTSTR lpszNodeName, int intNodeValue)
{
	if ( lpszNodeName && intNodeValue != PROP_UNSUPPORTED )
	{
		IXMLDOMElementPtr pe = pXMLDOMDoc->createElement( lpszNodeName );
		if ( pe )
		{	
			if ( intNodeValue > 0 )
			{	
				AMLOGDEBUG( "<%s>%d</%s>", lpszNodeName, intNodeValue, lpszNodeName );

				char szTemp[32];
				StringCbPrintf( szTemp, sizeof( szTemp ), "%d", intNodeValue );
				pe->text = szTemp;
			}

			pElementSong->appendChild( pXMLDOMDoc->createTextNode( "\t\t" ) );
			pElementSong->appendChild( pe );
			pElementSong->appendChild( pXMLDOMDoc->createTextNode( "\n" ) );
			
			pe.Release();			
		}
	}
}

void PrintTrackInfoStringToFile(IXMLDOMDocumentPtr& pXMLDOMDoc, IXMLDOMElementPtr& pElementSong, LPCTSTR lpszNodeName, LPWSTR lpszNodeValue, bool fUseCData)
{
	USES_CONVERSION;

	if ( lpszNodeName && lpszNodeValue )
	{
		IXMLDOMElementPtr pe = pXMLDOMDoc->createElement( lpszNodeName );
		if ( pe )
		{
			if ( lpszNodeValue[0] != '\0' )
			{
				AMLOGDEBUG( "<%s>%s</%s>", lpszNodeName, W2T( lpszNodeValue ), lpszNodeName );

				if ( fUseCData )
				{
					IXMLDOMCDATASectionPtr pcd = pXMLDOMDoc->createCDATASection( lpszNodeValue );
					if ( pcd )
					{
						pe->appendChild( pcd );
						pcd.Release();
					}
				}
				else
				{
					pe->text = lpszNodeValue;				
				}
			}

			pElementSong->appendChild( pXMLDOMDoc->createTextNode( "\t\t" ) );
			pElementSong->appendChild( pe );
			pElementSong->appendChild( pXMLDOMDoc->createTextNode( "\n" ) );
			
			pe.Release();			
		}
	}
}

void PrintTrackInfoStringToFile(IXMLDOMDocumentPtr& pXMLDOMDoc, IXMLDOMElementPtr& pElementSong, LPCTSTR lpszNodeName, LPCSTR lpszNodeValue, bool fUseCData)
{
	if ( lpszNodeName && lpszNodeValue )
	{
		IXMLDOMElementPtr pe = pXMLDOMDoc->createElement( lpszNodeName );
		if ( pe )
		{
			if ( lpszNodeValue[0] != '\0' )
			{
				AMLOGDEBUG( "<%s>%s</%s>", lpszNodeName, lpszNodeValue, lpszNodeName );

				if ( fUseCData )
				{
					IXMLDOMCDATASectionPtr pcd = pXMLDOMDoc->createCDATASection( lpszNodeValue );
					if ( pcd )
					{
						pe->appendChild( pcd );
						pcd.Release();
					}
				}
				else
				{
					pe->text = lpszNodeValue;
				}
			}

			pElementSong->appendChild( pXMLDOMDoc->createTextNode( "\t\t" ) );
			pElementSong->appendChild( pe );
			pElementSong->appendChild( pXMLDOMDoc->createTextNode( "\n" ) );
			
			pe.Release();			
		}
	}
}

#define  INET_ERR_OUT_MSG_BOX_BUFFER_SIZE      1024
#define  INET_ERR_OUT_FORMAT_BUFFER_SIZE       2512

void WINAPI addLastErrorToMsg(LPTSTR szMsgBuffer, DWORD dwSize)
{
	TCHAR szNumberBuffer[32] = {0};

	_itoa( GetLastError(), szNumberBuffer, 10 );
	StringCchCat( szMsgBuffer, dwSize, TEXT( "\n   System Error number: " ) );
	StringCchCat( szMsgBuffer, dwSize, szNumberBuffer );
	StringCchCat( szMsgBuffer, dwSize, TEXT( ".\n" ) );
}

BOOL WINAPI InternetErrorOut(HWND hWnd, DWORD dwError, LPCTSTR szFailingFunctionName)
{
	HANDLE hProcHeap;
	TCHAR szMsgBoxBuffer[INET_ERR_OUT_MSG_BOX_BUFFER_SIZE] = {0};
	TCHAR szFormatBuffer[INET_ERR_OUT_FORMAT_BUFFER_SIZE] = {0};
	TCHAR szConnectiveText[] = { TEXT( "\n\nAdditional Information: " ) };
	TCHAR* lpszExtErrMsg = NULL;
	TCHAR* lpszCombinedErrMsg = NULL;
	DWORD dwInetError = 0;
	DWORD dwBaseLength = 0;
	DWORD dwExtLength = 0;

	if ( ( hProcHeap = GetProcessHeap() ) == NULL )
	{
		StringCchCopy( szMsgBoxBuffer, INET_ERR_OUT_MSG_BOX_BUFFER_SIZE, TEXT( "Call to GetProcessHeap( ) failed..." ) ); 
		goto InetErrorOutError_1;
	}

	dwBaseLength = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE, GetModuleHandle( "wininet.dll" ), dwError, 0, (LPTSTR) szFormatBuffer, INET_ERR_OUT_FORMAT_BUFFER_SIZE, NULL );
	if ( !dwBaseLength )
	{
		StringCchCopy( szMsgBoxBuffer, INET_ERR_OUT_MSG_BOX_BUFFER_SIZE, TEXT( "FormatMessage() failed..." ) ); 
		addLastErrorToMsg( szMsgBoxBuffer, INET_ERR_OUT_MSG_BOX_BUFFER_SIZE );
		goto InetErrorOutError_1;
	}

	if ( FAILED( StringCchPrintf( szMsgBoxBuffer, INET_ERR_OUT_MSG_BOX_BUFFER_SIZE, g_szStringMsgErrorConnect, dwError, szFailingFunctionName, szFormatBuffer ) ) )
	{
		StringCchCopy( szMsgBoxBuffer, INET_ERR_OUT_MSG_BOX_BUFFER_SIZE, 
		TEXT( "Call to StringCchPrintf( ) failed...\n" ) ); 
		goto InetErrorOutError_1;
	}

	StringCchLength( szMsgBoxBuffer, INET_ERR_OUT_MSG_BOX_BUFFER_SIZE, (size_t*) &dwBaseLength );
	// Adjust base-length value to count the number of bytes:
	dwBaseLength *= sizeof( TCHAR );

	if ( dwError == ERROR_INTERNET_EXTENDED_ERROR )
	{
		InternetGetLastResponseInfo( &dwInetError, NULL, &dwExtLength );
		// Adjust the extended-length value to a byte count 
		// that includes the terminating null:
		++dwExtLength *= sizeof( TCHAR );
    
		if ( ( lpszExtErrMsg = (TCHAR*) HeapAlloc( hProcHeap, 0, dwExtLength ) ) == NULL )
		{
			StringCchCat( szMsgBoxBuffer, INET_ERR_OUT_MSG_BOX_BUFFER_SIZE, TEXT( "\nFailure: Could not allocate buffer for addional details." ) ); 
			addLastErrorToMsg( szMsgBoxBuffer, INET_ERR_OUT_MSG_BOX_BUFFER_SIZE );
			goto InetErrorOutError_1;
		}

		if ( !InternetGetLastResponseInfo( &dwInetError, lpszExtErrMsg, &dwExtLength ) )
		{
			StringCchCat( szMsgBoxBuffer, INET_ERR_OUT_MSG_BOX_BUFFER_SIZE, TEXT( "\nCall to InternetGetLastResponseInfo( ) failed--" ) );
			addLastErrorToMsg( szMsgBoxBuffer, INET_ERR_OUT_MSG_BOX_BUFFER_SIZE );
			goto InetErrorOutError_2;
		}

		dwBaseLength += dwExtLength + sizeof( szConnectiveText );
	}

	//
	// Add one for NULL character
	//

	dwBaseLength++;

	//
	// Combine all the message together
	//

	if ( ( lpszCombinedErrMsg = (TCHAR*) HeapAlloc( hProcHeap, 0, dwBaseLength ) ) == NULL )
	{
		StringCchCat( szMsgBoxBuffer, INET_ERR_OUT_MSG_BOX_BUFFER_SIZE, TEXT( "\nFailure: Could not allocate final output buffer." ) );
		addLastErrorToMsg( szMsgBoxBuffer, INET_ERR_OUT_MSG_BOX_BUFFER_SIZE );
		goto InetErrorOutError_2;
	}

	if ( FAILED( StringCchCopy( lpszCombinedErrMsg, dwBaseLength, szMsgBoxBuffer ) ) )
	{	
		StringCchCat( szMsgBoxBuffer, INET_ERR_OUT_MSG_BOX_BUFFER_SIZE, TEXT( "\nFailure: Could not assemble final message 1." ) );
		goto InetErrorOutError_3;
	}
	
	if ( dwExtLength )
	{
		if ( FAILED( StringCchCat( lpszCombinedErrMsg, dwBaseLength, szConnectiveText ) ) )
		{
			StringCchCat( szMsgBoxBuffer, INET_ERR_OUT_MSG_BOX_BUFFER_SIZE, TEXT( "\nFailure: Could not assemble final message 2." ) );
			goto InetErrorOutError_3;
		}
         
		if ( FAILED( StringCchCat( lpszCombinedErrMsg, dwBaseLength, lpszExtErrMsg ) ) )
		{
			StringCchCat( szMsgBoxBuffer, INET_ERR_OUT_MSG_BOX_BUFFER_SIZE, TEXT( "\nFailure: Could not assemble final message 3." ) ); 
			goto InetErrorOutError_3;
		}	
	}

	AMLOGERROR( "%s", lpszCombinedErrMsg );

	HeapFree( hProcHeap, 0, lpszExtErrMsg );
	HeapFree( hProcHeap, 0, lpszCombinedErrMsg );

	return TRUE;

InetErrorOutError_3:
	HeapFree( hProcHeap, 0, lpszCombinedErrMsg );
InetErrorOutError_2:
	HeapFree( hProcHeap, 0, lpszExtErrMsg );
InetErrorOutError_1:
	MessageBox( hWnd, (LPCTSTR) szMsgBoxBuffer, PRODUCT_NAME, MB_ICONERROR | MB_OK );

	AMLOGERROR( "Dialog error: %s", szMsgBoxBuffer );

	return FALSE;
}

void DoAppleLookup(LPCTSTR lpszArtist, LPCTSTR lpszAlbum, LPCTSTR lpszTitle, LPTSTR lpszUrl, size_t cchUrl)
{
	DWORD dwDisposition = 0;
	HINTERNET hSession;
	HINTERNET hConnection;
	HINTERNET hData;
	DWORD dwContext = 0;
	LPTSTR lpszData = NULL;
	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;
	char szUrlSend[MAX_PATH];
	time_t timeNow = 0;

	StringCbPrintf( szUrlSend, sizeof( szUrlSend ), "%s%s+%s+%s", APPLE_LOOKUP_PATH, lpszTitle, lpszAlbum, lpszArtist );

	AMLOGDEBUG( "Apple URL = http://%s%s", APPLE_LOOKUP_HOSTNAME, szUrlSend );
	
	hSession = InternetOpen( g_szUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
	if ( hSession )
	{
		hConnection = InternetConnect( hSession, APPLE_LOOKUP_HOSTNAME, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, dwContext );
		if ( hConnection )
		{
			hData = HttpOpenRequest( hConnection, NULL, szUrlSend, NULL, NULL, NULL, INTERNET_FLAG_RELOAD, 0 );
			if ( hData )
			{
				if ( HttpSendRequest( hData, NULL, 0, NULL, 0) )
				{
					bool fDone = false;
					size_t sizeNow = 0;

					while ( !fDone )
					{
						if ( InternetQueryDataAvailable( hData, &dwSize, 0, 0 ) )
						{
							if ( dwSize > 0 )
							{
								LPTSTR lpszDataChunk = (char *) malloc( dwSize + 1 );
								if ( lpszDataChunk )
								{
									if ( InternetReadFile( hData, (LPVOID) lpszDataChunk, dwSize, &dwDownloaded ) )
									{
										AMLOGDEBUG( "InternetReadFile downloaded %d bytes", dwDownloaded );

										if ( dwDownloaded > 0 )
										{
											lpszDataChunk[ dwDownloaded ] = '\0';

											AMLOGDEBUG( "Got chunk = %s", lpszDataChunk );

											//
											// Do we need to make a backup?
											//

											if ( lpszData )
											{
												LPTSTR lpszDataTemp = (char *) malloc( strlen( lpszData ) + 1 );
												StringCbCopy( lpszDataTemp, strlen( lpszData ) + 1, lpszData );

												if ( lpszDataTemp )
												{					
													free( lpszData );
											
													lpszData = (char *) malloc( strlen( lpszDataTemp ) + dwDownloaded + 1 );
													if ( lpszData )
													{
														StringCbCopy( lpszData, strlen( lpszDataTemp ) + dwDownloaded + 1, lpszDataTemp );
														StringCbCat( lpszData, strlen( lpszDataTemp ) + dwDownloaded + 1, lpszDataChunk );
													}											

													free( lpszDataTemp );
												}
											}
											else
											{
												lpszData = (char *) malloc( dwDownloaded + 1 );
												if ( lpszData )
												{
													StringCbCopy( lpszData, dwDownloaded + 1, lpszDataChunk );
												}											

											}
										}
									}

									free( lpszDataChunk );
								}
							}
							else
							{
								fDone = true;
							}
						}
					}

					if ( lpszData )
					{
#ifdef _DEBUG
						FILE* pFile = fopen( "C:\\iTunesSearch.html", "wt" );
						if ( pFile )
						{
							fputs( lpszData, pFile );
										
							fclose( pFile );
						}
#endif
						LPTSTR lpszPointerPlaylist = strstr( lpszData, MARKER_APPLE_ALBUM_START );

						if ( lpszPointerPlaylist )
						{
							int i = 0;
							TCHAR szPlaylistID[MAX_PATH] = {0};
							TCHAR szSongID[MAX_PATH] = {0};

							lpszPointerPlaylist += strlen( MARKER_APPLE_ALBUM_START );

							for ( i = 0; isdigit( *lpszPointerPlaylist ); i++, lpszPointerPlaylist++ )
							{
								szPlaylistID[i] = *lpszPointerPlaylist;
							}

							LPTSTR lpszPointerSong = strstr( lpszPointerPlaylist, MARKER_APPLE_SONG_START );
							if ( lpszPointerSong )
							{
								lpszPointerSong += strlen( MARKER_APPLE_SONG_START );

								for ( i = 0; isdigit( *lpszPointerSong ); i++, lpszPointerSong++ )
								{
									szSongID[i] = *lpszPointerSong;
								}

								AMLOGDEBUG( "iTunes Playlist ID = %s", szPlaylistID );
								AMLOGDEBUG( "iTunes Song ID = %s", szSongID );
					
								if ( strlen( szPlaylistID ) > 0 && strlen( szSongID ) > 0 )
								{
									// Apple
									//StringCbPrintf( lpszUrl, cchUrl, "http://phobos.apple.com/WebObjects/MZStore.woa/wa/viewAlbum?selectedItemId=%s&playListId=%s", szSongID, szPlaylistID );								

									// LinkShare
									StringCbPrintf( lpszUrl, cchUrl, URL_APPLE_STORE, strlen( g_szAppleAssociate ) ? g_szAppleAssociate : DEFAULT_APPLE_ASSOCIATE, szSongID, szPlaylistID );

									DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_APPLE);
								}
							}
						}

						free( lpszData );
					}
				}

				InternetCloseHandle( hData );
			}

			InternetCloseHandle( hConnection );
		}

		InternetCloseHandle( hSession );
	}
}

void OAuthSignHMACSHA256(const char * lpszBaseString, const char * lpszSessionSecret, char * lpszValue, size_t cchValue)
{
    unsigned char buf[HMAC_SHA256_DIGEST_LENGTH];
    HMAC_SHA256_CTX ctx;
	
    HMAC_SHA256_Init(&ctx);
    HMAC_SHA256_UpdateKey(&ctx, (unsigned char *) lpszSessionSecret, strlen(lpszSessionSecret));
    HMAC_SHA256_EndKey(&ctx);
	
    HMAC_SHA256_StartMessage(&ctx);
    HMAC_SHA256_UpdateMessage(&ctx, (unsigned char *) lpszBaseString, strlen((char *) lpszBaseString));
    HMAC_SHA256_EndMessage(buf, &ctx);
	
    HMAC_SHA256_Done(&ctx);
	
	int len = b64_encode(buf, HMAC_SHA256_DIGEST_LENGTH, lpszValue, cchValue);
    lpszValue[len] = '\0';
}

void DoPing(BOOL fPlay, int intNext)
{
	HINTERNET hSession;
	HINTERNET hConnection;
	HINTERNET hData;
	DWORD dwContext = 0;
	char szValueHostName[MAX_PATH] = {0};
	char szValueUrlPath[MAX_PATH] = {0};
	int nScheme;

	USES_CONVERSION;

	AMLOGDEBUG( "Begin DoPing" );

	char szPostData[5*1024] = {0};

	if ( fPlay )
	{
		char szValue[MAX_PATH] = {0};

		StringCbCat( szPostData, sizeof( szPostData ), TAG_TITLE );
		StringCbCat( szPostData, sizeof( szPostData ), "=" );		
		WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.name[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
		UrlEncodeAndConcat( szPostData, sizeof( szPostData ), szValue );

		StringCbCat( szPostData, sizeof( szPostData ), "&" );
		StringCbCat( szPostData, sizeof( szPostData ), TAG_ARTIST );
		StringCbCat( szPostData, sizeof( szPostData ), "=" );
		WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.artist[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
		UrlEncodeAndConcat( szPostData, sizeof( szPostData ), szValue );

		StringCbCat( szPostData, sizeof( szPostData ), "&" );
		StringCbCat( szPostData, sizeof( szPostData ), TAG_ALBUM );
		StringCbCat( szPostData, sizeof( szPostData ), "=" );
		WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.album[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
		UrlEncodeAndConcat( szPostData, sizeof( szPostData ), szValue );

		StringCbCat( szPostData, sizeof( szPostData ), "&" );
		StringCbCat( szPostData, sizeof( szPostData ), TAG_GENRE );
		StringCbCat( szPostData, sizeof( szPostData ), "=" );
		WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.genre[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
		UrlEncodeAndConcat( szPostData, sizeof( szPostData ), szValue );

		StringCbCat( szPostData, sizeof( szPostData ), "&" );
		StringCbCat( szPostData, sizeof( szPostData ), TAG_KIND );
		StringCbCat( szPostData, sizeof( szPostData ), "=" );
		WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.kind[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
		UrlEncodeAndConcat( szPostData, sizeof( szPostData ), szValue );

		StringCbCat( szPostData, sizeof( szPostData ), "&" );
		StringCbCat( szPostData, sizeof( szPostData ), TAG_TRACK );
		StringCbCat( szPostData, sizeof( szPostData ), "=" );
		StringCbPrintf( szValue, sizeof( szValue ), "%d", g_paPlaylist[ intNext ].m_trackInfo.trackNumber );
		StringCbCat( szPostData, sizeof( szPostData ), szValue );

		if ( g_intMediaPlayerType == MEDIA_PLAYER_ITUNES || g_intMediaPlayerType == MEDIA_PLAYER_WMP )
		{		
			StringCbCat( szPostData, sizeof( szPostData ), "&" );
			StringCbCat( szPostData, sizeof( szPostData ), TAG_NUMTRACKS );
			StringCbCat( szPostData, sizeof( szPostData ), "=" );
			StringCbPrintf( szValue, sizeof( szValue ), "%d", g_paPlaylist[ intNext ].m_trackInfo.numTracks );
			StringCbCat( szPostData, sizeof( szPostData ), szValue );
		}

		StringCbCat( szPostData, sizeof( szPostData ), "&" );
		StringCbCat( szPostData, sizeof( szPostData ), TAG_YEAR );
		StringCbCat( szPostData, sizeof( szPostData ), "=" );
		StringCbPrintf( szValue, sizeof( szValue ), "%d", g_paPlaylist[ intNext ].m_trackInfo.year );
		StringCbCat( szPostData, sizeof( szPostData ), szValue );

		StringCbCat( szPostData, sizeof( szPostData ), "&" );
		StringCbCat( szPostData, sizeof( szPostData ), TAG_COMMENTS );
		StringCbCat( szPostData, sizeof( szPostData ), "=" );
		WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.comments[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
		UrlEncodeAndConcat( szPostData, sizeof( szPostData ), szValue );

		StringCbCat( szPostData, sizeof( szPostData ), "&" );
		StringCbCat( szPostData, sizeof( szPostData ), TAG_TIME );
		StringCbCat( szPostData, sizeof( szPostData ), "=" );
		StringCbPrintf( szValue, sizeof( szValue ), "%d", g_paPlaylist[ intNext ].m_trackInfo.totalTimeInMS / 1000 );
		StringCbCat( szPostData, sizeof( szPostData ), szValue );

		StringCbCat( szPostData, sizeof( szPostData ), "&" );
		StringCbCat( szPostData, sizeof( szPostData ), TAG_BITRATE );
		StringCbCat( szPostData, sizeof( szPostData ), "=" );
		StringCbPrintf( szValue, sizeof( szValue ), "%d", g_paPlaylist[ intNext ].m_trackInfo.bitRate );
		StringCbCat( szPostData, sizeof( szPostData ), szValue );

		if ( g_intMediaPlayerType == MEDIA_PLAYER_ITUNES || g_intMediaPlayerType == MEDIA_PLAYER_WMP )
		{
			StringCbCat( szPostData, sizeof( szPostData ), "&" );
			StringCbCat( szPostData, sizeof( szPostData ), TAG_RATING );
			StringCbCat( szPostData, sizeof( szPostData ), "=" );
			StringCbPrintf( szValue, sizeof( szValue ), "%d", g_paPlaylist[ intNext ].m_trackInfo.userRating / 20 );
			StringCbCat( szPostData, sizeof( szPostData ), szValue );
		}

		StringCbCat( szPostData, sizeof( szPostData ), "&" );
		StringCbCat( szPostData, sizeof( szPostData ), TAG_DISC );
		StringCbCat( szPostData, sizeof( szPostData ), "=" );
		StringCbPrintf( szValue, sizeof( szValue ), "%d", g_paPlaylist[ intNext ].m_trackInfo.discNumber );
		StringCbCat( szPostData, sizeof( szPostData ), szValue );

		if ( g_intMediaPlayerType == MEDIA_PLAYER_ITUNES || g_intMediaPlayerType == MEDIA_PLAYER_WMP )
		{
			StringCbCat( szPostData, sizeof( szPostData ), "&" );
			StringCbCat( szPostData, sizeof( szPostData ), TAG_NUMDISCS );
			StringCbCat( szPostData, sizeof( szPostData ), "=" );
			StringCbPrintf( szValue, sizeof( szValue ), "%d", g_paPlaylist[ intNext ].m_trackInfo.numDiscs );
			StringCbCat( szPostData, sizeof( szPostData ), szValue );
		}

		StringCbCat( szPostData, sizeof( szPostData ), "&" );
		StringCbCat( szPostData, sizeof( szPostData ), TAG_PLAYCOUNT );
		StringCbCat( szPostData, sizeof( szPostData ), "=" );
		StringCbPrintf( szValue, sizeof( szValue ), "%d", g_paPlaylist[ intNext ].m_trackInfo.playCount );
		StringCbCat( szPostData, sizeof( szPostData ), szValue );

		StringCbCat( szPostData, sizeof( szPostData ), "&" );
		StringCbCat( szPostData, sizeof( szPostData ), TAG_COMPILATION );
		StringCbCat( szPostData, sizeof( szPostData ), "=" );
		UrlEncodeAndConcat( szPostData, sizeof( szPostData ), g_paPlaylist[ intNext ].m_trackInfo.isCompilationTrack == TRUE ? "Yes" : "No" );

		if ( g_intAmazonEnabled )
		{
			StringCbCat( szPostData, sizeof( szPostData ), "&" );
			StringCbCat( szPostData, sizeof( szPostData ), TAG_URLAMAZON );
			StringCbCat( szPostData, sizeof( szPostData ), "=" );
			UrlEncodeAndConcat( szPostData, sizeof( szPostData ), g_paPlaylist[ intNext ].m_szUrlAmazon );
		}

		if ( g_intAppleEnabled )
		{
			StringCbCat( szPostData, sizeof( szPostData ), "&" );
			StringCbCat( szPostData, sizeof( szPostData ), TAG_URLAPPLE );
			StringCbCat( szPostData, sizeof( szPostData ), "=" );
			UrlEncodeAndConcat( szPostData, sizeof( szPostData ), g_paPlaylist[ intNext ].m_szUrlApple );
		}

		if ( g_intAmazonEnabled )
		{
			StringCbCat( szPostData, sizeof( szPostData ), "&" );
			StringCbCat( szPostData, sizeof( szPostData ), TAG_IMAGESMALL );
			StringCbCat( szPostData, sizeof( szPostData ), "=" );
			UrlEncodeAndConcat( szPostData, sizeof( szPostData ), g_paPlaylist[ intNext ].m_szImageSmallUrl );	

			StringCbCat( szPostData, sizeof( szPostData ), "&" );
			StringCbCat( szPostData, sizeof( szPostData ), TAG_IMAGE );
			StringCbCat( szPostData, sizeof( szPostData ), "=" );
			UrlEncodeAndConcat( szPostData, sizeof( szPostData ), g_paPlaylist[ intNext ].m_szImageUrl );

			StringCbCat( szPostData, sizeof( szPostData ), "&" );
			StringCbCat( szPostData, sizeof( szPostData ), TAG_IMAGELARGE );
			StringCbCat( szPostData, sizeof( szPostData ), "=" );
			UrlEncodeAndConcat( szPostData, sizeof( szPostData ), g_paPlaylist[ intNext ].m_szImageLargeUrl );
		}

		StringCbCat( szPostData, sizeof( szPostData ), "&" );
		StringCbCat( szPostData, sizeof( szPostData ), TAG_COMPOSER );
		StringCbCat( szPostData, sizeof( szPostData ), "=" );
		WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.composer[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
		UrlEncodeAndConcat( szPostData, sizeof( szPostData ), szValue );


		StringCbCat( szPostData, sizeof( szPostData ), "&" );
		StringCbCat( szPostData, sizeof( szPostData ), TAG_GROUPING );
		StringCbCat( szPostData, sizeof( szPostData ), "=" );
		WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.grouping[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
		UrlEncodeAndConcat( szPostData, sizeof( szPostData ), szValue );

		StringCbCat( szPostData, sizeof( szPostData ), "&" );
		StringCbCat( szPostData, sizeof( szPostData ), TAG_URLSOURCE );
		StringCbCat( szPostData, sizeof( szPostData ), "=" );
		UrlEncodeAndConcat( szPostData, sizeof( szPostData ), g_paPlaylist[ intNext ].m_szUrlSource );

		if ( g_intMediaPlayerType == MEDIA_PLAYER_ITUNES || g_intMediaPlayerType == MEDIA_PLAYER_WINAMP )
		{
			StringCbCat( szPostData, sizeof( szPostData ), "&" );
			StringCbCat( szPostData, sizeof( szPostData ), TAG_FILE );
			StringCbCat( szPostData, sizeof( szPostData ), "=" );
			WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.fileName[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
			UrlEncodeAndConcat( szPostData, sizeof( szPostData ), szValue );
		}

		if ( g_intMediaPlayerType == MEDIA_PLAYER_ITUNES )
		{
			StringCbCat( szPostData, sizeof( szPostData ), "&" );
			StringCbCat( szPostData, sizeof( szPostData ), TAG_ARTWORKID );
			StringCbCat( szPostData, sizeof( szPostData ), "=" );
			UrlEncodeAndConcat( szPostData, sizeof( szPostData ), g_paPlaylist[ intNext ].m_szArtworkID );
		}
	}

	if ( CrackUrl( g_szTrackBackUrl, szValueHostName, sizeof( szValueHostName ), szValueUrlPath, sizeof( szValueUrlPath ), nScheme ) )
	{				
		AMLOGDEBUG( "URL to Ping = %s", szValueUrlPath );

		//
		// Do the Ping
		//

		hSession = InternetOpen( g_szUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
		if ( hSession )
		{
			hConnection = InternetConnect( hSession, szValueHostName, nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, dwContext );
			if ( hConnection )
			{
				DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_NO_UI | INTERNET_FLAG_NO_COOKIES;

				if ( nScheme == INTERNET_SCHEME_HTTPS )
				{
					dwFlags |= INTERNET_FLAG_SECURE;
				}

				hData = HttpOpenRequest( hConnection, "POST", szValueUrlPath, NULL, NULL, NULL, dwFlags, 0 );
				if ( hData )
				{
					TCHAR szContentType[128] = "Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n";
					HttpAddRequestHeaders( hData, szContentType, -1L, HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD );

					//
					// Add a passphrase if configured
					// 

					if ( strlen( g_szTrackBackPassphrase ) > 0 )
					{
						TCHAR szHeader[MAX_PATH] = {0};
						StringCbPrintf( szHeader, sizeof( szHeader ), "X-NowPlaying: %s\r\n", g_szTrackBackPassphrase );
						HttpAddRequestHeaders( hData, szHeader, -1L, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE );
					}

Do_HttpSendRequest:
					if ( !HttpSendRequest( hData, NULL, 0, szPostData, strlen( szPostData ) ) )
					{				
						if ( GetLastError() == ERROR_INTERNET_INVALID_CA )
						{
							AMLOGDEBUG( "Failed to send request to URL: Invalid certificate authority detected. Retrying with ignore." );

							DWORD dwFlagsInternet = 0;

							DWORD dwBuffLen = sizeof( dwFlagsInternet );

							InternetQueryOption( hData, INTERNET_OPTION_SECURITY_FLAGS, (LPVOID) &dwFlagsInternet, &dwBuffLen );

							dwFlagsInternet |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;

							InternetSetOption( hData, INTERNET_OPTION_SECURITY_FLAGS, &dwFlagsInternet, sizeof( dwFlagsInternet ) );

							goto Do_HttpSendRequest;

						}
						else
						{
							AMLOGERROR ( "Ping failed with error %ld", ::GetLastError() );
						}
					}
					else
					{
						DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_PING);
					}

					InternetCloseHandle( hData );
				}

				InternetCloseHandle( hConnection );
			}
	
			InternetCloseHandle( hSession );
		}
	}
	else
	{
		MessageBox( NULL, "Invalid URL format.", PRODUCT_NAME, MB_OK + MB_ICONERROR );					
	}

	AMLOGDEBUG( "End DoPing" );
}


/**
 * split and parse URL parameters replied by the test-server
 * into <em>oauth_token</em> and <em>oauth_token_secret</em>.
 */
int parse_reply(const char *reply, char **token, char **secret)
{
	int rc;
	int ok=1;
	int i;
	char **rv = NULL;

	rc = oauth_split_url_parameters(reply, &rv);
	qsort(rv, rc, sizeof(char *), oauth_cmpstringp);
	
	for (i = 0; i < rc; i++)
	{
		if (secret && strncmp(rv[i], "oauth_token_secret=", 18) == 0)
		{
			*secret = strdup(&(rv[i][19]));
			ok = 0;
		}
		else if (token && strncmp(rv[i], "oauth_token=", 11) == 0)
		{			
			*token = strdup(&(rv[i][12]));
			ok = 0;
		}
	}
	
	if (rv)
	{
		free(rv);
	}

	return ok;
}

bool GetTokenFromReply(const char * reply, const char * name, char ** value)
{
	int rc;
	bool status = false;
	int i;
	char **rv = NULL;
	
	rc = oauth_split_url_parameters(reply, &rv);
	qsort(rv, rc, sizeof(char *), oauth_cmpstringp);

	for (i = 0; i < rc && !status; i++)
	{
		if (value && strncmp(rv[i], name, strlen(name)) == 0)
		{
			*value = strdup(&(rv[i][strlen(name) + 1]));
			status = true;

			AMLOGDEBUG("Found \"%s\" with value of \"%s\"\n", name, *value);
		}
	}
	
	if (rv)
	{
		free(rv);
	}

	return status;
}

void DoTwitterReset()
{
	StringCbCopy( g_szTwitterAuthKey, sizeof( g_szTwitterAuthKey ), "" );
	StringCbCopy( g_szTwitterAuthSecret, sizeof( g_szTwitterAuthSecret ), "" );
	g_timetTwitterLatest = 0;

	HKEY hKey;
	DWORD dwDisposition = 0;

	if ( RegCreateKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition ) == ERROR_SUCCESS )
	{						
		RegDeleteValue( hKey, MY_REGISTRY_VALUE_TWITTER_KEY );
		RegDeleteValue( hKey, MY_REGISTRY_VALUE_TWITTER_SECRET );
		RegDeleteValue( hKey, MY_REGISTRY_VALUE_TWITTER_SCREENNAME );
		RegDeleteValue( hKey, MY_REGISTRY_VALUE_TWITTER_LATEST );

		RegCloseKey( hKey );
	}
}

void DoTwitterAuthorize()
{
	HINTERNET hSession;
	HINTERNET hConnection;
	HINTERNET hData;
	DWORD dwContext = 0;
	char szValueHostName[MAX_PATH] = {0};
	char szValueUrlPath[MAX_PATH] = {0};
	int nScheme;

	AMLOGDEBUG( "Begin DoTwitterAuthorize" );

	char * req_url = oauth_sign_url2( TWITTER_REQUEST_TOKEN_URL, NULL, OA_HMAC, NULL, TWITTER_CONSUMER_KEY, TWITTER_SECRET_KEY, NULL, NULL );
	if ( req_url )
	{
		AMLOGDEBUG("Twitter OAuth Request: %s", req_url);

		if ( CrackUrl( req_url, szValueHostName, sizeof( szValueHostName ), szValueUrlPath, sizeof( szValueUrlPath ), nScheme ) )
		{				
			hSession = InternetOpen( g_szUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
			if ( hSession )
			{
				hConnection = InternetConnect( hSession, szValueHostName, nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, dwContext );
				if ( hConnection )
				{
					DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_NO_UI | INTERNET_FLAG_NO_COOKIES;

					if ( nScheme == INTERNET_SCHEME_HTTPS )
					{
						dwFlags |= INTERNET_FLAG_SECURE;
					}

					hData = HttpOpenRequest( hConnection, "GET", szValueUrlPath, NULL, NULL, NULL, dwFlags, 0 );
					if ( hData )
					{
						if ( HttpSendRequest( hData, NULL, 0, NULL, 0 ) )
						{
							DWORD dwStatus = 0;
							DWORD dwStatusSize = sizeof( dwStatus );

							HttpQueryInfo( hData, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, &dwStatus, &dwStatusSize, NULL );

							if ( dwStatus == HTTP_STATUS_OK )
							{
								char szResponse[10 * 1024] = {0};

								if ( ReadResponseBody( hData, &szResponse[0], sizeof( szResponse ) ) )
								{
									char * t_key = NULL;
									char * t_secret = NULL;

									if (parse_reply(szResponse, &t_key, &t_secret) == 0)
									{
										StringCbCopy( g_szTwitterAuthKey, sizeof(g_szTwitterAuthKey), t_key);
										StringCbCopy( g_szTwitterAuthSecret, sizeof(g_szTwitterAuthSecret), t_secret);

										MessageBox( NULL, TWITTER_AUTHORIZE_MESSAGE, PRODUCT_NAME, MB_OK + MB_ICONINFORMATION );
									
										char szTwitterAuthUrl[MAX_PATH] = {0};

										StringCchPrintfA(szTwitterAuthUrl, sizeof(szTwitterAuthUrl), "%s?oauth_token=%s&oauth_callback=oob", TWITTER_AUTHORIZE_URL, t_key);

										AMLOGDEBUG("Twitter Login URL: %s", szTwitterAuthUrl);
										
										LaunchURL(szTwitterAuthUrl);
					
										free(t_key);
										free(t_secret);
									}
									else
									{
										AMLOGERROR( "parse_reply failed" );

										MessageBox( NULL, TWITTER_MESSAGE_AUTHORIZATION_FAILED, PRODUCT_NAME, MB_OK + MB_ICONERROR );
									}
								}
								else
								{
									AMLOGERROR( "ReadResponseBody failed" );

									MessageBox( NULL, TWITTER_MESSAGE_AUTHORIZATION_FAILED, PRODUCT_NAME, MB_OK + MB_ICONERROR );
								}
							}
							else
							{
								AMLOGERROR( "Request failed with %lu", dwStatus );

								MessageBox( NULL, TWITTER_MESSAGE_AUTHORIZATION_FAILED, PRODUCT_NAME, MB_OK + MB_ICONERROR );
							}
						}
						else
						{				
							AMLOGERROR ( "Ping failed with %ld", ::GetLastError() );
						}

						InternetCloseHandle( hData );
					}
					else
					{
						AMLOGERROR ( "HttpOpenRequest failed with %ld", ::GetLastError() );
					}

					InternetCloseHandle( hConnection );
				}
				else
				{
					AMLOGERROR ( "InternetConnect failed with %ld", ::GetLastError() );
				}

				InternetCloseHandle( hSession );
			}
			else
			{
				AMLOGERROR ( "InternetOpen failed with %ld", ::GetLastError() );
			}
		}
		else
		{		
			AMLOGERROR( "CrackUrl failed" );
		}

		free( req_url );
	}
	else
	{
		AMLOGERROR( "oauth_sign_url2 failed" );
	}

	AMLOGDEBUG( "End DoTwitterAuthorize" );
}

void DoTwitterVerify(LPCTSTR lpszPin)
{	
	char szTwitterAccessTokenUrl[MAX_PATH] = {0};
	HINTERNET hSession;
	HINTERNET hConnection;
	HINTERNET hData;
	DWORD dwContext = 0;
	char szValueHostName[MAX_PATH] = {0};
	char szValueUrlPath[MAX_PATH] = {0};
	int nScheme;

	AMLOGDEBUG( "Begin DoTwitterVerify" );

	StringCchPrintf( szTwitterAccessTokenUrl, sizeof(szTwitterAccessTokenUrl), "%s?oauth_verifier=%s", TWITTER_ACCESS_TOKEN_URL, lpszPin );

	AMLOGDEBUG( "URL = %s", szTwitterAccessTokenUrl );

	char * postarg = NULL;
	char * req_url = oauth_sign_url2( szTwitterAccessTokenUrl, &postarg, OA_HMAC, NULL, TWITTER_CONSUMER_KEY, TWITTER_SECRET_KEY, g_szTwitterAuthKey, g_szTwitterAuthSecret );
	if ( req_url )
	{
		if ( CrackUrl( req_url, szValueHostName, sizeof( szValueHostName ), szValueUrlPath, sizeof( szValueUrlPath ), nScheme ) )
		{
			hSession = InternetOpen( g_szUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
			if ( hSession )
			{
				hConnection = InternetConnect( hSession, szValueHostName, nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, dwContext );
				if ( hConnection )
				{
					DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_NO_UI | INTERNET_FLAG_NO_COOKIES;

					if ( nScheme == INTERNET_SCHEME_HTTPS )
					{
						dwFlags |= INTERNET_FLAG_SECURE;
					}

					hData = HttpOpenRequest( hConnection, "POST", szValueUrlPath, NULL, NULL, NULL, dwFlags, 0 );
					if ( hData )
					{
						if ( HttpSendRequest( hData, NULL, 0, postarg, strlen( postarg ) ) )
						{
							DWORD dwStatus = 0;
							DWORD dwStatusSize = sizeof( dwStatus );

							HttpQueryInfo( hData, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, &dwStatus, &dwStatusSize, NULL );

							if ( dwStatus == HTTP_STATUS_OK )
							{
								char szResponse[10 * 1024] = {0};

								if ( ReadResponseBody( hData, &szResponse[0], sizeof( szResponse ) ) )
								{
									char *t_key = NULL;
									char *t_secret = NULL;
									
									if (parse_reply(szResponse, &t_key, &t_secret) == 0)
									{
										StringCbCopy( g_szTwitterAuthKey, sizeof(g_szTwitterAuthKey), t_key);
										StringCbCopy( g_szTwitterAuthSecret, sizeof(g_szTwitterAuthSecret), t_secret);

										SavePreference(MY_REGISTRY_VALUE_TWITTER_KEY, t_key);
										SavePreference(MY_REGISTRY_VALUE_TWITTER_SECRET, t_secret);
								
										char * lpszScreenName = NULL;
										if (GetTokenFromReply(szResponse, TWITTER_TOKEN_SCREENNAME, &lpszScreenName))
										{
											SavePreference(MY_REGISTRY_VALUE_TWITTER_SCREENNAME, lpszScreenName);
											
											free(lpszScreenName);
										}
									
										MessageBox(NULL, TWITTER_MESSAGE_SUCCESS, PRODUCT_NAME, MB_OK + MB_ICONINFORMATION);

										free(t_key);
										free(t_secret);
									}
									else
									{
										MessageBox(NULL, TWITTER_MESSAGE_ACCESS_FAILED, PRODUCT_NAME, MB_OK + MB_ICONERROR);
									}
								}
								else
								{
									AMLOGERROR( "ReadResponseBody failed" );
								}
							}
							else
							{
								AMLOGERROR ( "HttpQueryInfo returned %ld", dwStatus );
							}
						}
						else
						{				
							AMLOGERROR ( "Ping failed with %ld", ::GetLastError() );
						}

						InternetCloseHandle( hData );
					}
					else
					{
						AMLOGERROR ( "HttpOpenRequest failed with %ld", ::GetLastError() );
					}

					InternetCloseHandle( hConnection );
				}
				else
				{
					AMLOGERROR ( "InternetConnect failed with %ld", ::GetLastError() );
				}

				InternetCloseHandle( hSession );
			}
			else
			{
				AMLOGERROR ( "InternetOpen failed with %ld", ::GetLastError() );
			}
		}
		else
		{
			AMLOGERROR ( "CrackUrl failed" );
		}
		
		free(postarg);
		free(req_url);
	}
	else
	{
		AMLOGERROR( "oauth_sign_url2 failed" );
	}

	AMLOGDEBUG( "End DoTwitterVerify" );
}

void DoTwitterTimestamp()
{
	// Get new timestamp
	g_timetTwitterLatest = time(NULL);
	
	// Save it in case we exit the media player
	SavePreference(MY_REGISTRY_VALUE_TWITTER_LATEST, g_timetTwitterLatest);
}

void DoTwitter(BOOL fPlay, int intNext)
{
	HINTERNET hSession;
	HINTERNET hConnection;
	HINTERNET hData;
	DWORD dwContext = 0;
	char szValueHostName[MAX_PATH] = {0};
	char szValueUrlPath[MAX_PATH] = {0};
	int nScheme;

	USES_CONVERSION;

	AMLOGDEBUG( "Begin DoTwitter" );

	char szPostData[5*1024] = {0};

	if ( fPlay )
	{
		char szValue[MAX_PATH] = {0};
		char szValue1[MAX_PATH] = {0};

		//StringCbCat( szPostData, sizeof( szPostData ), TWITTER_POST_VALUE );
		//StringCbCat( szPostData, sizeof( szPostData ), "=" );

		if ( strlen( g_szTwitterMessage ) == 0 )
		{
			StringCbCat( szPostData, sizeof( szPostData ), TWITTER_MESSAGE_DEFAULT );
		}
		else
		{
			StringCbCat( szPostData, sizeof( szPostData ), g_szTwitterMessage );
		}

	
		WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.artist[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
		ZeroMemory( szValue1, sizeof( szValue1 ) );
		UrlEncodeAndConcat( szValue1, sizeof( szValue1 ), szValue );
		ReplaceString( szPostData, sizeof( szPostData ), TWITTER_TAG_ARTIST, szValue1 );

		WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.name[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
		ZeroMemory( szValue1, sizeof( szValue1 ) );
		UrlEncodeAndConcat( szValue1, sizeof( szValue1 ), szValue );
		ReplaceString( szPostData, sizeof( szPostData ), TWITTER_TAG_TITLE, szValue1 );

		WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.album[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
		ZeroMemory( szValue1, sizeof( szValue1 ) );
		UrlEncodeAndConcat( szValue1, sizeof( szValue1 ), szValue );
		ReplaceString( szPostData, sizeof( szPostData ), TWITTER_TAG_ALBUM, szValue1 );

		WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.genre[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
		ZeroMemory( szValue1, sizeof( szValue1 ) );
		UrlEncodeAndConcat( szValue1, sizeof( szValue1 ), szValue );
		ReplaceString( szPostData, sizeof( szPostData ), TWITTER_TAG_GENRE, szValue1 );

		WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.kind[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
		ZeroMemory( szValue1, sizeof( szValue1 ) );
		UrlEncodeAndConcat( szValue1, sizeof( szValue1 ), szValue );
		ReplaceString( szPostData, sizeof( szPostData ), TWITTER_TAG_KIND, szValue1 );

		StringCbPrintf( szValue1, sizeof( szValue1 ), "%d", g_paPlaylist[ intNext ].m_trackInfo.trackNumber );
		ReplaceString( szPostData, sizeof( szPostData ), TWITTER_TAG_TRACK, szValue1 );

		StringCbPrintf( szValue1, sizeof( szValue1 ), "%d", g_paPlaylist[ intNext ].m_trackInfo.numTracks );
		ReplaceString( szPostData, sizeof( szPostData ), TWITTER_TAG_NUMTRACKS, szValue1 );

		StringCbPrintf( szValue1, sizeof( szValue1 ), "%d", g_paPlaylist[ intNext ].m_trackInfo.year);
		ReplaceString( szPostData, sizeof( szPostData ), TWITTER_TAG_YEAR, szValue1 );

		WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.comments[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
		ZeroMemory( szValue1, sizeof( szValue1 ) );
		UrlEncodeAndConcat( szValue1, sizeof( szValue1 ), szValue );
		ReplaceString( szPostData, sizeof( szPostData ), TWITTER_TAG_COMMENTS, szValue1 );
		
		StringCbPrintf( szValue1, sizeof( szValue1 ), "%d", g_paPlaylist[ intNext ].m_trackInfo.totalTimeInMS / 1000 );
		ReplaceString( szPostData, sizeof( szPostData ), TWITTER_TAG_TIME, szValue1 );

		StringCbPrintf( szValue1, sizeof( szValue1 ), "%d", g_paPlaylist[ intNext ].m_trackInfo.bitRate );
		ReplaceString( szPostData, sizeof( szPostData ), TWITTER_TAG_BITRATE, szValue1 );

		StringCbPrintf( szValue1, sizeof( szValue1 ), "%d", g_paPlaylist[ intNext ].m_trackInfo.userRating / 20 );
		ReplaceString( szPostData, sizeof( szPostData ), TWITTER_TAG_RATING, szValue1 );

		StringCbPrintf( szValue1, sizeof( szValue1 ), "%d", g_paPlaylist[ intNext ].m_trackInfo.discNumber );
		ReplaceString( szPostData, sizeof( szPostData ), TWITTER_TAG_DISC, szValue1 );

		StringCbPrintf( szValue1, sizeof( szValue1 ), "%d", g_paPlaylist[ intNext ].m_trackInfo.numDiscs );
		ReplaceString( szPostData, sizeof( szPostData ), TWITTER_TAG_NUMDISCS, szValue1 );

		StringCbPrintf( szValue1, sizeof( szValue1 ), "%d", g_paPlaylist[ intNext ].m_trackInfo.playCount );
		ReplaceString( szPostData, sizeof( szPostData ), TWITTER_TAG_PLAYCOUNT, szValue1 );

		WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.composer[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
		ZeroMemory( szValue1, sizeof( szValue1 ) );
		UrlEncodeAndConcat( szValue1, sizeof( szValue1 ), szValue );
		ReplaceString( szPostData, sizeof( szPostData ), TWITTER_TAG_COMPOSER, szValue1 );

		WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.grouping[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
		ZeroMemory( szValue1, sizeof( szValue1 ) );
		UrlEncodeAndConcat( szValue1, sizeof( szValue1 ), szValue );
		ReplaceString( szPostData, sizeof( szPostData ), TWITTER_TAG_GROUPING, szValue1 );

		WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.fileName[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
		ZeroMemory( szValue1, sizeof( szValue1 ) );
		UrlEncodeAndConcat( szValue1, sizeof( szValue1 ), szValue );
		ReplaceString( szPostData, sizeof( szPostData ), TWITTER_TAG_FILE, szValue1 );
	}

	//
	// Do the Ping
	//

	char szTwitterUrl[MAX_PATH] = {0};
	StringCchPrintfA(szTwitterUrl, sizeof(szTwitterUrl), "%s?%s=%s", TWITTER_UPDATE_URL, TWITTER_POST_VALUE, szPostData);

	char * postarg = NULL;
	char * req_url = oauth_sign_url2( szTwitterUrl, &postarg, OA_HMAC, NULL, TWITTER_CONSUMER_KEY, TWITTER_SECRET_KEY, g_szTwitterAuthKey, g_szTwitterAuthSecret );
	if ( req_url )
	{
		if ( CrackUrl( req_url, szValueHostName, sizeof( szValueHostName ), szValueUrlPath, sizeof( szValueUrlPath ), nScheme ) )
		{
			hSession = InternetOpen( g_szUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
			if ( hSession )
			{
				hConnection = InternetConnect( hSession, szValueHostName, nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, dwContext );
				if ( hConnection )
				{
					DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_NO_UI | INTERNET_FLAG_NO_COOKIES;

					if ( nScheme == INTERNET_SCHEME_HTTPS )
					{
						dwFlags |= INTERNET_FLAG_SECURE;
					}

					hData = HttpOpenRequest( hConnection, "POST", szValueUrlPath, NULL, NULL, NULL, dwFlags, 0 );
					if ( hData )
					{
						TCHAR szContentType[128] = "Content-Type: application/x-www-form-urlencoded\r\n";
						HttpAddRequestHeaders( hData, szContentType, -1L, HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD );

						if ( HttpSendRequest( hData, NULL, 0, postarg, strlen( postarg ) ) )
						{
							DWORD dwStatus = 0;
							DWORD dwStatusSize = sizeof( dwStatus );

							HttpQueryInfo( hData, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, &dwStatus, &dwStatusSize, NULL );

							if ( dwStatus == HTTP_STATUS_DENIED )
							{
								AMLOGERROR( "Invalid Twitter username and/or password" );
							}
							else
							{
								char szResponse[10 * 1024] = {0};

								if ( ReadResponseBody( hData, &szResponse[0], sizeof( szResponse ) ) )
								{
									TCHAR szId[1024] = {0};

									if ( GetJsonTagContents( szResponse, "id_str", szId, sizeof( szId ) ) )
									{
										DoTwitterTimestamp();

										DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_TWEET);
									}
								}
							}
						}
						else
						{				
							AMLOGERROR ( "Ping failed with %ld", ::GetLastError() );
						}

						InternetCloseHandle( hData );
					}
					else
					{
						AMLOGERROR ( "HttpOpenRequest failed with %ld", ::GetLastError() );
					}

					InternetCloseHandle( hConnection );
				}
				else
				{
					AMLOGERROR ( "InternetConnect failed with %ld", ::GetLastError() );
				}

				InternetCloseHandle( hSession );
			}
			else
			{
				AMLOGERROR ( "InternetOpen failed with %ld", ::GetLastError() );
			}
		}
	
		free(postarg);
		free(req_url);
	}

	AMLOGDEBUG( "End DoTwitter" );
}

void DoFacebookUser()
{
	HINTERNET hSession;
	HINTERNET hConnection;
	HINTERNET hData;
	DWORD dwContext = 0;
	TCHAR szPostData[5*1024] = {NULL};
	TCHAR szTimestamp[MAX_PATH] = {NULL};
	TCHAR szUid[MAX_PATH] = {NULL};
	TCHAR szPath[MAX_PATH] = {NULL};
	int nScheme = INTERNET_SCHEME_HTTPS;

	AMLOGDEBUG( "Begin DoFacebookUser" );

	StringCbPrintf( szTimestamp, sizeof( szTimestamp ), "%lu", time( NULL ) );

		StringCbCat( szPostData, sizeof( szPostData ), "&call_id=" );
		StringCbCat( szPostData, sizeof( szPostData ), szTimestamp );
		StringCbCat( szPostData, sizeof( szPostData ), "&format=XML" );
		StringCbCat( szPostData, sizeof( szPostData ), "&access_token=" );
		StringCbCat( szPostData, sizeof( szPostData ), g_szFacebookSessionKey );

		AMLOGDEBUG( "POST = %s", szPostData );

		//
		// Do the Ping
		//

		hSession = InternetOpen( g_szUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
		if ( hSession )
		{
			hConnection = InternetConnect( hSession, FACEBOOK_HOSTNAME, nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, dwContext );
			if ( hConnection )
			{
				DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_NO_UI | INTERNET_FLAG_NO_COOKIES;

				if ( nScheme == INTERNET_SCHEME_HTTPS )
				{
					dwFlags |= INTERNET_FLAG_SECURE;
				}

				StringCbPrintf( szPath, sizeof( szPath ), "%s%s", FACEBOOK_PATH, FACEBOOK_ACTION_USER );

				hData = HttpOpenRequest( hConnection, "POST", szPath, NULL, NULL, NULL, dwFlags, 0 );
				if ( hData )
				{
					TCHAR szContentType[128] = "Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n";
					HttpAddRequestHeaders( hData, szContentType, -1L, HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD );

					if ( HttpSendRequest( hData, NULL, 0, szPostData, strlen( szPostData ) ) )
					{
						char szResponse[10 * 1024] = {0};

						if ( ReadResponseBody( hData, &szResponse[0], sizeof( szResponse ) ) )
						{
							TCHAR szName[1024] = {0};

							if ( GetXMLTagContents( szResponse, FACEBOOK_RESPONSE_UID, szName, sizeof( szName ) ) )
							{
								SavePreference(MY_REGISTRY_VALUE_FACEBOOK_UID, szName);
							}
						}
					}
					else
					{				
						AMLOGERROR( "Ping failed with %ld", ::GetLastError() );
					}

					InternetCloseHandle( hData );
				}
				else
				{
					AMLOGERROR( "HttpOpenRequest failed with %ld", ::GetLastError() );
				}

				InternetCloseHandle( hConnection );
			}
			else
			{
				AMLOGERROR( "InternetConnect failed with %ld", ::GetLastError() );
			}

			InternetCloseHandle( hSession );
		}
		else
		{
			AMLOGERROR( "InternetOpen failed with %ld", ::GetLastError() );
		}

	AMLOGDEBUG( "End DoFacebookUser" );
}

void DoFacebookInfo()
{
	HINTERNET hSession;
	HINTERNET hConnection;
	HINTERNET hData;
	DWORD dwContext = 0;
	TCHAR szPostData[5*1024] = {NULL};
	TCHAR szTimestamp[MAX_PATH] = {NULL};
	TCHAR szUid[MAX_PATH] = {NULL};
	TCHAR szPath[MAX_PATH] = {NULL};
	int nScheme = INTERNET_SCHEME_HTTPS;

	AMLOGDEBUG( "Begin DoFacebookInfo" );

	StringCbPrintf( szTimestamp, sizeof( szTimestamp ), "%lu", time( NULL ) );

	if ( LoadMyPreferenceString( MY_REGISTRY_VALUE_FACEBOOK_UID, szUid, sizeof( szUid ) ) )
	{	
		StringCbCat( szPostData, sizeof( szPostData ), "&call_id=" );
		StringCbCat( szPostData, sizeof( szPostData ), szTimestamp );
		StringCbCat( szPostData, sizeof( szPostData ), "&fields=name" );
		StringCbCat( szPostData, sizeof( szPostData ), "&format=XML" );
		StringCbCat( szPostData, sizeof( szPostData ), "&access_token=" );
		StringCbCat( szPostData, sizeof( szPostData ), g_szFacebookSessionKey );
		StringCbCat( szPostData, sizeof( szPostData ), "&uids=" );
		StringCbCat( szPostData, sizeof( szPostData ), szUid );

		AMLOGDEBUG( "POST = %s", szPostData );

		//
		// Do the Ping
		//

		hSession = InternetOpen( g_szUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
		if ( hSession )
		{
			hConnection = InternetConnect( hSession, FACEBOOK_HOSTNAME, nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, dwContext );
			if ( hConnection )
			{
				DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_NO_UI | INTERNET_FLAG_NO_COOKIES;

				if ( nScheme == INTERNET_SCHEME_HTTPS )
				{
					dwFlags |= INTERNET_FLAG_SECURE;
				}

				StringCbPrintf( szPath, sizeof( szPath ), "%s%s", FACEBOOK_PATH, FACEBOOK_ACTION_INFO );

				hData = HttpOpenRequest( hConnection, "POST", szPath, NULL, NULL, NULL, dwFlags, 0 );
				if ( hData )
				{
					TCHAR szContentType[128] = "Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n";
					HttpAddRequestHeaders( hData, szContentType, -1L, HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD );

					if ( HttpSendRequest( hData, NULL, 0, szPostData, strlen( szPostData ) ) )
					{
						char szResponse[10 * 1024] = {0};

						if ( ReadResponseBody( hData, &szResponse[0], sizeof( szResponse ) ) )
						{
							TCHAR szName[1024] = {0};

							if ( GetXMLTagContents( szResponse, FACEBOOK_RESPONSE_NAME, szName, sizeof( szName ) ) )
							{
								SavePreference(MY_REGISTRY_VALUE_FACEBOOK_SCREENNAME, szName);
							}
						}
					}
					else
					{				
						AMLOGERROR( "Ping failed with %ld", ::GetLastError() );
					}

					InternetCloseHandle( hData );
				}
				else
				{
					AMLOGERROR( "HttpOpenRequest failed with %ld", ::GetLastError() );
				}

				InternetCloseHandle( hConnection );
			}
			else
			{
				AMLOGERROR( "InternetConnect failed with %ld", ::GetLastError() );
			}

			InternetCloseHandle( hSession );
		}
		else
		{
			AMLOGERROR( "InternetOpen failed with %ld", ::GetLastError() );
		}
	}

	AMLOGDEBUG( "End DoFacebookInfo" );
}

void DoTwitterUiState(HWND hwndDlg)
{
	char szValue[MAX_PATH] = {0};

	if ( LoadMyPreferenceString( MY_REGISTRY_VALUE_TWITTER_SCREENNAME, szValue, sizeof( szValue ) ) )
	{
		MySetWindowText( hwndDlg, IDC_TWITTER_SCREENNAME, szValue );

		ShowWindow( GetDlgItem( hwndDlg, IDC_TWITTER_SCREENNAME_LABEL ), SW_SHOW );
		ShowWindow( GetDlgItem( hwndDlg, IDC_TWITTER_SCREENNAME ), SW_SHOW );
		ShowWindow( GetDlgItem( hwndDlg, IDC_TWITTER_RESET), SW_SHOW );
		
		ShowWindow( GetDlgItem( hwndDlg, IDC_TWITTER_AUTHORIZE ), SW_HIDE );
		ShowWindow( GetDlgItem( hwndDlg, IDC_TWITTER_VERIFY ), SW_HIDE);
		ShowWindow( GetDlgItem( hwndDlg, IDC_TWITTER_PIN ), SW_HIDE );
		ShowWindow( GetDlgItem( hwndDlg, IDC_TWITTER_PIN_LABEL ), SW_HIDE );
	}
	else
	{
		ShowWindow( GetDlgItem( hwndDlg, IDC_TWITTER_SCREENNAME_LABEL ), SW_HIDE );
		ShowWindow( GetDlgItem( hwndDlg, IDC_TWITTER_SCREENNAME ), SW_HIDE );
		ShowWindow( GetDlgItem( hwndDlg, IDC_TWITTER_RESET ), SW_HIDE );
		
		ShowWindow( GetDlgItem( hwndDlg, IDC_TWITTER_AUTHORIZE ), SW_SHOW );
		ShowWindow( GetDlgItem( hwndDlg, IDC_TWITTER_VERIFY), SW_SHOW );
		ShowWindow( GetDlgItem( hwndDlg, IDC_TWITTER_PIN ), SW_SHOW );
		ShowWindow( GetDlgItem( hwndDlg, IDC_TWITTER_PIN_LABEL ), SW_SHOW );
	}
}

void DoFacebookReset()
{
	StringCbCopy( g_szFacebookSessionKey, sizeof( g_szFacebookSessionKey ), "" );
	g_timetFacebookLatest = 0;

	HKEY hKey;
	DWORD dwDisposition = 0;

	if ( RegCreateKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition ) == ERROR_SUCCESS )
	{						
		RegDeleteValue( hKey, MY_REGISTRY_VALUE_FACEBOOK_SESSION_KEY );
		RegDeleteValue( hKey, MY_REGISTRY_VALUE_FACEBOOK_SCREENNAME );
		RegDeleteValue( hKey, MY_REGISTRY_VALUE_FACEBOOK_UID );
		RegDeleteValue( hKey, MY_REGISTRY_VALUE_FACEBOOK_LATEST );

		RegCloseKey( hKey );
	}
}

void DoFacebookTimestamp()
{
	// Get new timestamp
	g_timetFacebookLatest = time(NULL);
	
	// Save it in case we exit the media player
	SavePreference(MY_REGISTRY_VALUE_FACEBOOK_LATEST, g_timetFacebookLatest);
}

void DoFacebookUiState(HWND hwndDlg)
{
	LONG lResult = 0;
	char szValue[MAX_PATH] = {0};

	if (LoadMyPreferenceString(MY_REGISTRY_VALUE_FACEBOOK_SCREENNAME, szValue, sizeof(szValue)))
	{
		SetWindowText( GetDlgItem( hwndDlg, IDC_FACEBOOK_SCREENNAME ), szValue );

		ShowWindow(GetDlgItem(hwndDlg, IDC_FACEBOOK_SCREENNAME_LABEL), SW_SHOW);
		ShowWindow(GetDlgItem(hwndDlg, IDC_FACEBOOK_SCREENNAME), SW_SHOW);
		ShowWindow(GetDlgItem(hwndDlg, IDC_FACEBOOK_RESET), SW_SHOW);
		
		ShowWindow(GetDlgItem(hwndDlg, IDC_FACEBOOK_ADD), SW_HIDE);
		ShowWindow(GetDlgItem(hwndDlg, IDC_FACEBOOK_AUTHORIZE), SW_HIDE);
	}
	else
	{
		ShowWindow(GetDlgItem(hwndDlg, IDC_FACEBOOK_SCREENNAME_LABEL), SW_HIDE);
		ShowWindow(GetDlgItem(hwndDlg, IDC_FACEBOOK_SCREENNAME), SW_HIDE);
		ShowWindow(GetDlgItem(hwndDlg, IDC_FACEBOOK_RESET), SW_HIDE);
		
		ShowWindow(GetDlgItem(hwndDlg, IDC_FACEBOOK_ADD), SW_SHOW);
		ShowWindow(GetDlgItem(hwndDlg, IDC_FACEBOOK_AUTHORIZE), SW_SHOW);
	}
}

void DoFacebookSubs(LPTSTR lpszMessage, size_t cbMessage, int intNext)
{
	char szValue[MAX_PATH] = {0};
	char szValue1[MAX_PATH] = {0};

	WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.artist[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_ARTIST, szValue );

	WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.name[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_TITLE, szValue );

	WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.album[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_ALBUM, szValue );

	WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.genre[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_GENRE, szValue );

	WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.kind[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_KIND, szValue );

	StringCbPrintf( szValue1, sizeof( szValue1 ), "%d", g_paPlaylist[ intNext ].m_trackInfo.trackNumber );
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_TRACK, szValue1 );

	StringCbPrintf( szValue1, sizeof( szValue1 ), "%d", g_paPlaylist[ intNext ].m_trackInfo.numTracks );
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_NUMTRACKS, szValue1 );

	StringCbPrintf( szValue1, sizeof( szValue1 ), "%d", g_paPlaylist[ intNext ].m_trackInfo.year);
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_YEAR, szValue1 );

	WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.comments[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_COMMENTS, szValue );
	
	StringCbPrintf( szValue1, sizeof( szValue1 ), "%d", g_paPlaylist[ intNext ].m_trackInfo.totalTimeInMS / 1000 );
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_TIME, szValue1 );

	StringCbPrintf( szValue1, sizeof( szValue1 ), "%d", g_paPlaylist[ intNext ].m_trackInfo.bitRate );
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_BITRATE, szValue1 );

	StringCbPrintf( szValue1, sizeof( szValue1 ), "%d", g_paPlaylist[ intNext ].m_trackInfo.userRating / 20 );
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_RATING, szValue1 );

	StringCbPrintf( szValue1, sizeof( szValue1 ), "%d", g_paPlaylist[ intNext ].m_trackInfo.discNumber );
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_DISC, szValue1 );

	StringCbPrintf( szValue1, sizeof( szValue1 ), "%d", g_paPlaylist[ intNext ].m_trackInfo.numDiscs );
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_NUMDISCS, szValue1 );

	StringCbPrintf( szValue1, sizeof( szValue1 ), "%d", g_paPlaylist[ intNext ].m_trackInfo.playCount );
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_PLAYCOUNT, szValue1 );

	WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.composer[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_COMPOSER, szValue );

	WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.grouping[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_GROUPING, szValue );

	WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.fileName[1], -1, &szValue[0], sizeof( szValue ), NULL, NULL );
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_FILE, szValue );

	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_IMAGESMALL, g_paPlaylist[ intNext ].m_szImageSmallUrl );
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_IMAGE, g_paPlaylist[ intNext ].m_szImageUrl  );
	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_IMAGELARGE,  g_paPlaylist[ intNext ].m_szImageLargeUrl );

	ReplaceString( lpszMessage, cbMessage, TWITTER_TAG_URLAMAZON,  g_paPlaylist[ intNext ].m_szUrlAmazon );

	SubBlock( lpszMessage, cbMessage, TWITTER_TAG_AMAZONMATCH_START, TWITTER_TAG_AMAZONMATCH_STOP, strlen( g_paPlaylist[intNext].m_szUrlAmazon ) > 0 ? true : false );
}

void DoFacebookStream(BOOL fPlay, int intNext)
{
	HINTERNET hSession;
	HINTERNET hConnection;
	HINTERNET hData;
	DWORD dwContext = 0;
	TCHAR szPath[MAX_PATH] = {NULL};
	int nScheme = INTERNET_SCHEME_HTTPS;

	AMLOGDEBUG( "Begin DoFacebookStream" );

	char szPostData[5*1024] = {0};
	char szMessage[5*1024] = {0};
	char szMessage1[5*1024] = {0};
	char szAttachment[5*1024] = {0};
	char szAttachment1[5*1024] = {0};
	char szActionLinks[5*1024] = {0};
	char szActionLinks1[5*1024] = {0};
	char szUid[MAX_PATH] = {NULL};

	LoadMyPreferenceString( MY_REGISTRY_VALUE_FACEBOOK_UID, szUid, sizeof( szUid ) );

	if ( fPlay && ( g_intAmazonEnabled == 0 || ( strlen( g_paPlaylist[ intNext ].m_szUrlAmazon ) > 0 && strlen( g_paPlaylist[ intNext ].m_szImageSmallUrl ) > 0 ) ) )
	{
		
		//
		// ACCESS TOKEN
		//

		StringCbCat( szPostData, sizeof( szPostData ), "&access_token=" );
		StringCbCat( szPostData, sizeof( szPostData ), g_szFacebookSessionKey );

		//
		// MESSAGE - The actual text of the post
		//

		if ( strlen( g_szFacebookMessage ) == 0 )
		{
			StringCbCat( szMessage, sizeof( szPostData ), FACEBOOK_MESSAGE_DEFAULT );
		}
		else
		{
			StringCbCat( szMessage, sizeof( szPostData ), g_szFacebookMessage );
		}

		DoFacebookSubs( szMessage, sizeof( szMessage ), intNext );
	
		SubBlock( szMessage, sizeof( szMessage ), TWITTER_TAG_AMAZONMATCH_START, TWITTER_TAG_AMAZONMATCH_STOP, strlen( g_paPlaylist[intNext].m_szUrlAmazon ) > 0 ? true : false );

		AMLOGDEBUG( "Facebook Message = %s", szMessage );

		UrlEncodeAndConcat( szMessage1, sizeof( szMessage1 ), szMessage );

		StringCbCat( szPostData, sizeof( szPostData ), "&" );
		StringCbCat( szPostData, sizeof( szPostData ), FACEBOOK_PARAMETER_MESSAGE );
		StringCbCat( szPostData, sizeof( szPostData ), "=" );
		StringCbCat( szPostData, sizeof( szPostData ), szMessage1 );

		//
		// PICTURE - Thumbnail shown from Amazon
		//

		if ( strlen( g_paPlaylist[ intNext ].m_szImageSmallUrl ) > 0 )
		{
			char * lpszImageSmallUrl = UrlDecode(g_paPlaylist[ intNext ].m_szImageSmallUrl);
			if (lpszImageSmallUrl)
			{
				StringCbCat( szPostData, sizeof( szPostData ), "&" );
				StringCbCat( szPostData, sizeof( szPostData ), FACEBOOK_PARAMETER_PICTURE );
				StringCbCat( szPostData, sizeof( szPostData ), "=" );
				UrlEncodeAndConcat( szPostData, sizeof( szPostData ), lpszImageSmallUrl );

				free(lpszImageSmallUrl);
			}
		}

        //
        // NAME - Track name
        //

		char szValueName[MAX_PATH] = {0};
		WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.name[1], -1, &szValueName[0], sizeof( szValueName ), NULL, NULL );
        
        if ( strlen( szValueName ) > 0 )
		{
			StringCbCat( szPostData, sizeof( szPostData ), "&" );
			StringCbCat( szPostData, sizeof( szPostData ), FACEBOOK_PARAMETER_NAME );
			StringCbCat( szPostData, sizeof( szPostData ), "=" );
			StringCbCat( szPostData, sizeof( szPostData ), szValueName );
		}

		//
        // LINK - Album URL from iTunes (Amazon link doesn't show you additional info)
        //

		if ( strlen( g_paPlaylist[ intNext ].m_szUrlApple ) > 0 )
		{
			StringCbCat( szPostData, sizeof( szPostData ), "&" );
			StringCbCat( szPostData, sizeof( szPostData ), FACEBOOK_PARAMETER_LINK );
			StringCbCat( szPostData, sizeof( szPostData ), "=" );
			UrlEncodeAndConcat( szPostData, sizeof( szPostData ), g_paPlaylist[ intNext ].m_szUrlApple );
		}

		//
        // CAPTION - Artist shown under track name
        //
		
		char szValueArtist[MAX_PATH] = {0};
		WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.artist[1], -1, &szValueArtist[0], sizeof( szValueArtist ), NULL, NULL );

		if ( strlen( szValueArtist ) > 0 )
		{
			StringCbCat( szPostData, sizeof( szPostData ), "&" );
			StringCbCat( szPostData, sizeof( szPostData ), FACEBOOK_PARAMETER_CAPTION );
			StringCbCat( szPostData, sizeof( szPostData ), "=" );
			StringCbCat( szPostData, sizeof( szPostData ), szValueArtist );
		}

		//
        // PROPERTIES
        //

		if (false)
		{
			char szProperties[MAX_PATH] = {0};
			bool fHasProp = false;

			StringCbCat( szProperties, sizeof( szProperties ), "\"properties\":{" );

			char szValueAlbum[MAX_PATH] = {0};
			WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.album[1], -1, &szValueAlbum[0], sizeof( szValueAlbum ), NULL, NULL );

			if ( strlen( szValueAlbum ) > 0 )
			{
				if (fHasProp) StringCbCat( szProperties, sizeof( szProperties ), "," );
				StringCbCat( szProperties, sizeof( szProperties ), "\"Album\":\"" );
				StringCbCat( szProperties, sizeof( szProperties ), szValueAlbum );
				StringCbCat( szProperties, sizeof( szProperties ), "\"" );

				fHasProp = true;
			}

			if (g_paPlaylist[ intNext ].m_trackInfo.year > 0)
			{
				char szValueYear[MAX_PATH] = {0};
				StringCbPrintf( szValueYear, sizeof( szValueYear ), "%d", g_paPlaylist[ intNext ].m_trackInfo.year );
				if ( strlen( szValueYear ) > 0 )
				{
					if (fHasProp) StringCbCat( szProperties, sizeof( szProperties ), "," );
					StringCbCat( szProperties, sizeof( szProperties ), "\"Year\":\"" );
					StringCbCat( szProperties, sizeof( szProperties ), szValueYear );
					StringCbCat( szProperties, sizeof( szProperties ), "\"" );

					fHasProp = true;
				}
			}

			StringCbCat( szProperties, sizeof( szProperties ), "}," );

			StringCbCat( szPostData, sizeof( szPostData ), "&" );
			StringCbCat( szPostData, sizeof( szPostData ), FACEBOOK_PARAMETER_PROPERTIES );
			StringCbCat( szPostData, sizeof( szPostData ), "=" );
			StringCbCat( szPostData, sizeof( szPostData ), szProperties );
		}
              
        //
        // ACTIONS - Additional sub-links
        //

		StringCbPrintf( szActionLinks, sizeof( szActionLinks ), "[{\"name\":\"Artist\",\"link\":\"http://en.wikipedia.org/w/index.php?search=%s\"}]", szValueArtist );
		UrlEncodeAndConcat( szActionLinks1, sizeof( szActionLinks1 ), szActionLinks );


		StringCbCat( szPostData, sizeof( szPostData ), "&" );
		StringCbCat( szPostData, sizeof( szPostData ), FACEBOOK_PARAMETER_ACTIONS );
		StringCbCat( szPostData, sizeof( szPostData ), "=" );
		StringCbCat( szPostData, sizeof( szPostData ), szActionLinks1 );

		//
		// DONE
		//

		AMLOGDEBUG( "POST (%d) = %s", strlen( szPostData ), szPostData );

		//
		// Do the Ping
		//

		hSession = InternetOpen( g_szUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
		if ( hSession )
		{
			hConnection = InternetConnect( hSession, FACEBOOK_HOSTNAME_GRAPH, nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, dwContext );
			if ( hConnection )
			{
				DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_NO_UI | INTERNET_FLAG_NO_COOKIES;

				if ( nScheme == INTERNET_SCHEME_HTTPS )
				{
					dwFlags |= INTERNET_FLAG_SECURE;
				}

				StringCbPrintf( szPath, sizeof( szPath ), "/%s/%s", szUid, FACEBOOK_PATH_FEED );

				hData = HttpOpenRequest( hConnection, "POST", szPath, NULL, NULL, NULL, dwFlags, 0 );
				if ( hData )
				{
					TCHAR szContentType[128] = "Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n";
					HttpAddRequestHeaders( hData, szContentType, -1L, HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD );

					if ( HttpSendRequest( hData, NULL, 0, szPostData, strlen( szPostData ) ) )
					{
						char szResponse[10 * 1024] = {0};

						if ( ReadResponseBody( hData, &szResponse[0], sizeof( szResponse ) ) )
						{
							TCHAR szErrorMsg[1024] = {0};

							if ( GetXMLTagContents( szResponse, "error_msg", szErrorMsg, sizeof( szErrorMsg ) ) )
							{
								AMLOGERROR( "%s", szErrorMsg );
							}
							else
							{
								DoFacebookTimestamp();

								DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_FACEBOOK);
							}
						}
					}
					else
					{				
						AMLOGERROR( "Ping failed with %ld", ::GetLastError() );
					}

					InternetCloseHandle( hData );
				}
				else
				{
					AMLOGERROR( "HttpOpenRequest failed with %ld", ::GetLastError() );
				}

				InternetCloseHandle( hConnection );
			}
			else
			{
				AMLOGERROR( "InternetConnect failed with %ld", ::GetLastError() );
			}

			InternetCloseHandle( hSession );
		}
		else
		{
			AMLOGERROR( "InternetOpen failed with %ld", ::GetLastError() );
		}
	}

	AMLOGDEBUG( "End DoFacebookStream" );
}

void UploadFiles(LPCTSTR lpszFileXMLLocal, LPCTSTR lpszFileXMLRemote, LPCTSTR lpszFileImageLocal, LPCTSTR lpszFileImageRemote)
{
	AMLOGDEBUG( "Begin UploadFiles" );

	if ( strlen( g_szFTPHost ) > 0 && strlen( g_szFTPUser ) > 0 && strlen( g_szFTPPassword ) > 0 && strlen( g_szFTPPath ) > 0 )
	{
		//
		// Try SFTP
		//

		if ( strcmpi( g_szUploadProtocol, UPLOAD_PROTOCOL_LABEL_SFTP ) == 0 )
		{
			IwodSFTPComPtr pFtpCom = NULL;

			HRESULT hr = NULL;
			_variant_t vFileLocal;
			_variant_t vFileRemote;

			_bstr_t bstrLicense( "GS5A-HF5Q-32NG-QRLX" );
			_bstr_t bstrSite( g_szFTPHost );
			_bstr_t bstrUser( g_szFTPUser );
			_bstr_t bstrPswd( g_szFTPPassword );

			hr = pFtpCom.CreateInstance( CLSID_wodSFTPCom, NULL );
			if ( SUCCEEDED( hr ) )
			{			
				wFtpEvents* pwEvents = new wFtpEvents( pFtpCom );

				hr = pFtpCom->put_LicenseKey( bstrLicense );
				if ( SUCCEEDED( hr ) )
				{
					hr = pFtpCom->put_Blocking( VARIANT_TRUE );
					if ( SUCCEEDED( hr ) )
					{				
						hr = pFtpCom->put_Hostname( bstrSite );
						if ( SUCCEEDED( hr ) )
						{
							hr = pFtpCom->put_Login( bstrUser );
							if ( SUCCEEDED( hr ) )
							{
								hr = pFtpCom->put_Password( bstrPswd );
								if ( SUCCEEDED( hr ) )
								{
									hr = pFtpCom->raw_Connect();
									if ( SUCCEEDED( hr ) )
									{
										//
										// Upload the image file
										//

										if ( lpszFileImageLocal && lpszFileImageRemote && strlen( lpszFileImageLocal ) && strlen( lpszFileImageRemote ) && FileExists( lpszFileImageLocal ) )
										{
											hr = pFtpCom->put_TransferMode( Binary );
											if ( SUCCEEDED( hr ) )
											{
												vFileLocal = lpszFileImageLocal;
												vFileRemote = lpszFileImageRemote;
										
												hr = pFtpCom->raw_PutFile( vFileLocal, vFileRemote );
												if ( SUCCEEDED( hr ) )
												{
													AMLOGDEBUG( "Image file uploaded to %s", lpszFileImageRemote );
												}
												else
												{
													if ( pFtpCom->LastError == 30018 )
													{
														AMLOGERROR( g_szStringMsgErrorConnect, pFtpCom->ServerErrorCode, "SFTP file upload", (LPCTSTR) pFtpCom->ServerErrorText );
													}
													else
													{																										
														AMLOGERROR( g_szStringMsgErrorConnect, pFtpCom->LastError, "SFTP file upload", (LPCTSTR) pFtpCom->GetErrorText( pFtpCom->LastError ) );
													}
												}
											}
										}

										//
										// Upload XML file
										//

										if ( lpszFileXMLLocal && lpszFileXMLRemote && strlen( lpszFileXMLLocal ) && strlen( lpszFileXMLRemote ) && FileExists( lpszFileXMLLocal ) )
										{
											hr = pFtpCom->put_TransferMode( AscII );
											if ( SUCCEEDED( hr ) )
											{
												vFileLocal = lpszFileXMLLocal;
												vFileRemote = lpszFileXMLRemote;
										
												hr = pFtpCom->raw_PutFile( vFileLocal, vFileRemote );
												if ( SUCCEEDED( hr ) )
												{
													AMLOGDEBUG( "XML file uploaded to %s", lpszFileXMLRemote );

													DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_SFTP);
												}
												else
												{
													TCHAR szMsg[1024] = {0};

													if ( pFtpCom->LastError == 30018 )
													{
														AMLOGERROR( g_szStringMsgErrorConnect, pFtpCom->ServerErrorCode, "SFTP file upload", (LPCTSTR) pFtpCom->ServerErrorText );
													}
													else
													{	
														AMLOGERROR( g_szStringMsgErrorConnect, pFtpCom->LastError, "SFTP file upload", (LPCTSTR) pFtpCom->GetErrorText( pFtpCom->LastError ) );
													}
												}
											}
										}

										hr = pFtpCom->raw_Disconnect();
										if ( FAILED( hr ) )
										{
											hr = pFtpCom->raw_Abort();
										}
									}
									else
									{
										AMLOGERROR( g_szStringMsgErrorConnect, pFtpCom->LastError, "SFTP connect", (LPCTSTR) pFtpCom->GetErrorText( pFtpCom->LastError ) );
									}
								}
							}
						}
					}
				}

				delete pwEvents;
				pFtpCom.Release();
				pFtpCom = NULL;
			}
		}
		else if ( strcmpi( g_szUploadProtocol, UPLOAD_PROTOCOL_LABEL_FTP ) == 0 )
		{
		    HINTERNET hSession = NULL;
			HINTERNET hConnection = NULL;
			DWORD dwContext = 0;

			hSession = InternetOpen( g_szUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
			if ( hSession )
			{
				hConnection = InternetConnect( hSession, g_szFTPHost, INTERNET_DEFAULT_FTP_PORT, g_szFTPUser, g_szFTPPassword, INTERNET_SERVICE_FTP, g_intFTPPassive != 0 ? INTERNET_FLAG_PASSIVE : 0, dwContext );
				if ( hConnection )
				{	
					//
					// Image file
					//
					
					if ( lpszFileImageLocal && lpszFileImageRemote && strlen( lpszFileImageLocal ) && strlen( lpszFileImageRemote ) && FileExists( lpszFileImageLocal ) )
					{
						if ( FtpPutFile( hConnection, lpszFileImageLocal, lpszFileImageRemote, FTP_TRANSFER_TYPE_BINARY, dwContext ) )
						{
							AMLOGDEBUG( "File uploaded to %s", lpszFileImageRemote );
						}
						else
						{
							DWORD dwError = GetLastError();

							AMLOGERROR( "Error %d: FtpPutFile failed for %s", dwError, lpszFileImageRemote );

							if ( dwError >= INTERNET_ERROR_BASE && dwError <= INTERNET_ERROR_LAST )
							{
								InternetErrorOut( NULL, dwError, "FTP file upload" );
							}
						}
					}
					
					//
					// XML file
					//

					if ( lpszFileXMLLocal && lpszFileXMLRemote && strlen( lpszFileXMLLocal ) && strlen( lpszFileXMLRemote ) && FileExists( lpszFileXMLLocal ) )
					{
						if ( FtpPutFile( hConnection, lpszFileXMLLocal, lpszFileXMLRemote, FTP_TRANSFER_TYPE_ASCII, dwContext ) )
						{
							AMLOGDEBUG( "File uploaded to %s", lpszFileXMLRemote );

							DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_FTP);
						}
						else
						{
							DWORD dwError = GetLastError();
															
							AMLOGERROR( "Error %d: FtpPutFile failed for %s", dwError, lpszFileXMLRemote );

							if ( dwError >= INTERNET_ERROR_BASE && dwError <= INTERNET_ERROR_LAST )
							{
								InternetErrorOut( NULL, dwError, "FTP file upload" );
							}
						}
					}
	
					InternetCloseHandle( hConnection );
				}
				else
				{
					DWORD dwError = GetLastError();
												
					AMLOGERROR( "Error %d: InternetConnect failed", dwError );

					if ( dwError >= INTERNET_ERROR_BASE && dwError <= INTERNET_ERROR_LAST )
					{
						InternetErrorOut( NULL, dwError, "FTP connect" );
					}
				}
		
				InternetCloseHandle( hSession );
			}
		}
	}

	AMLOGDEBUG( "End UploadFiles" );
}

void PlaylistStore()
{
	HKEY hKey;
	int i = 0;
	int intOrder = 1;
	DWORD dwDisposition = 0;

	AMLOGDEBUG( "Begin PlaylistStore" );
	
	if ( RegCreateKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition ) == ERROR_SUCCESS )
	{									
		if ( RegSetValueEx( hKey, MY_REGISTRY_VALUE_PLAYLIST_CACHE, 0, REG_BINARY, (BYTE*) g_paPlaylist, sizeof( CPlaylistItem ) * g_intPlaylistLength ) == ERROR_SUCCESS )
		{
			RegSetValueEx( hKey, MY_REGISTRY_VALUE_PLAYLIST_CACHE_INDEX, 0, REG_DWORD, (BYTE*) &g_intNext, sizeof( g_intNext ) );
		}

		RegCloseKey( hKey );
	}

	AMLOGDEBUG( "End PlaylistStore" );
}

void DoAmazonLookup(LPCTSTR lpszASIN, LPCTSTR lpszArtist, LPCTSTR lpszAlbum, LPTSTR lpszImageUrl, size_t cchImageUrl, LPTSTR lpszImageLargeUrl, size_t cchImageLargeUrl, LPTSTR lpszImageSmallUrl, size_t cchImageSmallUrl, LPTSTR lpszUrl, size_t cchUrl)
{
	HRESULT hr = S_OK;	
	char szUrlHost[MAX_PATH] = {0};
	char szUrlPath[5*1024] = {0};
	DWORD dwDisposition = 0;
	HINTERNET hSession;
	HINTERNET hConnection;
	HINTERNET hData;
	DWORD dwContext = 0;
	LPTSTR lpszData = NULL;
	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;

	//
	// Timestamp
	//

	time_t now_t = time( NULL );
	struct tm now;
	now = *gmtime( &now_t );			
	char szTimestamp[MAX_PATH] = {0};			
	StringCbPrintf( szTimestamp, sizeof( szTimestamp ), TIMEZONE_FORMAT_GMT_AMAZON, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec );	

	//
	// Figure out the server based on locale
	//

	if ( strcmpi( g_szAmazonLocale, AMAZON_LOCALE_LABEL_UK ) == 0 )
	{
		StringCbCopy( szUrlHost, sizeof( szUrlHost ), "ecs.amazonaws.co.uk" );
	}
	else if ( strcmpi( g_szAmazonLocale, AMAZON_LOCALE_LABEL_DE ) == 0 )
	{
		StringCbCopy( szUrlHost, sizeof( szUrlHost ), "ecs.amazonaws.de" );
	}
	else if ( strcmpi( g_szAmazonLocale, AMAZON_LOCALE_LABEL_JP ) == 0 )
	{
		StringCbCopy( szUrlHost, sizeof( szUrlHost ), "ecs.amazonaws.jp" );
	}
	else if ( strcmpi( g_szAmazonLocale, AMAZON_LOCALE_LABEL_CA ) == 0 )
	{
		StringCbCopy( szUrlHost, sizeof( szUrlHost ), "ecs.amazonaws.ca" );
	}
	else if ( strcmpi( g_szAmazonLocale, AMAZON_LOCALE_LABEL_FR ) == 0 )
	{
		StringCbCopy( szUrlHost, sizeof( szUrlHost ), "ecs.amazonaws.fr" );
	}
	else
	{
		StringCbCopy( szUrlHost, sizeof( szUrlHost ), "ecs.amazonaws.com" );
	}

	//
	// Form the query
	//

	StringCbCat( szUrlPath, sizeof( szUrlPath ), "/onca/xml?AWSAccessKeyId=" );
	UrlEncodeAndConcat( szUrlPath, sizeof( szUrlPath ), MY_AMAZON_ACCESS_KEY_ID );

	if ( g_intAmazonUseASIN > 0 && lpszASIN && strlen( lpszASIN ) == 10 )
	{		
		StringCbCat( szUrlPath, sizeof( szUrlPath ), "&AssociateTag=" );
		UrlEncodeAndConcat( szUrlPath, sizeof( szUrlPath ), strlen( g_szAmazonAssociate ) > 0 ? g_szAmazonAssociate : MY_AMAZON_ASSOCIATE_ID );

		StringCbCat( szUrlPath, sizeof( szUrlPath ), "&ItemId=" );
		UrlEncodeAndConcat( szUrlPath, sizeof( szUrlPath ), lpszASIN );

		StringCbCat( szUrlPath, sizeof( szUrlPath ), "&Operation=ItemLookup" );

		StringCbCat( szUrlPath, sizeof( szUrlPath ), "&ResponseGroup=Medium" );
		StringCbCat( szUrlPath, sizeof( szUrlPath ), "&Service=AWSECommerceService" );
		
		StringCbCat( szUrlPath, sizeof( szUrlPath ), "&Timestamp=" );
		StringCbCat( szUrlPath, sizeof( szUrlPath ), szTimestamp );
	}
	else
	{
		if ( lpszArtist )
		{
			StringCbCat( szUrlPath, sizeof( szUrlPath ), "&Artist=" );
			UrlEncodeAndConcat( szUrlPath, sizeof( szUrlPath ), lpszArtist );
		}

		StringCbCat( szUrlPath, sizeof( szUrlPath ), "&AssociateTag=" );
		UrlEncodeAndConcat( szUrlPath, sizeof( szUrlPath ), strlen( g_szAmazonAssociate ) > 0 ? g_szAmazonAssociate : MY_AMAZON_ASSOCIATE_ID );

		StringCbCat( szUrlPath, sizeof( szUrlPath ), "&Operation=ItemSearch" );
		StringCbCat( szUrlPath, sizeof( szUrlPath ), "&ResponseGroup=Medium" );
		StringCbCat( szUrlPath, sizeof( szUrlPath ), "&SearchIndex=Music" );
		StringCbCat( szUrlPath, sizeof( szUrlPath ), "&Service=AWSECommerceService" );

		StringCbCat( szUrlPath, sizeof( szUrlPath ), "&Timestamp=" );
		StringCbCat( szUrlPath, sizeof( szUrlPath ), szTimestamp );

		if ( lpszAlbum )
		{
			StringCbCat( szUrlPath, sizeof( szUrlPath ), "&Title=" );
			UrlEncodeAndConcat( szUrlPath, sizeof( szUrlPath ), lpszAlbum );
		}
	}

	char szSigData[1024 * 2] = {0};
	StringCbCat( szSigData, sizeof( szSigData ), "GET\n" );
	StringCbCat( szSigData, sizeof( szSigData ), szUrlHost );
	StringCbCat( szSigData, sizeof( szSigData ), "\n/onca/xml\n" );
	StringCbCat( szSigData, sizeof( szSigData ), strrchr( szUrlPath, '?' ) + sizeof(TCHAR) );

	// Calculate the signature
	char szSignature[MAX_PATH] = {0};
	OAuthSignHMACSHA256( szSigData, MY_AMAZON_SECRET_KEY, szSignature, sizeof( szSignature ) );

	// Add the signature to the post data
	StringCbCat( szUrlPath, sizeof( szUrlPath ), "&Signature=" );
	UrlEncodeAndConcat( szUrlPath, sizeof( szUrlPath ), szSignature );

	AMLOGDEBUG( "Amazon Request = %s", szUrlPath );

	//
	// Send the request
	//
	
	hSession = InternetOpen( g_szUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
	if ( hSession )
	{
		hConnection = InternetConnect( hSession, szUrlHost, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, dwContext );
		if ( hConnection )
		{
			hData = HttpOpenRequest( hConnection, NULL, szUrlPath, NULL, NULL, NULL, INTERNET_FLAG_RELOAD, 0 );
			if ( hData )
			{
				if ( HttpSendRequest( hData, NULL, 0, NULL, 0 ) )
				{
					bool fDone = false;
					size_t sizeNow = 0;

					while ( !fDone )
					{
						if ( InternetQueryDataAvailable( hData, &dwSize, 0, 0 ) )
						{
							if ( dwSize > 0 )
							{
								LPTSTR lpszDataChunk = (char *) malloc( dwSize + 1 );
								if ( lpszDataChunk )
								{
									if ( InternetReadFile( hData, (LPVOID) lpszDataChunk, dwSize, &dwDownloaded ) )
									{
										AMLOGDEBUG( "InternetReadFile downloaded %d bytes", dwDownloaded );

										if ( dwDownloaded > 0 )
										{
											lpszDataChunk[ dwDownloaded ] = '\0';

											AMLOGDEBUG( "Got chunk = %s", lpszDataChunk );

											//
											// Do we need to make a backup?
											//

											if ( lpszData )
											{
												LPTSTR lpszDataTemp = (char *) malloc( strlen( lpszData ) + 1 );
												StringCbCopy( lpszDataTemp, strlen( lpszData ) + 1, lpszData );

												if ( lpszDataTemp )
												{					
													free( lpszData );
											
													lpszData = (char *) malloc( strlen( lpszDataTemp ) + dwDownloaded + 1 );
													if ( lpszData )
													{
														StringCbCopy( lpszData, strlen( lpszDataTemp ) + dwDownloaded + 1, lpszDataTemp );
														StringCbCat( lpszData, strlen( lpszDataTemp ) + dwDownloaded + 1, lpszDataChunk );
													}											

													free( lpszDataTemp );
												}
											}
											else
											{
												lpszData = (char *) malloc( dwDownloaded + 1 );
												if ( lpszData )
												{
													StringCbCopy( lpszData, dwDownloaded + 1, lpszDataChunk );
												}											

											}
										}
									}

									free( lpszDataChunk );
								}
							}
							else
							{
								fDone = true;
							}
						}
					}

					if ( lpszData )
					{
#ifdef _DEBUG
						FILE* pFile = fopen( "C:\\AmazonSearch.html", "wt" );
						if ( pFile )
						{
							fputs( lpszData, pFile );
										
							fclose( pFile );
						}
#endif

						//
						// Find the data you want
						//

						TCHAR szTempImage[1024] = {0};


						if ( GetXMLTagContents( lpszData, "SmallImage", szTempImage, sizeof( szTempImage ) ) )
						{
							GetXMLTagContents( szTempImage, "URL", lpszImageSmallUrl, cchImageSmallUrl );
						}

						if ( GetXMLTagContents( lpszData, "MediumImage", szTempImage, sizeof( szTempImage ) ) )
						{
							GetXMLTagContents( szTempImage, "URL", lpszImageUrl, cchImageUrl );
						}

						if ( GetXMLTagContents( lpszData, "LargeImage", szTempImage, sizeof( szTempImage ) ) )
						{
							if ( GetXMLTagContents( szTempImage, "URL", lpszImageLargeUrl, cchImageLargeUrl ) )
							{
								StringCbCopy( g_szAlbumArtUrl, sizeof( g_szAlbumArtUrl ), lpszImageLargeUrl );

								FreeVisualImage();

								DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_AMAZON);
							}
							else
							{
								g_szAlbumArtUrl[0] = '\0';
							}
						}
						
						GetXMLTagContents( lpszData, "DetailPageURL", lpszUrl, cchUrl );

						//
						// Done
						//

						free( lpszData );
					}
				}

				InternetCloseHandle( hData );
			}

			InternetCloseHandle( hConnection );
		}

		InternetCloseHandle( hSession );
	}
}

void DoTrackInfo(IXMLDOMDocumentPtr& pXMLDOMDoc,IXMLDOMElementPtr& pElementNP, const int i, const int intOrder)
{
	USES_CONVERSION;

	if ( pXMLDOMDoc )
	{
		IXMLDOMElementPtr pElementSong = pXMLDOMDoc->createElement("song");
		if ( pElementSong )
		{
			if ( g_intPlaylistLength > 1 )
			{
				IXMLDOMAttributePtr pa = pXMLDOMDoc->createAttribute("order");
				if ( pa ) 
				{
					pa->value = (short) intOrder;
					pElementSong->setAttributeNode( pa );
					pa.Release();
				}
			}

			IXMLDOMAttributePtr pAttribTime = pXMLDOMDoc->createAttribute( "timestamp" );
			if ( pAttribTime ) 
			{				
				pAttribTime->value = g_paPlaylist[i].m_szTimestamp;
				pElementSong->setAttributeNode( pAttribTime );
				pAttribTime.Release();
			}

			pElementSong->appendChild( pXMLDOMDoc->createTextNode( "\n" ) );

			PrintTrackInfoStringToFile( pXMLDOMDoc, pElementSong, TAG_TITLE, &g_paPlaylist[i].m_trackInfo.name[1], g_intUseXmlCData > 0 ? true : false );
			PrintTrackInfoStringToFile( pXMLDOMDoc, pElementSong, TAG_ARTIST, &g_paPlaylist[i].m_trackInfo.artist[1], g_intUseXmlCData > 0 ? true : false  );
			PrintTrackInfoStringToFile( pXMLDOMDoc, pElementSong, TAG_ALBUM, &g_paPlaylist[i].m_trackInfo.album[1], g_intUseXmlCData > 0 ? true : false  );
			PrintTrackInfoStringToFile( pXMLDOMDoc, pElementSong, TAG_GENRE, &g_paPlaylist[i].m_trackInfo.genre[1], g_intUseXmlCData > 0 ? true : false  );
			PrintTrackInfoStringToFile( pXMLDOMDoc, pElementSong, TAG_KIND, &g_paPlaylist[i].m_trackInfo.kind[1], false );
			PrintTrackInfoNumberToFile( pXMLDOMDoc, pElementSong, TAG_TRACK, g_paPlaylist[i].m_trackInfo.trackNumber );
			
			if ( g_intMediaPlayerType == MEDIA_PLAYER_ITUNES || g_intMediaPlayerType == MEDIA_PLAYER_WMP )
			{		
				PrintTrackInfoNumberToFile( pXMLDOMDoc, pElementSong, TAG_NUMTRACKS, g_paPlaylist[i].m_trackInfo.numTracks );
			}

			PrintTrackInfoNumberToFile( pXMLDOMDoc, pElementSong, TAG_YEAR, g_paPlaylist[i].m_trackInfo.year );
			PrintTrackInfoStringToFile( pXMLDOMDoc, pElementSong, TAG_COMMENTS, &g_paPlaylist[i].m_trackInfo.comments[1], g_intUseXmlCData > 0 ? true : false  );
			PrintTrackInfoNumberToFile( pXMLDOMDoc, pElementSong, TAG_TIME, g_paPlaylist[i].m_trackInfo.totalTimeInMS / 1000 );
			PrintTrackInfoNumberToFile( pXMLDOMDoc, pElementSong, TAG_BITRATE, g_paPlaylist[i].m_trackInfo.bitRate );

			if ( g_intMediaPlayerType == MEDIA_PLAYER_ITUNES || g_intMediaPlayerType == MEDIA_PLAYER_WMP )
			{
				PrintTrackInfoNumberToFile( pXMLDOMDoc, pElementSong, TAG_RATING, g_paPlaylist[i].m_trackInfo.userRating / 20 );
			}

			PrintTrackInfoNumberToFile( pXMLDOMDoc, pElementSong, TAG_DISC, g_paPlaylist[i].m_trackInfo.discNumber );

			if ( g_intMediaPlayerType == MEDIA_PLAYER_ITUNES || g_intMediaPlayerType == MEDIA_PLAYER_WMP )
			{
				PrintTrackInfoNumberToFile( pXMLDOMDoc, pElementSong, TAG_NUMDISCS, g_paPlaylist[i].m_trackInfo.numDiscs );
			}

			PrintTrackInfoNumberToFile( pXMLDOMDoc, pElementSong, TAG_PLAYCOUNT, g_paPlaylist[i].m_trackInfo.playCount );
			PrintTrackInfoStringToFile( pXMLDOMDoc, pElementSong, TAG_COMPILATION, g_paPlaylist[i].m_trackInfo.isCompilationTrack == TRUE ? L"Yes" : L"No", false );

			if ( g_intAmazonEnabled )
			{
				PrintTrackInfoStringToFile( pXMLDOMDoc, pElementSong, TAG_URLAMAZON, g_paPlaylist[i].m_szUrlAmazon, false );
			}

			if ( g_intAppleEnabled )
			{
				PrintTrackInfoStringToFile( pXMLDOMDoc, pElementSong, TAG_URLAPPLE, g_paPlaylist[i].m_szUrlApple, false );
			}

			if ( g_intAmazonEnabled )
			{
				PrintTrackInfoStringToFile( pXMLDOMDoc, pElementSong, TAG_IMAGESMALL, g_paPlaylist[i].m_szImageSmallUrl, false );
				PrintTrackInfoStringToFile( pXMLDOMDoc, pElementSong, TAG_IMAGE, g_paPlaylist[i].m_szImageUrl, false );
				PrintTrackInfoStringToFile( pXMLDOMDoc, pElementSong, TAG_IMAGELARGE, g_paPlaylist[i].m_szImageLargeUrl, false );
			}

			PrintTrackInfoStringToFile( pXMLDOMDoc, pElementSong, TAG_COMPOSER, &g_paPlaylist[i].m_trackInfo.composer[1],  g_intUseXmlCData > 0 ? true : false );
			PrintTrackInfoStringToFile( pXMLDOMDoc, pElementSong, TAG_GROUPING,  &g_paPlaylist[i].m_trackInfo.grouping[1], g_intUseXmlCData > 0 ? true : false );
			PrintTrackInfoStringToFile( pXMLDOMDoc, pElementSong, TAG_URLSOURCE, g_paPlaylist[i].m_szUrlSource, false );
			PrintTrackInfoStringToFile( pXMLDOMDoc, pElementSong, TAG_FILE, &g_paPlaylist[i].m_trackInfo.fileName[1], g_intUseXmlCData > 0 ? true : false );

			if ( g_intMediaPlayerType == MEDIA_PLAYER_ITUNES || g_intMediaPlayerType == MEDIA_PLAYER_WMP )
			{
				PrintTrackInfoStringToFile( pXMLDOMDoc, pElementSong, TAG_ARTWORKID, g_paPlaylist[i].m_szArtworkID, false );
			}

			if ( g_intMediaPlayerType == MEDIA_PLAYER_WMP )
			{
				char szTempDir[256] = {0};
				StringCbCopy( szTempDir, sizeof( szTempDir ), W2T(&g_paPlaylist[i].m_trackInfo.fileName[1]) );
				MyPathRemoveFileSpec(szTempDir);

				char szTempLocalFile[256] = {0};

				StringCbPrintf( szTempLocalFile, sizeof( szTempLocalFile ), "%sAlbumArt_%s_Small.jpg", szTempDir, g_paPlaylist[i].m_szArtworkID );
				if ( FileExists( szTempLocalFile ) )
				{
					PrintTrackInfoStringToFile( pXMLDOMDoc, pElementSong, TAG_IMAGELOCALSMALL, szTempLocalFile , false );
				}

				StringCbPrintf( szTempLocalFile, sizeof( szTempLocalFile ), "%sAlbumArt_%s_Large.jpg", szTempDir, g_paPlaylist[i].m_szArtworkID );
				if ( FileExists( szTempLocalFile ) )
				{
					PrintTrackInfoStringToFile( pXMLDOMDoc, pElementSong, TAG_IMAGELOCALLARGE, szTempLocalFile, false );
				}
			}

			pElementSong->appendChild( pXMLDOMDoc->createTextNode( "\t" ) );

			pElementNP->appendChild( pXMLDOMDoc->createTextNode( "\t" ) );
			pElementNP->appendChild( pElementSong );
			pElementNP->appendChild( pXMLDOMDoc->createTextNode( "\n" ) );
			pElementSong.Release();	
		}
	}
}

bool WriteXML(int intNext, bool fPlaying, LPCTSTR lpszFile)
{
	bool fSuccess = false;

	AMLOGDEBUG( "Begin WriteXML" );

	if ( strlen( lpszFile ) )
	{
		HRESULT hr = S_OK;
		int i = 0;
		int intOrder = 1;
		char szTemp[256];

		//
		// Setup timestamp
		//

		time_t now_t;
		struct tm now;
		time( &now_t );
		now = *localtime( &now_t );
		LONG lTimezone = GetTimezoneInMinutes();

		//
		// Write XML file
		//

		try
		{
			IXMLDOMDocumentPtr pXMLDOMDoc; 
			
			hr = pXMLDOMDoc.CreateInstance( __uuidof( DOMDocument ) );
			if ( SUCCEEDED( hr ) )
			{
				//
				// Create a processing instruction targeted for xml.
				//

				StringCbPrintf( szTemp, sizeof( szTemp ), "version='1.0' encoding='%s'", g_szXMLEncoding );

				IXMLDOMProcessingInstructionPtr piXml = pXMLDOMDoc->createProcessingInstruction( "xml", szTemp );
				if ( piXml ) 
				{
					pXMLDOMDoc->appendChild( piXml );
					piXml.Release();
				}

				//
				// Create a processing instruction targeted for xml-stylesheet.
				//

				if ( strlen( g_szStyleSheet ) > 0 )
				{
					StringCbPrintf( szTemp, sizeof( szTemp ), "type='text/xsl' href='%s'", g_szStyleSheet );

					IXMLDOMProcessingInstructionPtr piStyle = pXMLDOMDoc->createProcessingInstruction( "xml-stylesheet", szTemp );
					if ( piStyle )
					{
						pXMLDOMDoc->appendChild( piStyle );
						piStyle.Release();
					}
				}
			
				//
				// Create the root element (i.e., the documentElement).
				//

				IXMLDOMElementPtr pe;
				pe = pXMLDOMDoc->createElement("now_playing");
				if ( pe )
				{
					IXMLDOMAttributePtr pAttribPlaying = pXMLDOMDoc->createAttribute( "playing" );
					if ( pAttribPlaying ) 
					{
						if ( fPlaying )
						{
							pAttribPlaying->value = "1";
						}
						else
						{
							pAttribPlaying->value = "0";
						}

						pe->setAttributeNode( pAttribPlaying );
						pAttribPlaying.Release();
					}

					IXMLDOMAttributePtr pAttribTime = pXMLDOMDoc->createAttribute( "timestamp" );
					if ( pAttribTime ) 
					{				
						char szTemp[256];
						
						if ( lTimezone == 0 )
						{
							StringCbPrintf( szTemp, sizeof( szTemp ), TIMEZONE_FORMAT_GMT, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec );
						}
						else
						{
							StringCbPrintf( szTemp, sizeof( szTemp ), TIMEZONE_FORMAT_ALL, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec, lTimezone < 0 ? "+" : "-", abs( lTimezone / 60 ), abs( lTimezone - ( lTimezone / 60 ) * 60 ) );
						}

						pAttribTime->value = szTemp;

						pe->setAttributeNode( pAttribTime );
						pAttribTime.Release();
					}

					//
					// Add the root element to the DOM instance.
					//

					pXMLDOMDoc->appendChild( pe );
					pXMLDOMDoc->documentElement->appendChild( pXMLDOMDoc->createTextNode( "\n" ) );

					pe.Release();

					if ( g_intPlaylistLength > 1 )
					{				
						for ( i = intNext; i >= 0; i-- )
						{
							if ( g_paPlaylist[ i ].m_fSet )
							{
								DoTrackInfo( pXMLDOMDoc, pXMLDOMDoc->documentElement, i, intOrder );
								intOrder++;
							}
						}

						for ( i = g_intPlaylistLength - 1; i > intNext; i-- )
						{
							if ( g_paPlaylist[ i ].m_fSet )
							{
								DoTrackInfo( pXMLDOMDoc, pXMLDOMDoc->documentElement, i, intOrder );
								intOrder++;
							}
						}
					}
					else if ( g_paPlaylist[ i ].m_fSet )
					{
						DoTrackInfo( pXMLDOMDoc, pXMLDOMDoc->documentElement, i, intOrder );
					}
				}

				//
				// Write it out
				//

				CheckParentDirectory( lpszFile );

				AMLOGDEBUG( "Save XML = %s", lpszFile );

				pXMLDOMDoc->save( lpszFile );

				//
				// Success
				//

				fSuccess = true;
			}
			else
			{
				AMLOGERROR( "CreateInstance failed (0x%08X)", hr );
			}
		}
		catch( _com_error e )
		{
			AMLOGERROR( "Exception 0x%08X occured: %s", e.Error(), (LPCTSTR) e.Description() );
		}

		AMLOGDEBUG( "End WriteXML" );
	}
	else
	{
		AMLOGDEBUG( "No XML file location configured" );
	}

	return fSuccess;
}

void DoStop()
{
	char szFilePreparedLocal[MAX_PATH] = {0};
	char szFilePreparedRemote[MAX_PATH] = {0};

	if ( g_intPublishStop )
	{
		//
		// Do the XML output
		//

		PrepareFilename( g_szOutputFile, szFilePreparedLocal, sizeof( szFilePreparedLocal ) );

		if ( WriteXML( g_intNext, false, szFilePreparedLocal ) )
		{
			//
			// Prepare the file name
			//

			PrepareFilename( g_szFTPPath, szFilePreparedRemote, sizeof( szFilePreparedRemote ) );

			//
			// Upload
			//

			UploadFiles( szFilePreparedLocal, szFilePreparedRemote, NULL, NULL );
		}

		//
		// DoPing
		//

		if ( strlen( g_szTrackBackUrl ) > 0 )
		{		
			DoPing( FALSE, 0 );
		}
	}
}

void DoPlayWorker(int intNext)
{
	int nSize = 0;
	char szFilePreparedLocal[MAX_PATH] = {0};
	char szFilePreparedRemote[MAX_PATH] = {0};
	char szArtworkPathLocal[MAX_PATH] = {0};
	char szArtworkPathRemote[MAX_PATH] = {0};

	USES_CONVERSION;


	DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_PLAY);

	//
	// Calculate the XML file name
	//
	
	PrepareFilename( g_szOutputFile, szFilePreparedLocal, sizeof( szFilePreparedLocal ) );

	//
	// Do Amazon lookup
	//

	if ( g_intAmazonEnabled )
	{
		if ( g_paPlaylist[ intNext ].m_trackInfo.isCompilationTrack == TRUE )
		{
			DoAmazonLookup( W2A( &g_paPlaylist[ intNext ].m_trackInfo.grouping[1] ), NULL, W2A( &g_paPlaylist[ intNext ].m_trackInfo.album[1] ), g_paPlaylist[ intNext ].m_szImageUrl, sizeof( g_paPlaylist[ intNext ].m_szImageUrl ), g_paPlaylist[ intNext ].m_szImageLargeUrl, sizeof( g_paPlaylist[ intNext ].m_szImageLargeUrl ), g_paPlaylist[ intNext ].m_szImageSmallUrl, sizeof( g_paPlaylist[ intNext ].m_szImageSmallUrl ), g_paPlaylist[ intNext ].m_szUrlAmazon, sizeof( g_paPlaylist[ intNext ].m_szUrlAmazon ) );
		}
		else
		{
			DoAmazonLookup( W2A( &g_paPlaylist[ intNext ].m_trackInfo.grouping[1] ), W2A( &g_paPlaylist[ intNext ].m_trackInfo.artist[1] ), W2A( &g_paPlaylist[ intNext ].m_trackInfo.album[1] ), g_paPlaylist[ intNext ].m_szImageUrl, sizeof( g_paPlaylist[ intNext ].m_szImageUrl ), g_paPlaylist[ intNext ].m_szImageLargeUrl, sizeof( g_paPlaylist[ intNext ].m_szImageLargeUrl ), g_paPlaylist[ intNext ].m_szImageSmallUrl, sizeof( g_paPlaylist[ intNext ].m_szImageSmallUrl ), g_paPlaylist[ intNext ].m_szUrlAmazon, sizeof( g_paPlaylist[ intNext ].m_szUrlAmazon ) );

		}
	}

	//
	// Do Apple lookup
	//

	if ( g_intAppleEnabled )
	{
		DoAppleLookup( W2A( &g_paPlaylist[ intNext ].m_trackInfo.artist[1] ), W2A( &g_paPlaylist[ intNext ].m_trackInfo.album[1] ), W2A( &g_paPlaylist[ intNext ].m_trackInfo.name[1] ), g_paPlaylist[ intNext ].m_szUrlApple, sizeof( g_paPlaylist[ intNext ].m_szUrlApple ) );
	}

	//
	// Do artwork export
	//
	
	if ( g_intArtworkUpload > 0 )
	{		
		//
		// Setup the storage paths
		//			

		StringCbCopy( szArtworkPathLocal, sizeof( szArtworkPathLocal ), szFilePreparedLocal );
		PathRemoveFileSpec( szArtworkPathLocal );

		StringCbCopy( szArtworkPathRemote, sizeof( szArtworkPathRemote ), g_szFTPPath );
		MyPathRemoveFileSpec( szArtworkPathRemote );

		//
		// Calculate the artwork ID by combining the artist and the album
		//

		char szArtworkID[1024] = {0};

		nSize = WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.artist[1], -1, NULL, 0, NULL, NULL );
		if ( nSize > 0 )
		{
			LPTSTR lpszValue = new char[ nSize ];
			if ( lpszValue )
			{
				WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.artist[1], -1, lpszValue, nSize, NULL, NULL );
				StringCbCat( szArtworkID, sizeof( szArtworkID ), lpszValue );
				delete [] lpszValue;
			}
		}

		nSize = WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.album[1], -1, NULL, 0, NULL, NULL );
		if ( nSize > 0 )
		{
			LPTSTR lpszValue = new char[ nSize ];
			if ( lpszValue )
			{
				WideCharToMultiByte( CP_UTF8, 0, &g_paPlaylist[ intNext ].m_trackInfo.album[1], -1, lpszValue, nSize, NULL, NULL );
				StringCbCat( szArtworkID, sizeof( szArtworkID ), lpszValue );
				delete [] lpszValue;
			}
		}

		MD5_CTX udtMD5Context;
		unsigned char szDigest[16];

		MD5Init( &udtMD5Context );
		MD5Update( &udtMD5Context, (unsigned char *) szArtworkID, strlen( szArtworkID ) );
		MD5Final( szDigest, &udtMD5Context );

		StringCchPrintfA( &szArtworkID[0], sizeof( szArtworkID ),
				"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				szDigest[0], szDigest[1], szDigest[2], szDigest[3], szDigest[4], szDigest[5],
				szDigest[6], szDigest[7], szDigest[8], szDigest[9], szDigest[10], szDigest[11],
				szDigest[12], szDigest[13], szDigest[14], szDigest[15] );

		StringCbCat( szArtworkPathLocal, sizeof( szArtworkPathLocal ), "\\now_playing-" );
		StringCbCat( szArtworkPathRemote, sizeof( szArtworkPathRemote ), "now_playing-" );

		StringCbCat( szArtworkPathLocal, sizeof( szArtworkPathLocal ), szArtworkID );
		StringCbCat( szArtworkPathRemote, sizeof( szArtworkPathRemote ), szArtworkID );

		//
		// Try and save the artwork to a file if supported
		//

		if ( ExportArtwork( szArtworkPathLocal, sizeof( szArtworkPathLocal ), szArtworkPathRemote, sizeof( szArtworkPathRemote ) ) )
		{		
			ResizeArtwork( szArtworkPathLocal );

			StringCbCopy( g_paPlaylist[ intNext ].m_szArtworkID, sizeof( g_paPlaylist[ intNext ].m_szArtworkID ), szArtworkID );
		}
		else
		{
			ZeroMemory( szArtworkPathLocal, sizeof( szArtworkPathLocal ) );
			ZeroMemory( szArtworkPathRemote, sizeof( szArtworkPathRemote ) );
		}
	}

	//
	// Do the XML output
	//

	if ( WriteXML( intNext, true, szFilePreparedLocal ) )
	{
		//
		// Prepare the file name
		//

		PrepareFilename( g_szFTPPath, szFilePreparedRemote, sizeof( szFilePreparedRemote ) );

		//
		// Upload
		//

		UploadFiles( szFilePreparedLocal, szFilePreparedRemote, szArtworkPathLocal, szArtworkPathRemote );

		//
		// Delete the artwork file
		//

		if ( strlen( szArtworkPathLocal ) > 0 && strlen( g_szFTPPath ) > 0 )
		{
			MyDeleteFile( szArtworkPathLocal );
		}
	}

	//
	// Do the TrackBack ping
	//

	if ( strlen( g_szTrackBackUrl ) > 0 )
	{
		DoPing( TRUE, intNext );
	}

	//
	// Do Twitter
	//

	if ( g_intTwitterEnabled && strlen( g_szTwitterAuthKey ) > 0 && strlen( g_szTwitterAuthSecret ) > 0 )
	{				
		time_t now = time(NULL);
		time_t then =  g_timetTwitterLatest + g_intTwitterRateLimitMinutes * 60;
		
		if (now > then)
		{
			DoTwitter( TRUE, intNext );
		}
		else
		{					
			AMLOGDEBUG("Rate limited at %d minutes; %lu seconds until resume; Skipping Twitter", g_intTwitterRateLimitMinutes, then - now);					
		}
	}

	//
	// Do Facebook
	//

	if ( g_intFacebookEnabled && strlen( g_szFacebookSessionKey ) > 0 )
	{
		time_t now = time(NULL);
		time_t then =  g_timetFacebookLatest + g_intFacebookRateLimitMinutes * 60;

		if (now > then)
		{
			DoFacebookStream( TRUE, intNext );
		}
		else
		{					
			AMLOGDEBUG("Rate limited at %d minutes; %lu seconds until resume; Skipping Facebook", g_intFacebookRateLimitMinutes, then - now);					
		}
	}
}

void FreeVisualImage()
{
	if ( g_hBitmap )
	{
		DeleteObject( g_hBitmap );
					
		g_hBitmap = NULL;
	}
}

void DoFacebookAuthorize()
{
	bool fResult = false;
	HINTERNET hSession;
	HINTERNET hConnection;
	HINTERNET hData;
	DWORD dwContext = 0;
	char szUrl[MAX_PATH] = {NULL};
	char szToken[MAX_PATH] = {NULL};
	char szPath[MAX_PATH] = {NULL};
	int nScheme = INTERNET_SCHEME_HTTP;

	AMLOGDEBUG( "Begin DoFacebookAuthorize" );

	hSession = InternetOpen( g_szUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
	if ( hSession )
	{
		hConnection = InternetConnect( hSession, FACEBOOK_AUTHORIZE_HOSTNAME, nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, dwContext );
		if ( hConnection )
		{
			DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | INTERNET_FLAG_NO_UI | INTERNET_FLAG_NO_COOKIES;

			if ( nScheme == INTERNET_SCHEME_HTTPS )
			{
				dwFlags |= INTERNET_FLAG_SECURE;
			}

			StringCbPrintf( szPath, sizeof( szPath ), "%s%s", FACEBOOK_AUTHORIZE_PATH, g_szFacebookAuthToken );

			hData = HttpOpenRequest( hConnection, "GET", szPath, NULL, NULL, NULL, dwFlags, 0 );
			if ( hData )
			{			
				if ( HttpSendRequest( hData, NULL, 0, NULL, 0 ) )
				{
					char szResponse[10 * 1024] = {0};

					if ( ReadResponseBody( hData, &szResponse[0], sizeof( szResponse ) ) )
					{
						TCHAR szSessionKey[MAX_PATH] = {NULL};

						if ( GetXMLTagContents( szResponse, FACEBOOK_RESPONSE_SESSION_KEY, g_szFacebookSessionKey, sizeof( g_szFacebookSessionKey ) ) )
						{
							HKEY hKey;
							DWORD dwDisposition = 0;

							if ( RegCreateKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition ) == ERROR_SUCCESS )
							{						
								RegSetValueEx( hKey, MY_REGISTRY_VALUE_FACEBOOK_SESSION_KEY, 0, REG_SZ, (BYTE*) &g_szFacebookSessionKey[0], strlen( g_szFacebookSessionKey ) + 1 );
										
								RegCloseKey( hKey );
							}

							DoFacebookUser();

							DoFacebookInfo();

							MessageBox( NULL, "Facebook authorization successfully completed!", PRODUCT_NAME, MB_OK + MB_ICONINFORMATION );

							fResult = true;
						}
					}
				}
				else
				{				
					AMLOGERROR ( "Ping failed with %ld", ::GetLastError() );
				}

				InternetCloseHandle( hData );
			}
			else
			{
				AMLOGERROR( "HttpOpenRequest failed with %ld", ::GetLastError() );
			}

			InternetCloseHandle( hConnection );
		}
		else
		{
			AMLOGERROR( "InternetConnect failed with %ld", ::GetLastError() );
		}

		InternetCloseHandle( hSession );
	}
	else
	{
		AMLOGERROR( "InternetOpen failed with %ld", ::GetLastError() );
	}

	if ( !fResult )
	{
		MessageBox( NULL, "Facebook authorization was not successful.", PRODUCT_NAME, MB_OK + MB_ICONERROR );
	}

	AMLOGDEBUG( "End DoFacebookAuthorize" );
}

void DoFacebookAdd()
{
	char szUrl[MAX_URL] = {NULL};
	unsigned char* lpszUuid = NULL;
	UUID uuid;

	AMLOGDEBUG( "Begin DoFacebookAdd" );

	ZeroMemory(&uuid, sizeof(UUID));

	UuidCreate(&uuid);

	UuidToString(&uuid, &lpszUuid);

	StringCchPrintfA( &szUrl[0], sizeof( szUrl ), "%s%s&version=%s", FACEBOOK_URL_ADD, lpszUuid, g_szVersion );

	MessageBox( NULL, FACEBOOK_MESSAGE_ADD, PRODUCT_NAME, MB_OK + MB_ICONINFORMATION );

	ShellExecute( NULL, "open", szUrl, NULL, NULL, SW_SHOW ); 

	StringCbCopy( g_szFacebookAuthToken, sizeof( g_szFacebookAuthToken ), (const char *) lpszUuid );

	if (lpszUuid != NULL)
	{
		RpcStringFree(&lpszUuid);
	}

	AMLOGDEBUG( "End DoFacebookAdd" );
}

BOOL CALLBACK OptionDlgAboutPageProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
	char szVersion[MAX_PATH] = {0};
	char szVersionLabel[MAX_PATH] = {0};

	switch ( message ) 
	{ 
		case WM_INITDIALOG: 
		{
			//
			// Setup the UI
			//

			SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_URL ), URL_HOME );

			//
			// Load up the values
			//

			GetMyVersion( szVersion, sizeof( szVersion ) );

			StringCbPrintf( szVersionLabel, sizeof( szVersionLabel ), "Version: %s", szVersion );

			SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_VERSION ), szVersionLabel );

			return TRUE;
		}
    } 

    return FALSE; 
} 

BOOL CALLBACK OptionDlgOptionsPageProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
	HKEY hKey;
	BOOL fTranslated = FALSE;
	DWORD dwDisposition = 0;
	int intPlaylistLength = DEFAULT_PLAYLIST_LENGTH;

	switch ( message ) 
	{ 
		case WM_INITDIALOG: 
		{
			//
			// Load up the values
			//

			SetDlgItemInt( hwndDlg, IDC_EDIT_DELAY, g_intPlaylistBufferDelay, TRUE );
			SetDlgItemInt( hwndDlg, IDC_EDIT_SKIPSHORT, g_intSkipShort, TRUE );
			SetDlgItemText( hwndDlg, IDC_EDIT_SKIPKINDS, g_szSkipKinds );

			CheckDlgButton( hwndDlg, IDC_CHECK_SEND_STOP, g_intPublishStop == 0 ? BST_UNCHECKED : BST_CHECKED );
			CheckDlgButton( hwndDlg, IDC_CHECK_CLEAR_PLAYLIST, g_intPlaylistClear == 0 ? BST_UNCHECKED : BST_CHECKED );

			SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_LOGGING ), CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) LOGGING_LABEL_NONE );
			SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_LOGGING ), CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) LOGGING_LABEL_ERROR );
			SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_LOGGING ), CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) LOGGING_LABEL_INFO );
			SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_LOGGING ), CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) LOGGING_LABEL_DEBUG );

			switch ( g_AmLog.GetLogLevel() )
			{				
				case CAmLoglevelError:
					SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_LOGGING ), CB_SELECTSTRING, -1, (LPARAM) (LPCTSTR) LOGGING_LABEL_ERROR );
					break;
				case CAmLoglevelInfo:
					SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_LOGGING ), CB_SELECTSTRING, -1, (LPARAM) (LPCTSTR) LOGGING_LABEL_INFO );
					break;
				case CAmLoglevelDebug:
					SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_LOGGING ), CB_SELECTSTRING, -1, (LPARAM) (LPCTSTR) LOGGING_LABEL_DEBUG );
					break;
				default:
					SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_LOGGING ), CB_SELECTSTRING, -1, (LPARAM) (LPCTSTR) LOGGING_LABEL_NONE );
					break;
			}

			return TRUE;
		}

		case WM_COMMAND:
		{
			//
			// Enable the Apply button if changes were made
			//
			
			if ( LOWORD( wParam ) == IDC_COMBO_LOGGING && HIWORD( wParam ) == CBN_SELCHANGE ||
				 LOWORD( wParam ) == IDC_EDIT_DELAY && HIWORD( wParam ) == EN_CHANGE ||
				 LOWORD( wParam ) == IDC_EDIT_SKIPSHORT && HIWORD( wParam ) == EN_CHANGE ||
				 LOWORD( wParam ) == IDC_EDIT_SKIPKINDS && HIWORD( wParam ) == EN_CHANGE ||
				 LOWORD( wParam ) == IDC_CHECK_SEND_STOP && HIWORD( wParam ) == BN_CLICKED ||
				 LOWORD( wParam ) == IDC_CHECK_CLEAR_PLAYLIST && HIWORD( wParam ) == BN_CLICKED )
			{
				PropSheet_Changed( GetParent( hwndDlg ), hwndDlg );
			}
			
			break;
		}

		case WM_NOTIFY:
		{
			LPPSHNOTIFY lppsn = (LPPSHNOTIFY) lParam; 

			switch ( lppsn->hdr.code )
			{
				case PSN_KILLACTIVE:
				{
					//
					// Validate: If validate fails, send true.
					//

					GetDlgItemInt( hwndDlg, IDC_EDIT_DELAY, &fTranslated, TRUE );
					if ( !fTranslated )
					{
						MessageBox( NULL, "Invalid \"Playlist Delay\" value.  Try again.", PRODUCT_NAME, MB_OK + MB_ICONERROR );
						SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );
					}
					else
					{
						SetWindowLong( hwndDlg, DWL_MSGRESULT, FALSE );
					}					
					
					
					GetDlgItemInt( hwndDlg, IDC_EDIT_SKIPSHORT, &fTranslated, TRUE );
					if ( !fTranslated )
					{
						MessageBox( NULL, "Invalid \"Skip Shorter Than\" value.  Try again.", PRODUCT_NAME, MB_OK + MB_ICONERROR );
						SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );
					}
					else
					{
						SetWindowLong( hwndDlg, DWL_MSGRESULT, FALSE );
					}

					return TRUE;

					break;
				}

				case PSN_APPLY:
				{
					//
					// Get the values
					//

					GetDlgItemText( hwndDlg, IDC_EDIT_SKIPKINDS, g_szSkipKinds, sizeof( g_szSkipKinds ) );

					g_intPlaylistBufferDelay = GetDlgItemInt( hwndDlg, IDC_EDIT_DELAY, &fTranslated, TRUE );
					g_intSkipShort = GetDlgItemInt( hwndDlg, IDC_EDIT_SKIPSHORT, &fTranslated, TRUE );
					g_intPublishStop = IsDlgButtonChecked( hwndDlg, IDC_CHECK_SEND_STOP ) == BST_CHECKED ? 1 : 0;
					g_intPlaylistClear = IsDlgButtonChecked( hwndDlg, IDC_CHECK_CLEAR_PLAYLIST ) == BST_CHECKED ? 1 : 0;

					long lResult = SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_LOGGING ), CB_GETCURSEL, 0, 0 );
					if ( lResult != CB_ERR )
					{
						TCHAR szLogging[MAX_PATH];
						SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_LOGGING ), CB_GETLBTEXT, lResult, (LPARAM) (LPTSTR) szLogging );

						if ( strcmp( szLogging, LOGGING_LABEL_ERROR ) == 0 )
						{
							g_AmLog.SetLogLevel( CAmLoglevelError );
						}
						else if ( strcmp( szLogging, LOGGING_LABEL_INFO ) == 0 )
						{
							g_AmLog.SetLogLevel( CAmLoglevelInfo );
						}
						else if ( strcmp( szLogging, LOGGING_LABEL_DEBUG ) == 0 )
						{
							g_AmLog.SetLogLevel( CAmLoglevelDebug );
						}
						else
						{
							g_AmLog.SetLogLevel( CAmLoglevelNothing );
						}
					}

					//
					// Store all the entries
					//

					if ( RegCreateKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition ) == ERROR_SUCCESS )
					{
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_GUID, 0, REG_SZ, (BYTE*) &g_szGuid[0], strlen( g_szGuid ) + 1 );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_PUBLISH_STOP, 0, REG_DWORD, (BYTE*) &g_intPublishStop, sizeof( g_intPublishStop ) );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_CLEAR_PLAYLIST, 0, REG_DWORD, (BYTE*) &g_intPlaylistClear, sizeof( g_intPlaylistClear ) );

						if ( fTranslated )
						{
							RegSetValueEx( hKey, MY_REGISTRY_VALUE_DELAY, 0, REG_DWORD, (BYTE*) &g_intPlaylistBufferDelay, sizeof( g_intPlaylistBufferDelay ) );
							RegSetValueEx( hKey, MY_REGISTRY_VALUE_SKIPSHORT, 0, REG_DWORD, (BYTE*) &g_intSkipShort, sizeof( g_intSkipShort ) );
							RegSetValueEx( hKey, MY_REGISTRY_VALUE_SKIPKINDS, 0, REG_SZ, (BYTE*) &g_szSkipKinds[0], strlen( g_szSkipKinds ) + 1 );

							int intLogLevel = g_AmLog.GetLogLevel();
							RegSetValueEx( hKey, MY_REGISTRY_VALUE_LOGGING, 0, REG_DWORD, (BYTE*) &intLogLevel, sizeof( intLogLevel ) );
						}
						
						RegCloseKey( hKey );
					}

					//
					// Success
					//

					SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );

					SetFocus( hwndDlg );

					PropSheet_UnChanged( GetParent( hwndDlg ), hwndDlg );

					break;
				}
			}
		}
    } 

    return FALSE; 
} 

BOOL CALLBACK OptionDlgXMLPageProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
	HKEY hKey;
	DWORD dwDisposition = 0;
	BOOL fTranslated = FALSE;
	int intPlaylistLength = DEFAULT_PLAYLIST_LENGTH;

	switch ( message ) 
	{ 
		case WM_INITDIALOG: 
		{
			//
			// Load up the values
			//

			CheckDlgButton( hwndDlg, IDC_CHECK_XML_CDATA, g_intUseXmlCData == 0 ? BST_UNCHECKED : BST_CHECKED );			

			SetDlgItemText( hwndDlg, IDC_EDIT_STYLESHEET, g_szStyleSheet );
			SetDlgItemText( hwndDlg, IDC_EDIT_FILE, g_szOutputFile );

			SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_XML_ENCODING ), CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) XML_ENCODING_LABEL_UTF_8 );
			SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_XML_ENCODING ), CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) XML_ENCODING_LABEL_ISO_8859_1 );
			SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_XML_ENCODING ), CB_SELECTSTRING, -1, (LPARAM) (LPCTSTR) g_szXMLEncoding );

			SetDlgItemInt(  hwndDlg, IDC_EDIT_PLAYLIST_LEN, g_intPlaylistLength, TRUE );
						
			return TRUE;
		}

		case WM_COMMAND:
		{
			//
			// Enable the Apply button if changes were made
			//

			if ( LOWORD( wParam ) == IDC_EDIT_PLAYLIST_LEN && HIWORD( wParam ) == EN_CHANGE ||
				 LOWORD( wParam ) == IDC_EDIT_FILE && HIWORD( wParam ) == EN_CHANGE ||
				 LOWORD( wParam ) == IDC_EDIT_STYLESHEET && HIWORD( wParam ) == EN_CHANGE ||				 
				 LOWORD( wParam ) == IDC_CHECK_XML_CDATA && HIWORD( wParam ) == BN_CLICKED ||
				 LOWORD( wParam ) == IDC_COMBO_XML_ENCODING && HIWORD( wParam ) == CBN_SELCHANGE )
			{
				PropSheet_Changed( GetParent( hwndDlg ), hwndDlg );
			}
			
			break;
		}

		case WM_NOTIFY:
		{
			LPPSHNOTIFY lppsn = (LPPSHNOTIFY) lParam; 

			switch ( lppsn->hdr.code )
			{
				case PSN_KILLACTIVE:
				{
					//
					// Validate: If validate fails, send true.
					//

					TCHAR szPath[MAX_PATH] = {0};

					if ( GetDlgItemText( hwndDlg, IDC_EDIT_FILE, szPath, sizeof( szPath ) ) )
					{
						if ( strlen( szPath ) == 0 )
						{
							SetFocus( GetDlgItem( hwndDlg, IDC_EDIT_FILE ) );
							MessageBox( NULL, "Invalid output file.  You must enter a file path to continue.", PRODUCT_NAME, MB_OK + MB_ICONERROR );
							SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );
							return TRUE;
						}

						if ( PathIsFileSpec( szPath ) )
						{
							SetFocus( GetDlgItem( hwndDlg, IDC_EDIT_FILE ) );
							MessageBox( NULL, "Invalid output file.  You must specify the complete file path.", PRODUCT_NAME, MB_OK + MB_ICONERROR );
							SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );
							return TRUE;
						}
					}

					GetDlgItemInt( hwndDlg, IDC_EDIT_PLAYLIST_LEN, &fTranslated, TRUE );
					if ( !fTranslated )
					{
						MessageBox( NULL, "Invalid playlist length value. Try again.", PRODUCT_NAME, MB_OK + MB_ICONERROR );
						SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );
					}
					else
					{
						SetWindowLong( hwndDlg, DWL_MSGRESULT, FALSE );
					}
					
					return TRUE;

					break;
				}

				case PSN_APPLY:
				{
					//
					// Get the values
					//

					GetDlgItemText( hwndDlg, IDC_EDIT_STYLESHEET, g_szStyleSheet, MAX_PATH );
					GetDlgItemText( hwndDlg, IDC_EDIT_FILE, g_szOutputFile, MAX_PATH );

					g_intUseXmlCData = IsDlgButtonChecked( hwndDlg, IDC_CHECK_XML_CDATA ) == BST_CHECKED ? 1 : 0;

					LONG lResult = SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_XML_ENCODING ), CB_GETCURSEL, 0, 0 );
					if ( lResult != CB_ERR )
					{
						SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_XML_ENCODING ), CB_GETLBTEXT, lResult, (LPARAM) (LPTSTR) g_szXMLEncoding );
					}

					intPlaylistLength = GetDlgItemInt( hwndDlg, IDC_EDIT_PLAYLIST_LEN, &fTranslated, TRUE );

					//
					// Store all the entries
					//

					if ( RegCreateKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition ) == ERROR_SUCCESS )
					{
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_GUID, 0, REG_SZ, (BYTE*) &g_szGuid[0], strlen( g_szGuid ) + 1 );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_FILE, 0, REG_SZ, (BYTE*) &g_szOutputFile[0], strlen( g_szOutputFile ) + 1 );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_STYLESHEET, 0, REG_SZ, (BYTE*) &g_szStyleSheet[0], strlen( g_szStyleSheet ) + 1 );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_XML_CDATA, 0, REG_DWORD, (BYTE*) &g_intUseXmlCData, sizeof( g_intUseXmlCData ) );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_XML_ENCODING, 0, REG_SZ, (BYTE*) &g_szXMLEncoding[0], strlen( g_szXMLEncoding ) + 1 );

						if ( fTranslated )
						{
							RegSetValueEx( hKey, MY_REGISTRY_VALUE_PLAYLIST_LENGTH, 0, REG_DWORD, (BYTE*) &intPlaylistLength, sizeof( intPlaylistLength ) );
						}
						
						RegCloseKey( hKey );
					}

					//
					// Success
					//

					SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );

					SetFocus( hwndDlg );

					PropSheet_UnChanged( GetParent( hwndDlg ), hwndDlg );

					break;
				}
			}
			 
			break;
		}
    } 

    return FALSE; 
} 

BOOL CALLBACK OptionDlgUploadPageProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
	HKEY hKey;
	DWORD dwDisposition = 0;
	BOOL fTranslated = FALSE;
	char szTemp[MAX_PATH] = {0};
	long lResult = 0;

	switch ( message ) 
	{ 
		case WM_INITDIALOG: 
		{
			//
			// Load up the values
			//

			SetDlgItemText( hwndDlg, IDC_EDIT_USER, g_szFTPUser );
			SetDlgItemText( hwndDlg, IDC_EDIT_PASSWORD, g_szFTPPassword );
			SetDlgItemText( hwndDlg, IDC_EDIT_HOST, g_szFTPHost );
			SetDlgItemText( hwndDlg, IDC_EDIT_PATH, g_szFTPPath );

			SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_UPLOAD_PROTOCOL ), CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) UPLOAD_PROTOCOL_LABEL_NONE );
			SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_UPLOAD_PROTOCOL ), CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) UPLOAD_PROTOCOL_LABEL_FTP );
			SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_UPLOAD_PROTOCOL ), CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) UPLOAD_PROTOCOL_LABEL_SFTP );
			SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_UPLOAD_PROTOCOL ), CB_SELECTSTRING, -1, (LPARAM) (LPCTSTR) g_szUploadProtocol );

			if ( g_intMediaPlayerType == MEDIA_PLAYER_ITUNES )
			{			
				CheckDlgButton( hwndDlg, IDC_CHECK_ARTWORK_UPLOAD, g_intArtworkUpload > 0 ? BST_CHECKED : BST_UNCHECKED );

				SetDlgItemInt( hwndDlg, IDC_EDIT_ARTWORK_WIDTH, g_intArtworkWidth, TRUE );
			}
			else
			{
				ShowWindow( GetDlgItem( hwndDlg, IDC_CHECK_ARTWORK_UPLOAD ), SW_HIDE );
				ShowWindow( GetDlgItem( hwndDlg, IDC_EDIT_ARTWORK_WIDTH ), SW_HIDE );
				ShowWindow( GetDlgItem( hwndDlg, IDC_STATIC_ART1 ), SW_HIDE );
				ShowWindow( GetDlgItem( hwndDlg, IDC_STATIC_ART2 ), SW_HIDE );
			}

			return TRUE;
		}

		case WM_COMMAND:
		{
			//
			// Enable the Apply button if changes were made
			//

			if ( LOWORD( wParam ) == IDC_COMBO_UPLOAD_PROTOCOL && HIWORD( wParam ) == CBN_SELCHANGE ||
				 LOWORD( wParam ) == IDC_EDIT_USER && HIWORD( wParam ) == EN_CHANGE ||
				 LOWORD( wParam ) == IDC_EDIT_PASSWORD && HIWORD( wParam ) == EN_CHANGE ||
				 LOWORD( wParam ) == IDC_EDIT_HOST && HIWORD( wParam ) == EN_CHANGE ||
				 LOWORD( wParam ) == IDC_EDIT_PATH && HIWORD( wParam ) == EN_CHANGE ||
				 LOWORD( wParam ) == IDC_CHECK_ARTWORK_UPLOAD && HIWORD( wParam ) == BN_CLICKED ||
				 LOWORD( wParam ) == IDC_EDIT_ARTWORK_WIDTH && HIWORD( wParam ) == EN_CHANGE )
			{
				PropSheet_Changed( GetParent( hwndDlg ), hwndDlg );
			}
			
			break;
		}

		case WM_NOTIFY:
		{
			LPPSHNOTIFY lppsn = (LPPSHNOTIFY) lParam; 

			switch ( lppsn->hdr.code )
			{
				case PSN_KILLACTIVE:
				{
					//
					// Validate: If validate fails, send true.
					//

					lResult = SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_UPLOAD_PROTOCOL ), CB_GETCURSEL, 0, 0 );
					if ( lResult != CB_ERR )
					{
						TCHAR szUploadProtocol[MAX_PATH] = {0};
						TCHAR szFTPHost[MAX_PATH] = {0};
						TCHAR szFTPUser[MAX_PATH] = {0};
						TCHAR szFTPPassword[MAX_PATH] = {0};
						TCHAR szFTPPath[MAX_PATH] = {0};

						SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_UPLOAD_PROTOCOL ), CB_GETLBTEXT, lResult, (LPARAM) (LPTSTR) szUploadProtocol );

						if ( strcmpi( szUploadProtocol, UPLOAD_PROTOCOL_LABEL_NONE ) != 0 )
						{
							GetDlgItemText( hwndDlg, IDC_EDIT_HOST, szFTPHost, sizeof( szFTPHost ) );
							if ( strlen( szFTPHost ) == 0 )
							{
								MessageBox( NULL, "You must enter a host to continue or turn the protocol off.", PRODUCT_NAME, MB_OK + MB_ICONERROR );
								SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );
								return TRUE;
							}

							GetDlgItemText( hwndDlg, IDC_EDIT_USER, szFTPUser, sizeof( szFTPUser ) );
							if ( strlen( szFTPUser ) == 0 )
							{
								MessageBox( NULL, "You must enter a user to continue or turn the protocol off.", PRODUCT_NAME, MB_OK + MB_ICONERROR );
								SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );
								return TRUE;
							}

							GetDlgItemText( hwndDlg, IDC_EDIT_PASSWORD, szFTPPassword, sizeof( szFTPPassword ) );
							if ( strlen( szFTPPassword ) == 0 )
							{
								MessageBox( NULL, "You must enter a password to continue or turn the protocol off.", PRODUCT_NAME, MB_OK + MB_ICONERROR );
								SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );
								return TRUE;
							}

							GetDlgItemText( hwndDlg, IDC_EDIT_PATH, szFTPPath, sizeof( szFTPPath ) );
							if ( strlen( szFTPPath ) == 0 )
							{
								MessageBox( NULL, "You must enter a path to continue or turn the protocol off.", PRODUCT_NAME, MB_OK + MB_ICONERROR );
								SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );
								return TRUE;
							}
						}
					}

					//
					// Success
					//
					
					SetWindowLong( hwndDlg, DWL_MSGRESULT, FALSE );

					return TRUE;

					break;
				}

				case PSN_APPLY:
				{
					//
					// Get the values
					//

					lResult = SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_UPLOAD_PROTOCOL ), CB_GETCURSEL, 0, 0 );
					if ( lResult != CB_ERR )
					{
						SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_UPLOAD_PROTOCOL ), CB_GETLBTEXT, lResult, (LPARAM) (LPTSTR) g_szUploadProtocol );
					}

					GetDlgItemText( hwndDlg, IDC_EDIT_HOST, g_szFTPHost, MAX_PATH );
					GetDlgItemText( hwndDlg, IDC_EDIT_USER, g_szFTPUser, MAX_PATH );
					GetDlgItemText( hwndDlg, IDC_EDIT_PASSWORD, g_szFTPPassword, MAX_PATH );
					GetDlgItemText( hwndDlg, IDC_EDIT_PATH, g_szFTPPath, MAX_PATH );	
					
					g_intArtworkUpload = IsDlgButtonChecked( hwndDlg, IDC_CHECK_ARTWORK_UPLOAD ) == BST_CHECKED ? 1 : 0;

					g_intArtworkWidth = GetDlgItemInt( hwndDlg, IDC_EDIT_ARTWORK_WIDTH, &fTranslated, TRUE );

					//
					// Store all the entries
					//

					if ( RegCreateKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition ) == ERROR_SUCCESS )
					{
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_GUID, 0, REG_SZ, (BYTE*) &g_szGuid[0], strlen( g_szGuid ) + 1 );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_PROTOCOL, 0, REG_SZ, (BYTE*) &g_szUploadProtocol[0], strlen( g_szUploadProtocol ) + 1 );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_HOST, 0, REG_SZ, (BYTE*) &g_szFTPHost[0], strlen( g_szFTPHost ) + 1 );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_USER, 0, REG_SZ, (BYTE*) &g_szFTPUser[0], strlen( g_szFTPUser ) + 1 );						
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_PATH, 0, REG_SZ, (BYTE*) &g_szFTPPath[0], strlen( g_szFTPPath ) + 1 );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_FTP_PASSIVE, 0, REG_DWORD, (BYTE*) &g_intFTPPassive, sizeof( g_intFTPPassive ) );
						
						if ( strlen( g_szFTPPassword ) > 0 )
						{
							EncryptString( g_szFTPPassword, szTemp, MAX_PATH );
						}
						else
						{
							memset( szTemp, 0, MAX_PATH );
						}

						RegSetValueEx( hKey, MY_REGISTRY_VALUE_PASSWORD, 0, REG_SZ, (BYTE*) &szTemp[0], strlen( szTemp ) + 1 );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_ARTWORK_UPLOAD, 0, REG_DWORD, (BYTE*) &g_intArtworkUpload, sizeof( g_intArtworkUpload ) );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_ARTWORK_WIDTH, 0, REG_DWORD, (BYTE*) &g_intArtworkWidth, sizeof( g_intArtworkWidth ) );

						RegCloseKey( hKey );
					}

					//
					// Success
					//

					SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );

					SetFocus( hwndDlg );

					PropSheet_UnChanged( GetParent( hwndDlg ), hwndDlg );

					break;
				}
			}
			 
			break;
		}
    } 

    return FALSE; 
} 

BOOL CALLBACK OptionDlgPingPageProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
	HKEY hKey;
	BOOL fTranslated = FALSE;
	DWORD dwDisposition = 0;
	char szTemp[MAX_PATH];
	int intPlaylistLength = DEFAULT_PLAYLIST_LENGTH;

	switch ( message ) 
	{ 
		case WM_INITDIALOG: 
		{
			//
			// Load up the values
			//

			SetDlgItemText( hwndDlg, IDC_EDIT_TRACKBACK_URL, g_szTrackBackUrl );
			SetDlgItemText( hwndDlg, IDC_EDIT_TRACKBACK_PASSPHRASE, g_szTrackBackPassphrase );
						
			return TRUE;
		}

		case WM_COMMAND:
		{
			//
			// Enable the Apply button if changes were made
			//
			
			if ( LOWORD( wParam ) == IDC_EDIT_TRACKBACK_URL && HIWORD( wParam ) == EN_CHANGE ||
				 LOWORD( wParam ) == IDC_EDIT_TRACKBACK_PASSPHRASE && HIWORD( wParam ) == EN_CHANGE )
			{
				PropSheet_Changed( GetParent( hwndDlg ), hwndDlg );
			}
			
			break;
		}

		case WM_NOTIFY:
		{
			LPPSHNOTIFY lppsn = (LPPSHNOTIFY) lParam; 

			switch ( lppsn->hdr.code )
			{
				case PSN_KILLACTIVE:
				{
					//
					// Validate: If validate fails, send true.
					//

					SetWindowLong( hwndDlg, DWL_MSGRESULT, FALSE );
					
					return TRUE;

					break;
				}

				case PSN_APPLY:
				{
					//
					// Get the values
					//

					GetDlgItemText( hwndDlg, IDC_EDIT_TRACKBACK_URL, g_szTrackBackUrl, MAX_PATH );
					GetDlgItemText( hwndDlg, IDC_EDIT_TRACKBACK_PASSPHRASE, g_szTrackBackPassphrase, MAX_PATH );
	
					//
					// Store all the entries
					//

					if ( RegCreateKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition ) == ERROR_SUCCESS )
					{
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_GUID, 0, REG_SZ, (BYTE*) &g_szGuid[0], strlen( g_szGuid ) + 1 );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_TRACKBACK_URL, 0, REG_SZ, (BYTE*) &g_szTrackBackUrl[0], strlen( g_szTrackBackUrl ) + 1 );						
				
						if ( strlen( g_szTrackBackPassphrase ) > 0 )
						{
							EncryptString( g_szTrackBackPassphrase, szTemp, MAX_PATH );
						}
						else
						{
							memset( szTemp, 0, MAX_PATH );
						}
						
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_TRACKBACK_PASSPHRASE, 0, REG_SZ, (BYTE*) &szTemp[0], strlen( szTemp ) + 1 );
						
						RegCloseKey( hKey );
					}

					//
					// Success
					//

					SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );

					SetFocus( hwndDlg );

					PropSheet_UnChanged( GetParent( hwndDlg ), hwndDlg );

					break;
				}
			}
			 
			break;
		}		
    } 

    return FALSE; 
} 

BOOL CALLBACK OptionDlgAmazonPageProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
	HKEY hKey;
	BOOL fTranslated = FALSE;
	DWORD dwDisposition = 0;
	TCHAR szBuffer[128];
	HMODULE hModule = NULL;

	switch ( message ) 
	{ 
		case WM_INITDIALOG: 
		{
			//
			// Setup the UI
			//

			hModule = GetModuleHandle( g_szThisDllPath );
			if ( !hModule )
			{
				hModule = _Module.GetResourceInstance();
			}

			if ( g_intMediaPlayerType == MEDIA_PLAYER_WMP )
			{
				LoadString( hModule, IDS_HINT_USE_SET, szBuffer, sizeof( szBuffer ) );
				SetWindowText( GetDlgItem( hwndDlg, IDC_CHECK_AMAZON_ASIN ), szBuffer );
			}
			else if ( g_intMediaPlayerType == MEDIA_PLAYER_YAHOO )
			{
				ShowWindow( GetDlgItem( hwndDlg, IDC_CHECK_AMAZON_ASIN ), SW_HIDE );
				ShowWindow( GetDlgItem( hwndDlg, IDC_STATIC_AMAZON_ASIN_HINT ), SW_HIDE );
			}

			//
			// Load up the values
			//

			SetDlgItemText( hwndDlg, IDC_EDIT_AMAZON_ASSOCIATE, g_szAmazonAssociate );

			SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_AMAZON_LOCALE ), CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) AMAZON_LOCALE_LABEL_CA );
			SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_AMAZON_LOCALE ), CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) AMAZON_LOCALE_LABEL_DE );
			SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_AMAZON_LOCALE ), CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) AMAZON_LOCALE_LABEL_FR );
			SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_AMAZON_LOCALE ), CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) AMAZON_LOCALE_LABEL_JP );
			SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_AMAZON_LOCALE ), CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) AMAZON_LOCALE_LABEL_UK );
			SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_AMAZON_LOCALE ), CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) AMAZON_LOCALE_LABEL_US );
			SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_AMAZON_LOCALE ), CB_SELECTSTRING, -1, (LPARAM) (LPCTSTR) g_szAmazonLocale );

			CheckDlgButton( hwndDlg, IDC_CHECK_AMAZON, g_intAmazonEnabled > 0 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton( hwndDlg, IDC_CHECK_AMAZON_ASIN, g_intAmazonUseASIN > 0 ? BST_CHECKED : BST_UNCHECKED );
			
			//
			// Setup enabled fields
			//

			EnableWindow( GetDlgItem( hwndDlg, IDC_EDIT_AMAZON_ASSOCIATE ), g_intAmazonEnabled ? TRUE : FALSE  );
			EnableWindow( GetDlgItem( hwndDlg, IDC_COMBO_AMAZON_LOCALE ), g_intAmazonEnabled ? TRUE : FALSE  );
			EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_AMAZON_ASIN ), g_intAmazonEnabled ? TRUE : FALSE  );
						
			return TRUE;
		}

		case WM_COMMAND:
		{
			//
			// Enable the Apply button if changes were made
			//

			if ( LOWORD( wParam ) == IDC_COMBO_AMAZON_LOCALE && HIWORD( wParam ) == CBN_SELCHANGE || 
				 LOWORD( wParam ) == IDC_EDIT_AMAZON_ASSOCIATE && HIWORD( wParam ) == EN_CHANGE || 
				 LOWORD( wParam ) == IDC_CHECK_AMAZON_ASIN && HIWORD( wParam ) == BN_CLICKED ||
				 LOWORD( wParam ) == IDC_CHECK_AMAZON && HIWORD( wParam ) == BN_CLICKED )
			{
				PropSheet_Changed( GetParent( hwndDlg ), hwndDlg );
			}

			//
			// Enable/disable properties based on enabled selection
			//

			if ( LOWORD( wParam ) == IDC_CHECK_AMAZON && HIWORD( wParam ) == BN_CLICKED )
			{
				BOOL fEnable = IsDlgButtonChecked( hwndDlg, IDC_CHECK_AMAZON ) == BST_CHECKED ? TRUE: FALSE;
				EnableWindow( GetDlgItem( hwndDlg, IDC_EDIT_AMAZON_ASSOCIATE ), fEnable );
				EnableWindow( GetDlgItem( hwndDlg, IDC_COMBO_AMAZON_LOCALE ), fEnable );
				EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_AMAZON_ASIN ), fEnable );
			}

			break;
		}

		case WM_NOTIFY:
		{
			LPPSHNOTIFY lppsn = (LPPSHNOTIFY) lParam; 

			switch ( lppsn->hdr.code )
			{
				case PSN_KILLACTIVE:
				{
					//
					// Validate: If validate fails, send true.
					//

					SetWindowLong( hwndDlg, DWL_MSGRESULT, FALSE );
				
					return TRUE;

					break;
				}

				case PSN_APPLY:
				{
					//
					// Get the values
					//

					LONG lResult = 0;

					GetDlgItemText( hwndDlg, IDC_EDIT_AMAZON_ASSOCIATE, g_szAmazonAssociate, MAX_PATH );

					lResult = SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_AMAZON_LOCALE ), CB_GETCURSEL, 0, 0 );
					if ( lResult != CB_ERR )
					{
						SendMessage( GetDlgItem( hwndDlg, IDC_COMBO_AMAZON_LOCALE ), CB_GETLBTEXT, lResult, (LPARAM) (LPTSTR) g_szAmazonLocale );
					}

					g_intAmazonEnabled = IsDlgButtonChecked( hwndDlg, IDC_CHECK_AMAZON ) == BST_CHECKED ? 1 : 0;
					g_intAmazonUseASIN = IsDlgButtonChecked( hwndDlg, IDC_CHECK_AMAZON_ASIN ) == BST_CHECKED ? 1 : 0;

					//
					// Store all the entries
					//

					if ( RegCreateKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition ) == ERROR_SUCCESS )
					{						
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_AMAZON_ASSOCIATE, 0, REG_SZ, (BYTE*) &g_szAmazonAssociate[0], strlen( g_szAmazonAssociate ) + 1 );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_AMAZON_LOCALE, 0, REG_SZ, (BYTE*) &g_szAmazonLocale[0], strlen( g_szAmazonLocale ) + 1 );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_AMAZON_USE_ASIN, 0, REG_DWORD, (BYTE*) &g_intAmazonUseASIN, sizeof( g_intAmazonUseASIN ) );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_AMAZON_ENABLED, 0, REG_DWORD, (BYTE*) &g_intAmazonEnabled, sizeof( g_intAmazonEnabled ) );

						RegCloseKey( hKey );
					}

					//
					// Success
					//

					SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );

					SetFocus( hwndDlg );

					PropSheet_UnChanged( GetParent( hwndDlg ), hwndDlg );

					break;
				}
			}
			 
			break;
		}
    } 

    return FALSE; 
} 

BOOL CALLBACK OptionDlgApplePageProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
	HKEY hKey;
	BOOL fTranslated = FALSE;
	DWORD dwDisposition = 0;

	switch ( message ) 
	{ 
		case WM_INITDIALOG: 
		{

			//
			// Load up the values
			//

			SetDlgItemText( hwndDlg, IDC_EDIT_APPLE_ASSOCIATE, g_szAppleAssociate );

			CheckDlgButton( hwndDlg, IDC_CHECK_APPLE, g_intAppleEnabled > 0 ? BST_CHECKED : BST_UNCHECKED );

			//
			// Setup enabled fields
			//

			EnableWindow( GetDlgItem( hwndDlg, IDC_EDIT_APPLE_ASSOCIATE ), g_intAppleEnabled ? TRUE : FALSE );
						
			return TRUE;
		}

		case WM_COMMAND:
		{
			//
			// Enable the Apply button if changes were made
			//

			if ( LOWORD( wParam ) == IDC_EDIT_APPLE_ASSOCIATE && HIWORD( wParam ) == EN_CHANGE || 
				 LOWORD( wParam ) == IDC_CHECK_APPLE && HIWORD( wParam ) == BN_CLICKED )
			{
				PropSheet_Changed( GetParent( hwndDlg ), hwndDlg );
			}

			//
			// Enable/disable properties based on enabled selection
			//

			if ( LOWORD( wParam ) == IDC_CHECK_APPLE && HIWORD( wParam ) == BN_CLICKED )
			{
				BOOL fEnable = IsDlgButtonChecked( hwndDlg, IDC_CHECK_APPLE ) == BST_CHECKED ? TRUE: FALSE;
				EnableWindow( GetDlgItem( hwndDlg, IDC_EDIT_APPLE_ASSOCIATE ), fEnable );
			}

			break;
		}

		case WM_NOTIFY:
		{
			LPPSHNOTIFY lppsn = (LPPSHNOTIFY) lParam; 

			switch ( lppsn->hdr.code )
			{
				case PSN_KILLACTIVE:
				{
					//
					// Validate: If validate fails, send true.
					//

					SetWindowLong( hwndDlg, DWL_MSGRESULT, FALSE );
				
					return TRUE;

					break;
				}

				case PSN_APPLY:
				{
					//
					// Get the values
					//

					LONG lResult = 0;

					GetDlgItemText( hwndDlg, IDC_EDIT_APPLE_ASSOCIATE, g_szAppleAssociate, MAX_PATH );

					g_intAppleEnabled = IsDlgButtonChecked( hwndDlg, IDC_CHECK_APPLE ) == BST_CHECKED ? 1 : 0;

					//
					// Store all the entries
					//

					if ( RegCreateKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition ) == ERROR_SUCCESS )
					{						
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_APPLE_ASSOCIATE, 0, REG_SZ, (BYTE*) &g_szAppleAssociate[0], strlen( g_szAppleAssociate ) + 1 );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_APPLE_ENABLED, 0, REG_DWORD, (BYTE*) &g_intAppleEnabled, sizeof( g_intAppleEnabled ) );

						RegCloseKey( hKey );
					}

					//
					// Success
					//

					SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );

					SetFocus( hwndDlg );

					PropSheet_UnChanged( GetParent( hwndDlg ), hwndDlg );

					break;
				}
			}
			 
			break;
		}
    } 

    return FALSE; 
} 

BOOL CALLBACK OptionDlgTwitterPageProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
	HKEY hKey;
	BOOL fTranslated = FALSE;
	DWORD dwDisposition = 0;
	char szTemp[MAX_PATH] = {NULL};

	switch ( message ) 
	{
		case WM_INITDIALOG: 
		{
			//
			// Load up the values
			//

			CheckDlgButton( hwndDlg, IDC_CHECK_TWITTER, g_intTwitterEnabled > 0 ? BST_CHECKED : BST_UNCHECKED );
			SetDlgItemInt( hwndDlg, IDC_EDIT_TWITTER_RATE_LIMIT, g_intTwitterRateLimitMinutes, TRUE );
			SetDialogPreference( hwndDlg, IDC_EDIT_TWITTER_MESSAGE, g_szTwitterMessage );

			DoTwitterUiState(hwndDlg);
						
			return TRUE;
		}

		case WM_COMMAND:
		{
			if ( LOWORD( wParam ) == IDC_TWITTER_AUTHORIZE && HIWORD( wParam ) == BN_CLICKED )
			{
				SetFocus( GetDlgItem( hwndDlg, IDC_TWITTER_PIN ) );

				DoTwitterAuthorize();
			}
			else if ( LOWORD( wParam ) == IDC_TWITTER_VERIFY && HIWORD( wParam ) == BN_CLICKED )
			{
				char szPin[MAX_PATH] = {0};

				GetDlgItemText( hwndDlg, IDC_TWITTER_PIN, szPin, sizeof(szPin) );

				if ( strlen( szPin ) > 0 )
				{
					DoTwitterVerify(szPin);

					SetDlgItemText( hwndDlg, IDC_TWITTER_PIN, "" );

					DoTwitterUiState(hwndDlg);
				}
				else
				{	
					SetFocus( GetDlgItem( hwndDlg, IDC_TWITTER_PIN ) );

					MessageBox( NULL, TWITTER_MESSAGE_PIN_REQUIRED, PRODUCT_NAME, MB_OK + MB_ICONERROR );
				}
			}
			else if ( LOWORD( wParam ) == IDC_TWITTER_RESET && HIWORD( wParam ) == BN_CLICKED )
			{
				DoTwitterReset();
		
				DoTwitterUiState(hwndDlg);
			}
			else if ( LOWORD( wParam ) == IDC_TWITTER_DEFAULT && HIWORD( wParam ) == BN_CLICKED )
			{
				SetDlgItemText( hwndDlg, IDC_EDIT_TWITTER_MESSAGE, TWITTER_MESSAGE_DEFAULT );

				PropSheet_Changed( GetParent( hwndDlg ), hwndDlg );
			}

			if ( LOWORD( wParam ) == IDC_CHECK_TWITTER && HIWORD( wParam ) == BN_CLICKED ||
				 LOWORD( wParam ) == IDC_EDIT_TWITTER_RATE_LIMIT && HIWORD( wParam ) == EN_CHANGE ||
				 LOWORD( wParam ) == IDC_EDIT_TWITTER_MESSAGE && HIWORD( wParam ) == EN_CHANGE )
			{
				PropSheet_Changed( GetParent( hwndDlg ), hwndDlg );
			}
			

			//
			// Enable/disable properties based on enabled selection
			//

			if ( LOWORD( wParam ) == IDC_CHECK_TWITTER && HIWORD( wParam ) == BN_CLICKED )
			{
				BOOL fEnable = IsDlgButtonChecked( hwndDlg, IDC_CHECK_TWITTER ) == BST_CHECKED ? TRUE: FALSE;
				EnableWindow( GetDlgItem( hwndDlg, IDC_EDIT_TWITTER_RATE_LIMIT ), fEnable );
				EnableWindow( GetDlgItem( hwndDlg, IDC_EDIT_TWITTER_MESSAGE ), fEnable );
			}
			
			break;
		}

		case WM_NOTIFY:
		{
			LPPSHNOTIFY lppsn = (LPPSHNOTIFY) lParam; 

			switch ( lppsn->hdr.code )
			{
				case PSN_KILLACTIVE:
				{
					//
					// Get the values
					//

					GetDlgItemText( hwndDlg, IDC_EDIT_TWITTER_MESSAGE, g_szTwitterMessage, MAX_PATH );

					GetDlgItemInt( hwndDlg, IDC_EDIT_TWITTER_RATE_LIMIT, &fTranslated, TRUE );
					if ( !fTranslated )
					{
						SetFocus( GetDlgItem( hwndDlg, IDC_EDIT_TWITTER_RATE_LIMIT ) );

						MessageBox( NULL, MESSAGE_INVALID_RATE_LIMIT, PRODUCT_NAME, MB_OK + MB_ICONERROR );
						SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );
					}
					else
					{
						SetWindowLong( hwndDlg, DWL_MSGRESULT, FALSE );
					}					
				
					return TRUE;

					break;
				}

				case PSN_APPLY:
				{
					//
					// Get the values
					//

					g_intTwitterEnabled = IsDlgButtonChecked( hwndDlg, IDC_CHECK_TWITTER ) == BST_CHECKED ? 1 : 0;
					g_intTwitterRateLimitMinutes = GetDlgItemInt( hwndDlg, IDC_EDIT_TWITTER_RATE_LIMIT, &fTranslated, TRUE );
					GetDialogPreference( hwndDlg, IDC_EDIT_TWITTER_MESSAGE, g_szTwitterMessage, sizeof( g_szTwitterMessage ) );
	
					//
					// Store all the entries
					//

					if ( RegCreateKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition ) == ERROR_SUCCESS )
					{
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_GUID, 0, REG_SZ, (BYTE*) &g_szGuid[0], strlen( g_szGuid ) + 1 );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_TWITTER_ENABLED, 0, REG_DWORD, (BYTE*) &g_intTwitterEnabled, sizeof( g_intTwitterEnabled ) );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_TWITTER_RATE_LIMIT_MINUTES, 0, REG_DWORD, (BYTE*) &g_intTwitterRateLimitMinutes, sizeof( g_intTwitterRateLimitMinutes ) );;
						SavePreference( hKey, MY_REGISTRY_VALUE_TWITTER_MESSAGE, g_szTwitterMessage );

						RegCloseKey( hKey );
					}

					//
					// Success
					//

					SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );

					SetFocus( hwndDlg );

					PropSheet_UnChanged( GetParent( hwndDlg ), hwndDlg );
					
					break;
				}
			}
			 
			break;
		}
    } 

    return FALSE; 
}

BOOL CALLBACK OptionDlgFacebookPageProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
	HKEY hKey;
	BOOL fTranslated = FALSE;
	DWORD dwDisposition = 0;
	char szTemp[MAX_PATH] = {NULL};
	LONG lResult = 0;

	switch ( message ) 
	{
		case WM_INITDIALOG: 
		{
			//
			// Load up the values
			//

			CheckDlgButton( hwndDlg, IDC_CHECK_FACEBOOK, g_intFacebookEnabled > 0 ? BST_CHECKED : BST_UNCHECKED );
			SetDlgItemInt( hwndDlg, IDC_FACEBOOK_RATE_LIMIT, g_intFacebookRateLimitMinutes, TRUE );
			SetDialogPreference( hwndDlg, IDC_EDIT_FACEBOOK_MESSAGE, g_szFacebookMessage );
			SetDialogPreference( hwndDlg, IDC_EDIT_FACEBOOK_DESCRIPTION, g_szFacebookAttachmentDescription );
			SetWindowText( GetDlgItem( hwndDlg, IDC_FACEBOOK_TAGS ), FACEBOOK_TAGS_SUPPORTED );

			DoFacebookUiState(hwndDlg);

			return TRUE;
		}

		case WM_COMMAND:
		{
			if ( LOWORD( wParam ) == IDC_FACEBOOK_ADD && HIWORD( wParam ) == BN_CLICKED )
			{
				DoFacebookAdd();
			}
			else if ( LOWORD( wParam ) == IDC_FACEBOOK_AUTHORIZE && HIWORD( wParam ) == BN_CLICKED )
			{
				DoFacebookAuthorize();

				DoFacebookUiState(hwndDlg);
			}
			else if ( LOWORD( wParam ) == IDC_FACEBOOK_RESET && HIWORD( wParam ) == BN_CLICKED )
			{
				DoFacebookReset();

				DoFacebookUiState(hwndDlg);
			}
			else if ( LOWORD( wParam ) == IDC_FACEBOOK_DEFAULT && HIWORD( wParam ) == BN_CLICKED )
			{
				SetDlgItemText( hwndDlg, IDC_EDIT_FACEBOOK_MESSAGE, FACEBOOK_MESSAGE_DEFAULT );
				PropSheet_Changed( GetParent( hwndDlg ), hwndDlg );
			}
			
			if ( LOWORD( wParam ) == IDC_CHECK_FACEBOOK && HIWORD( wParam ) == BN_CLICKED ||
					  LOWORD( wParam ) == IDC_FACEBOOK_RATE_LIMIT && HIWORD( wParam ) == EN_CHANGE ||
				      LOWORD( wParam ) == IDC_EDIT_FACEBOOK_MESSAGE && HIWORD( wParam ) == EN_CHANGE ||
					  LOWORD( wParam ) == IDC_EDIT_FACEBOOK_DESCRIPTION && HIWORD( wParam ) == EN_CHANGE )
			{
				DoFacebookUiState(hwndDlg);

				PropSheet_Changed( GetParent( hwndDlg ), hwndDlg );
			}			

			//
			// Enable/disable properties based on enabled selection
			//

			if ( LOWORD( wParam ) == IDC_CHECK_FACEBOOK && HIWORD( wParam ) == BN_CLICKED )
			{
				BOOL fEnable = IsDlgButtonChecked( hwndDlg, IDC_CHECK_FACEBOOK ) == BST_CHECKED ? TRUE: FALSE;
				EnableWindow( GetDlgItem( hwndDlg, IDC_EDIT_FACEBOOK_MESSAGE ), fEnable );
			}
			
			break;
		}

		case WM_NOTIFY:
		{
			LPPSHNOTIFY lppsn = (LPPSHNOTIFY) lParam; 

			switch ( lppsn->hdr.code )
			{
				case PSN_KILLACTIVE:
				{
					//
					// Get the values
					//

					GetDlgItemText( hwndDlg, IDC_EDIT_FACEBOOK_MESSAGE, g_szFacebookMessage, sizeof( g_szFacebookMessage ) );
					GetDlgItemText( hwndDlg, IDC_EDIT_FACEBOOK_DESCRIPTION, g_szFacebookAttachmentDescription, sizeof( g_szFacebookAttachmentDescription ) );

					int value = GetDlgItemInt( hwndDlg, IDC_FACEBOOK_RATE_LIMIT, &fTranslated, TRUE );
					if ( !fTranslated )
					{
						SetFocus( GetDlgItem( hwndDlg, IDC_FACEBOOK_RATE_LIMIT ) );

						MessageBox( NULL, MESSAGE_INVALID_RATE_LIMIT, PRODUCT_NAME, MB_OK + MB_ICONERROR );
						SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );
					}
					else if (value < FACEBOOK_RATE_LIMIT_MIN)
					{					
						MessageBox( NULL, MESSAGE_INVALID_RATE_LIMIT_FACEBOOK, PRODUCT_NAME, MB_OK + MB_ICONERROR );
						SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );
					}
					else
					{
						SetWindowLong( hwndDlg, DWL_MSGRESULT, FALSE );
					}
					
					return TRUE;

					break;
				}

				case PSN_APPLY:
				{
					//
					// Get the values
					//

					g_intFacebookEnabled = IsDlgButtonChecked( hwndDlg, IDC_CHECK_FACEBOOK ) == BST_CHECKED ? 1 : 0;
					g_intFacebookRateLimitMinutes = GetDlgItemInt( hwndDlg, IDC_FACEBOOK_RATE_LIMIT, &fTranslated, TRUE );
					GetDialogPreference( hwndDlg, IDC_EDIT_FACEBOOK_MESSAGE, g_szFacebookMessage, sizeof( g_szFacebookMessage ) );
					GetDialogPreference( hwndDlg, IDC_EDIT_FACEBOOK_DESCRIPTION, g_szFacebookAttachmentDescription, sizeof( g_szFacebookAttachmentDescription ) );
					
					//
					// Store all the entries
					//

					if ( RegCreateKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition ) == ERROR_SUCCESS )
					{
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_GUID, 0, REG_SZ, (BYTE*) &g_szGuid[0], strlen( g_szGuid ) + 1 );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_FACEBOOK_ENABLED, 0, REG_DWORD, (BYTE*) &g_intFacebookEnabled, sizeof( g_intFacebookEnabled ) );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_FACEBOOK_RATE_LIMIT_MINUTES, 0, REG_DWORD, (BYTE*) &g_intFacebookRateLimitMinutes, sizeof( g_intFacebookRateLimitMinutes ) );
						SavePreference( hKey, MY_REGISTRY_VALUE_FACEBOOK_MESSAGE, g_szFacebookMessage );
						SavePreference( hKey, MY_REGISTRY_VALUE_FACEBOOK_ATTACHMENT_DESCRIPTION, g_szFacebookAttachmentDescription );
						
						RegCloseKey( hKey );
					}

					//
					// Success
					//

					SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );

					SetFocus( hwndDlg );

					PropSheet_UnChanged( GetParent( hwndDlg ), hwndDlg );
					
					break;
				}
			}
			 
			break;
		}
    } 

    return FALSE; 
}

BOOL CALLBACK OptionDlgLicensePageProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
	HKEY hKey;
	BOOL fTranslated = FALSE;
	DWORD dwDisposition = 0;

	switch ( message ) 
	{
		case WM_INITDIALOG: 
		{
			//
			// Load up the values
			//

			SetDlgItemText( hwndDlg, IDC_EDIT_EMAIL, g_szEmail );
			SetDlgItemText( hwndDlg, IDC_EDIT_LICENSE_KEY, g_szLicenseKey );
						
			return TRUE;
		}

		case WM_COMMAND:
		{
			if ( LOWORD( wParam ) == IDC_EDIT_EMAIL && HIWORD( wParam ) == EN_CHANGE ||
				 LOWORD( wParam ) == IDC_EDIT_LICENSE_KEY && HIWORD( wParam ) == EN_CHANGE )
			{
				PropSheet_Changed( GetParent( hwndDlg ), hwndDlg );
			}
			
			break;
		}

		case WM_NOTIFY:
		{
			LPPSHNOTIFY lppsn = (LPPSHNOTIFY) lParam; 

			switch ( lppsn->hdr.code )
			{
				case PSN_KILLACTIVE:
				{
					//
					// Get the values
					//

					GetDlgItemText( hwndDlg, IDC_EDIT_EMAIL, g_szEmail, MAX_PATH );
					GetDlgItemText( hwndDlg, IDC_EDIT_LICENSE_KEY, g_szLicenseKey, MAX_PATH );

					//
					// Validate: If validate fails, send true.
					//
					
					if ( strlen( g_szEmail ) > 0 && strlen( g_szLicenseKey ) > 0 )
					{
						if ( !IsLicensed( MY_SECRET_KEY, g_szEmail, MY_REGISTRY_KEY, g_szLicenseKey ) )
						{
							MessageBox( NULL, "Invalid license key.  Try again.", PRODUCT_NAME, MB_OK + MB_ICONERROR );
							SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );
						}
						else
						{
							SetWindowLong( hwndDlg, DWL_MSGRESULT, FALSE );
						}
					}
					else
					{
						SetWindowLong( hwndDlg, DWL_MSGRESULT, FALSE );
					}										
					
					return TRUE;

					break;
				}

				case PSN_APPLY:
				{
					//
					// Get the values
					//

					GetDlgItemText( hwndDlg, IDC_EDIT_EMAIL, g_szEmail, MAX_PATH );
					GetDlgItemText( hwndDlg, IDC_EDIT_LICENSE_KEY, g_szLicenseKey, MAX_PATH );

					//
					// Set the licensed value
					//

					g_fLicensed = IsLicensed( MY_SECRET_KEY, g_szEmail, MY_REGISTRY_KEY, g_szLicenseKey );
	
					//
					// Store all the entries
					//

					if ( RegCreateKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition ) == ERROR_SUCCESS )
					{
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_GUID, 0, REG_SZ, (BYTE*) &g_szGuid[0], strlen( g_szGuid ) + 1 );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_EMAIL, 0, REG_SZ, (BYTE*) &g_szEmail[0], strlen( g_szEmail ) + 1 );
						RegSetValueEx( hKey, MY_REGISTRY_VALUE_LICENSE_KEY, 0, REG_SZ, (BYTE*) &g_szLicenseKey[0], strlen( g_szLicenseKey ) + 1 );
						
						RegCloseKey( hKey );	
					}

					//
					// Success
					//

					SetWindowLong( hwndDlg, DWL_MSGRESULT, TRUE );

					SetFocus( hwndDlg );

					PropSheet_UnChanged( GetParent( hwndDlg ), hwndDlg );
					
					break;
				}
			}
			 
			break;
		}
    } 

    return FALSE; 
}

DWORD WINAPI DoPropertySheetProc(LPVOID lpParam)
{
	HMODULE hModule = NULL;
	PROPSHEETPAGE psp[10];
	PROPSHEETHEADER psh;
	int i = 0;

	hModule = GetModuleHandle( g_szThisDllPath );
	if ( !hModule )
	{
		hModule = _Module.GetResourceInstance();
	}

	psp[i].dwSize = sizeof(PROPSHEETPAGE);
	psp[i].dwFlags = PSP_DEFAULT;
	psp[i].hInstance = hModule;
	psp[i].pszTemplate = MAKEINTRESOURCE( IDD_ABOUT );
	psp[i].pfnDlgProc = OptionDlgAboutPageProc;

	i++;

	psp[i].dwSize = sizeof(PROPSHEETPAGE);
	psp[i].dwFlags = PSP_DEFAULT;
	psp[i].hInstance = hModule;
	psp[i].pszTemplate = MAKEINTRESOURCE( IDD_OPTIONS );
	psp[i].pfnDlgProc = OptionDlgOptionsPageProc;

	i++;

	psp[i].dwSize = sizeof(PROPSHEETPAGE);
	psp[i].dwFlags = PSP_DEFAULT;
	psp[i].hInstance = hModule;
	psp[i].pszTemplate =  MAKEINTRESOURCE( IDD_XML );
	psp[i].pfnDlgProc = OptionDlgXMLPageProc;

	i++;

	psp[i].dwSize = sizeof(PROPSHEETPAGE);
	psp[i].dwFlags = PSP_DEFAULT;
	psp[i].hInstance = hModule;
	psp[i].pszTemplate =  MAKEINTRESOURCE( IDD_UPLOAD );
	psp[i].pfnDlgProc = OptionDlgUploadPageProc;

	i++;
		
	psp[i].dwSize = sizeof(PROPSHEETPAGE);
	psp[i].dwFlags = PSP_DEFAULT;
	psp[i].hInstance = hModule;
	psp[i].pszTemplate =  MAKEINTRESOURCE( IDD_PING );
	psp[i].pfnDlgProc = OptionDlgPingPageProc;

	i++;
		
	psp[i].dwSize = sizeof(PROPSHEETPAGE);
	psp[i].dwFlags = PSP_DEFAULT;
	psp[i].hInstance = hModule;
	psp[i].pszTemplate =  MAKEINTRESOURCE( IDD_AMAZON );
	psp[i].pfnDlgProc = OptionDlgAmazonPageProc;

	i++;

	psp[i].dwSize = sizeof(PROPSHEETPAGE);
	psp[i].dwFlags = PSP_DEFAULT;
	psp[i].hInstance = hModule;
	psp[i].pszTemplate =  MAKEINTRESOURCE( IDD_APPLE );
	psp[i].pfnDlgProc = OptionDlgApplePageProc;

	i++;

	psp[i].dwSize = sizeof(PROPSHEETPAGE);
	psp[i].dwFlags = PSP_DEFAULT;
	psp[i].hInstance = hModule;
	psp[i].pszTemplate =  MAKEINTRESOURCE( IDD_TWITTER );
	psp[i].pfnDlgProc = OptionDlgTwitterPageProc;

	i++;

	psp[i].dwSize = sizeof(PROPSHEETPAGE);
	psp[i].dwFlags = PSP_DEFAULT;
	psp[i].hInstance = hModule;
	psp[i].pszTemplate =  MAKEINTRESOURCE( IDD_FACEBOOK );
	psp[i].pfnDlgProc = OptionDlgFacebookPageProc;

	i++;

	psp[i].dwSize = sizeof(PROPSHEETPAGE);
	psp[i].dwFlags = PSP_DEFAULT;
	psp[i].hInstance = hModule;
	psp[i].pszTemplate =  MAKEINTRESOURCE( IDD_LICENSE );
	psp[i].pfnDlgProc = OptionDlgLicensePageProc;

	i++;

	//
	// Create
	//

	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_DEFAULT | PSH_PROPSHEETPAGE | PSH_USECALLBACK;
	psh.hwndParent = g_hwndParent;
	psh.hInstance = hModule;
	psh.pszCaption = (LPSTR) PRODUCT_NAME;
	psh.nPages = i;
	psh.nStartPage = 0;
	psh.ppsp = (LPCPROPSHEETPAGE) &psp;
	psh.pfnCallback = RemoveContextHelpProc;
    
	PropertySheet( &psh );
    
	return 0;
}

void DoPropertySheet(HWND hwndParent)
{
	DWORD dwThrdParam = 0;
	DWORD dwThreadId = 0;

	if ( hwndParent )
	{
		g_hwndParent = hwndParent;
	}
	else
	{
		g_hwndParent = ::GetForegroundWindow();
	}

	HANDLE hThread = CreateThread( NULL, 0, DoPropertySheetProc, &dwThrdParam, 0, &dwThreadId );
	if ( hThread )
	{
		CloseHandle( hThread );
	}
}

bool ReadResponseBody(HINTERNET hRequest, char * lpszResponse, size_t cbResponse)
{
	ZeroMemory( lpszResponse, cbResponse );
	bool fObtainedResponse = false;
	unsigned long numberOfBytesToRead = cbResponse - 1;
	unsigned long numberOfBytesRead = 0;
	unsigned long offset = 0;
	BOOL rc = FALSE;

	//----------------------------------------------------------------
	// We read data until we receive zero bytes, to indicate end of
	// the data.
	//
	// Read the response into a very large buffer.  Ignore the
	// remainder of the response if it is larger than this buffer.
	//
	// The InternetReadFile function takes four parameters:
	// 1 - the request handle returned from InternetOpenUrl(),
	// 2 - where to copy the the request response,
	// 3 - the number of bytes to read in that call,
	// 4 - the number of bytes read in that call.  This will be
	//     less than numberOfBytesToRead if there is a null
	//     terminator.
	//----------------------------------------------------------------
	while ( TRUE )
	{
		rc = InternetReadFile( hRequest, lpszResponse + offset, numberOfBytesToRead, &numberOfBytesRead );

		//------------------------------------------------------------
		// As long as the return code is TRUE then we are getting
		// good data
		//------------------------------------------------------------
		if ( rc == TRUE )
		{
			if ( numberOfBytesRead > 0 )
			{
				fObtainedResponse = true;
			}
			else
			{
				AMLOGDEBUG( _T("End of data after %d bytes"), offset );
				break;
			}

			offset += numberOfBytesRead;
			numberOfBytesToRead -= numberOfBytesRead;

			AMLOGDEBUG( _T("Read %d, left %d"), numberOfBytesRead, numberOfBytesToRead );
		}
		else
		{
			//--------------------------------------------------------
			// InternetReadFile call failed.  Use GetLastError()
			// to obtain more detailed information.
			//--------------------------------------------------------
			AMLOGERROR( _T("The call to InternetReadFile failed, with error (%d). %d bytes read so far and %d bytes left to read"), GetLastError(), numberOfBytesRead, numberOfBytesToRead );

			break;
		}
	}

	AMLOGDEBUG( _T("Response: %s"), lpszResponse );

	return fObtainedResponse;
}

void DoSoftwareUpdateCheck()
{
	HKEY hKey;
	DWORD dwDisposition = 0;
	HINTERNET hSession;
	HINTERNET hConnection;
	HINTERNET hData;
	DWORD dwContext = 0;
	LPTSTR lpszData = NULL;
	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;
	LPTSTR lpszVersionStart = NULL;
	LPTSTR lpszVersionStop = NULL;
	char szVersionCurrent[MAX_PATH];
	time_t timeNow = 0;

	AMLOGDEBUG( "Begin DoSoftwareUpdateCheck" );

	time( &timeNow );

	if ( timeNow > g_timeLastUpdateCheck + SOFTWARE_UPDATE_CHECK_INTERVAL_DAYS * 60 * 60 * 24 )
	{
		//
		// Update the timestamp
		//

		g_timeLastUpdateCheck = timeNow;

		if ( RegCreateKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition ) == ERROR_SUCCESS )
		{						
			RegSetValueEx( hKey, MY_REGISTRY_VALUE_UPDATE_TIMESTAMP, 0, REG_DWORD, (BYTE*) &g_timeLastUpdateCheck, sizeof( g_timeLastUpdateCheck ) );
			
			RegCloseKey( hKey );
		}

		hSession = InternetOpen( g_szUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );

		if ( hSession )
		{
			hConnection = InternetConnect( hSession, UPDATE_HOSTNAME, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, dwContext );

			if ( hConnection )
			{
				hData = HttpOpenRequest( hConnection, NULL, UPDATE_URL, NULL, NULL, NULL, INTERNET_FLAG_RELOAD, 0 );

				if ( hData )
				{
					if ( HttpSendRequest( hData, NULL, 0, NULL, 0) )
					{
						char szResponse[10 * 1024] = {0};

						if ( ReadResponseBody( hData, &szResponse[0], sizeof( szResponse ) ) )
						{
							lpszVersionStart = strstr( &szResponse[0], UPDATE_VERSION_TAG_BEGIN );

							if ( lpszVersionStart )
							{
								lpszVersionStart += strlen( UPDATE_VERSION_TAG_BEGIN );

								lpszVersionStop = strstr( lpszVersionStart, UPDATE_VERSION_TAG_END );

								if ( lpszVersionStop )
								{
									*lpszVersionStop = '\0';

									AMLOGDEBUG( lpszVersionStart );

									GetMyVersion( szVersionCurrent, sizeof( szVersionCurrent ) );

									AMLOGDEBUG( szVersionCurrent );

									if ( strcmp( lpszVersionStart, szVersionCurrent ) > 0 )
									{
										if ( MessageBox( NULL, "A software update is available for the Now Playing plug-in!\n\nWould you like to download it now?", PRODUCT_NAME, MB_YESNO + MB_ICONQUESTION ) == IDYES )
										{													
											ShellExecute( NULL, "open", URL_DOWNLOAD_UPDATE, NULL, NULL, SW_SHOW ); 

											Sleep( 5000 );
										}
									}
								}
							}
						}
					}
					else
					{
						AMLOGERROR( "HTTP Ping Send Failed" );
					}

					InternetCloseHandle( hData );
				}
				else
				{
					AMLOGERROR( "HTTP Ping Failed" );
				}

				InternetCloseHandle( hConnection );
			}

			InternetCloseHandle( hSession );
		}
	}

	AMLOGDEBUG( "End DoSoftwareUpdateCheck" );
}

void DoPlaylistLoad()
{
	HKEY hKey;
	DWORD dwBufLen = 0;
	
	if ( RegOpenKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey ) == ERROR_SUCCESS )
	{
		//
		// Get the needed buffer size
		//

		if ( RegQueryValueEx( hKey, MY_REGISTRY_VALUE_PLAYLIST_CACHE, NULL, NULL, NULL, &dwBufLen ) == ERROR_SUCCESS )
		{
			if ( dwBufLen > 0 && dwBufLen == sizeof( CPlaylistItem ) * g_intPlaylistLength )
			{
				LPBYTE pBuffer = (LPBYTE) malloc( dwBufLen );

				if ( pBuffer )
				{
					if ( RegQueryValueEx( hKey, MY_REGISTRY_VALUE_PLAYLIST_CACHE, NULL, NULL, pBuffer, &dwBufLen ) == ERROR_SUCCESS )
					{						
						//
						// Delete it once we read it to prevent any repeat crashes
						//

						RegDeleteValue( hKey, MY_REGISTRY_VALUE_PLAYLIST_CACHE );

						//
						// Copy it into our structure
						//

						memcpy( g_paPlaylist, pBuffer, dwBufLen );

						AMLOGDEBUG( "Loaded cached playlist" );

						dwBufLen = sizeof( int );
						
						if ( RegQueryValueEx( hKey, MY_REGISTRY_VALUE_PLAYLIST_CACHE_INDEX, NULL, NULL, (LPBYTE) &g_intNext, &dwBufLen ) == ERROR_SUCCESS )
						{
							if ( g_intNext > g_intPlaylistLength )
							{
								g_intNext = 0;
							}
						}
					}

					free( pBuffer );
				}
			}
			else
			{
				AMLOGDEBUG( "Skipping cache load because size is different" );
			}
		}

		RegCloseKey( hKey );
	}
}

void DoPlay(CPlaylistItem* trackInfo)
{
	if ( g_paPlaylist && trackInfo ) 
	{
		//
		// Signal that we played at least one song
		//

		g_fPlayedOnce = TRUE;

		if ( !g_fLicensed )
		{
			if ( g_intOnce > TRIAL_LIMIT )
			{		
				return;
			}
			else if ( g_intOnce == TRIAL_LIMIT )
			{
				TCHAR szMsg[1024];
				StringCbPrintf( szMsg, sizeof( szMsg ), "TRIAL VERSION\n\nPurchase the licensed version today! This trial version will only work for the first five songs you play each session.\n\n%s", URL_HOME );
				MessageBox( NULL, szMsg , PRODUCT_NAME, MB_OK + MB_ICONINFORMATION );
			}
		}

		//
		// If its a duplicate of the last one played, then skip it!
		//

		if ( g_paPlaylist[ g_intNext ] == trackInfo )
		{
			AMLOGINFO( "Duplicate track; skipping" );

			return;
		}

		//
		// Skip any short songs if configured
		//

		if ( g_intSkipShort > 0 && trackInfo->m_trackInfo.totalTimeInMS > 0 && (unsigned long) g_intSkipShort * 1000 > trackInfo->m_trackInfo.totalTimeInMS )
		{
			AMLOGINFO( "Duration %d ms; skipping", trackInfo->m_trackInfo.totalTimeInMS );

			return;
		}

		//
		// Skip any songs that match kinds
		//

		if (strlen(g_szSkipKinds) > 0)
		{
			bool skip = false;
		
			char szKind[MAX_PATH] = {0};
			WideCharToMultiByte( CP_UTF8, 0, &trackInfo->m_trackInfo.kind[1], -1, &szKind[0], sizeof( szKind ), NULL, NULL );
	
			char * lpszTokens = strdup(g_szSkipKinds);
			if (lpszTokens)
			{
				char * lpszToken = strtok(lpszTokens, ",");
				while (lpszToken != NULL && !skip)
				{					
					if (strcmp(lpszToken, szKind) == 0)
					{
						AMLOGINFO("Kind matched \"%s\"; skipping\n", szKind);

						skip = true;
					}
					else
					{
						lpszToken = strtok(NULL, ",");
					}
				}
				
				free(lpszTokens);
			}			
		
			if (skip)
			{
				return;
			}
		}

		//
		// Copy to the playlist
		//

		if ( g_intPlaylistLength > 1 )
		{
			//
			// If the delay has not passed, then update the previous entry again
			//

			if ( time( NULL ) >= g_timetChanged + g_intPlaylistBufferDelay )
			{
				g_intNext++;
			
				if ( g_intNext == g_intPlaylistLength )
				{
					g_intNext = 0;
				}			
			}

			//
			// Update the timer so we can see how long this updated entry stays
			//

			time( &g_timetChanged );
		}

		//
		// Copy to the playlist
		//

		g_paPlaylist[ g_intNext ] = trackInfo;
		
		//
		// Tell the worker thread
		//

		SetEvent( g_hSignalPlayEvent );

		//
		// Count for the trial
		//			

		if ( !g_fLicensed )
		{
			g_intOnce++;
		}
	}
}

void DoCleanup()
{
	DWORD dwExitCode = 0;
	int nTries = 0;

	AMLOGDEBUG( "Begin Cleanup" );

	//
	// Tell the workers that we are done
	//

	if ( g_hStopEvent )
	{
		SetEvent( g_hStopEvent );

		while ( GetExitCodeThread( g_hThreadPublish, &dwExitCode ) )
		{
			if ( dwExitCode == STILL_ACTIVE )
			{
				AMLOGDEBUG( "Waiting for PublishThreadFunc to exit" );

				Sleep( 500 );
			
			}
			else
			{
				break;
			}
		}

		nTries = 0;

		CloseHandle( g_hStopEvent );
	}

	//
	// Cleanup
	//

	FreeVisualImage();

	if ( g_paPlaylist != NULL )
	{
		delete [] g_paPlaylist;
	}

	if ( g_hThreadPublish ) 
	{
		CloseHandle( g_hThreadPublish );
		g_hThreadPublish = NULL;
	}

	AMLOGDEBUG( "End Cleanup" );
}

DWORD WINAPI PublishThreadFunc(LPVOID lpParam) 
{
	DWORD dwSignal = WAIT_TIMEOUT;
	HANDLE ahHandles[2];

	AMLOGDEBUG( "Begin PublishThreadFunc" );

	HRESULT hr = CoInitialize( NULL );
	if ( SUCCEEDED( hr ) )
	{
		ahHandles[0] = g_hStopEvent;
		ahHandles[1] = g_hSignalPlayEvent;

		while ( dwSignal != WAIT_FAILED && dwSignal != WAIT_OBJECT_0 )
		{
			dwSignal = WaitForMultipleObjects( 2, ahHandles, FALSE, INFINITE );

			if ( dwSignal == WAIT_OBJECT_0 + 1 )
			{
				//
				// New song was posted
				//

				DoPlayWorker( g_intNext );
			}
		}

		//
		// Store out all the songs in the playlist
		//

		if ( g_intPlaylistClear )
		{
			for ( int i = 0; i < g_intPlaylistLength; i++ )
			{
				g_paPlaylist[ i ].m_fSet = false;
			}
		}
		else
		{
			PlaylistStore();
		}

		//
		// If we ever played at least one song, then tell everyone we stopped.
		//

		if ( g_fPlayedOnce )
		{
			DoStop();
		}

		CoUninitialize();
	}

	AMLOGDEBUG( "Exit PublishThreadFunc" );

    return 0; 
}

bool IsFirstTime()
{
	bool value = true;
	char szValue[MAX_PATH] = {0};

	if (LoadMyPreferenceInt(MY_REGISTRY_VALUE_INIT) == 1)
	{
		value = false;
	}
	else if (LoadMyPreferenceString(MY_REGISTRY_VALUE_FILE, szValue, sizeof(szValue)) > 0)
	{
		value = false;

		SavePreference(MY_REGISTRY_VALUE_INIT, 1);
	}
	else
	{
		SavePreference(MY_REGISTRY_VALUE_INIT, 1);
	}

	return value;
}

bool DoInit(int intMediaPlayerType, LPCTSTR lpszPath)
{
	HKEY hKey;
	HRESULT hr = S_OK;
   	DWORD dwBufLen = 0;
   	DWORD dwThreadId = 0;
   	DWORD dwThrdParam = 1;
   	DWORD dwExitCode = 0;
  	TCHAR szTemp[MAX_PATH];

	//
	// Setup the player info
	//

	if ( !FileExists( lpszPath ) )
	{
		return false;
	}
	else
	{		
		hr = StringCbCopy( g_szThisDllPath, sizeof( g_szThisDllPath ), lpszPath );

		g_intMediaPlayerType = intMediaPlayerType;
	}

	//
	// Setup working directory
	//

	StringCbCopy( g_szAppWorkingDir, sizeof( g_szAppWorkingDir ), g_szThisDllPath );
	PathRemoveFileSpec( g_szAppWorkingDir );
	if ( g_intMediaPlayerType == MEDIA_PLAYER_ITUNES || g_intMediaPlayerType == MEDIA_PLAYER_WINAMP )
	{
		PathAddBackslash( g_szAppWorkingDir );
		StringCbCat( g_szAppWorkingDir, sizeof( g_szAppWorkingDir ), "Now Playing\\" );
	}

	//
	// Setup log file location
	//

	TCHAR szLogFile[MAX_PATH] = {0};
	hr = StringCbCopy( szLogFile, sizeof( szLogFile ), g_szAppWorkingDir );
	PathAddBackslash( szLogFile );
	hr = StringCbCat( szLogFile, sizeof( szLogFile ), "NowPlaying-Log.txt" );

	//
	// Setup log parameters
	//

	AMLOG_SETFILENAME( szLogFile );
#ifdef _DEBUG
	AMLOG_SETLOGLEVEL_DEBUG;
#endif

	AMLOGDEBUG( "Begin DoInit" );

	//
	// Setup user agent
	//

	hr = StringCbCopy( g_szUserAgent, sizeof( g_szUserAgent ), "NowPlaying/" );
	GetMyVersion( g_szVersion, sizeof( g_szVersion ) );
	hr = StringCbCat( g_szUserAgent, sizeof( g_szUserAgent ), g_szVersion );

	//
	// Fire up the update thread
	//

	g_hSignalPlayEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	g_hStopEvent = CreateEvent( NULL, TRUE, FALSE, NULL );


	g_hThreadPublish = CreateThread( NULL, 0, PublishThreadFunc, &dwThrdParam, 0, &dwThreadId );
	if ( g_hThreadPublish == NULL ) 
	{
		return false;
	}

	//
	// Init the settings
	//
	
	ZeroMemory( g_szGuid, sizeof( g_szGuid ) );
	ZeroMemory( g_szOutputFile, sizeof( g_szOutputFile ) );
	ZeroMemory( g_szLicenseKey, sizeof( g_szLicenseKey ) );
	ZeroMemory( g_szEmail, sizeof( g_szEmail ) );
	ZeroMemory( g_szUploadProtocol, sizeof( g_szUploadProtocol ) ); 
	ZeroMemory( g_szFTPHost, sizeof( g_szFTPHost ) ); 
	ZeroMemory( g_szFTPUser, sizeof( g_szFTPUser ) ); 
	ZeroMemory( g_szFTPPassword, sizeof( g_szFTPPassword ) ); 
	ZeroMemory( g_szSkipKinds, sizeof ( g_szSkipKinds ) );
	ZeroMemory( g_szFTPPath, sizeof( g_szFTPPath ) ); 
	ZeroMemory( g_szStyleSheet, sizeof( g_szStyleSheet ) ); 
	ZeroMemory( g_szTrackBackUrl, sizeof( g_szTrackBackUrl ) );
	ZeroMemory( g_szTrackBackPassphrase, sizeof( g_szTrackBackPassphrase ) );
	ZeroMemory( g_szAmazonAssociate, sizeof( g_szAmazonAssociate ) );
	ZeroMemory( g_szAmazonLocale, sizeof( g_szAmazonLocale ) );
	ZeroMemory( g_szXMLEncoding, sizeof( g_szXMLEncoding ) );
	ZeroMemory( g_szAppleAssociate, sizeof( g_szAppleAssociate ) );
	ZeroMemory( g_szTwitterMessage, sizeof( g_szTwitterMessage ) );
	ZeroMemory( g_szFacebookMessage, sizeof( g_szFacebookMessage ) );
	ZeroMemory( g_szFacebookAttachmentDescription, sizeof( g_szFacebookAttachmentDescription ) );
	ZeroMemory( g_szFacebookSessionKey, sizeof( g_szFacebookSessionKey ) );	

	//
	// Copy in some default values
	//

	StringCbCopy( g_szUploadProtocol, sizeof( g_szUploadProtocol ), UPLOAD_PROTOCOL_LABEL_NONE );
	StringCbCopy( g_szAmazonLocale, sizeof( g_szAmazonLocale ), AMAZON_LOCALE_LABEL_US );
	StringCbCopy( g_szXMLEncoding, sizeof( g_szXMLEncoding ), XML_ENCODING_LABEL_UTF_8 );

	//
	// Figure out where to write output at
	//

	StringCbCopy( g_szOutputFile, sizeof( g_szOutputFile ), g_szAppWorkingDir );
	PathAddBackslash( g_szOutputFile );
	StringCchCat( g_szOutputFile, sizeof( g_szOutputFile ), "now_playing.xml" );

	//
	// Read in our settings
	//

	if ( RegOpenKeyEx( HKEY_CURRENT_USER, MY_REGISTRY_KEY, 0, KEY_QUERY_VALUE, &hKey ) == ERROR_SUCCESS )
	{
		int intLogLevel = 0;
		dwBufLen = sizeof( intLogLevel );
		if ( RegQueryValueEx( hKey, MY_REGISTRY_VALUE_LOGGING, NULL, NULL, (LPBYTE) &intLogLevel, &dwBufLen ) == ERROR_SUCCESS )
		{
			switch ( intLogLevel )
			{
				case 0:
				{
					break;
				}
				
				case 1:
				{
					AMLOG_SETLOGLEVEL_ERROR;
					break;
				}
				case 2:
				{
					AMLOG_SETLOGLEVEL_INFO;
					break;
				}
				case 3:
				{
					AMLOG_SETLOGLEVEL_DEBUG;
					break;
				}					
			}
		}

		dwBufLen = sizeof( g_szGuid );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_GUID, NULL, NULL, (LPBYTE) g_szGuid, &dwBufLen );

		dwBufLen = sizeof( g_szOutputFile );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_FILE, NULL, NULL, (LPBYTE) g_szOutputFile, &dwBufLen );

		dwBufLen = sizeof( g_szLicenseKey );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_LICENSE_KEY, NULL, NULL, (LPBYTE) g_szLicenseKey, &dwBufLen );

		dwBufLen = sizeof( g_szEmail );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_EMAIL, NULL, NULL, (LPBYTE) g_szEmail, &dwBufLen );

		dwBufLen = sizeof( g_szXMLEncoding );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_XML_ENCODING, NULL, NULL, (LPBYTE) g_szXMLEncoding, &dwBufLen );

		dwBufLen = sizeof( g_szUploadProtocol );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_PROTOCOL, NULL, NULL, (LPBYTE) g_szUploadProtocol, &dwBufLen );

		dwBufLen = sizeof( g_intFTPPassive );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_FTP_PASSIVE, NULL, NULL, (LPBYTE) &g_intFTPPassive, &dwBufLen );

		dwBufLen = sizeof( g_szFTPHost );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_HOST, NULL, NULL, (LPBYTE) g_szFTPHost, &dwBufLen );

		dwBufLen = sizeof( g_szFTPUser );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_USER, NULL, NULL, (LPBYTE) g_szFTPUser, &dwBufLen );

		dwBufLen = sizeof( szTemp );
		if ( RegQueryValueEx( hKey, MY_REGISTRY_VALUE_PASSWORD, NULL, NULL, (LPBYTE) szTemp, &dwBufLen ) == ERROR_SUCCESS )
		{
			if ( strlen( szTemp ) > 0 )
			{
				DecryptString( szTemp, g_szFTPPassword, MAX_PATH );
			}
		}
		
		dwBufLen = sizeof( g_szFTPPath );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_PATH, NULL, NULL, (LPBYTE) g_szFTPPath, &dwBufLen );

		dwBufLen = sizeof( g_szStyleSheet );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_STYLESHEET, NULL, NULL, (LPBYTE) g_szStyleSheet, &dwBufLen );

		dwBufLen = sizeof( g_szAppleAssociate );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_APPLE_ASSOCIATE, NULL, NULL, (LPBYTE) g_szAppleAssociate, &dwBufLen );
		
		dwBufLen = sizeof( g_szAmazonAssociate );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_AMAZON_ASSOCIATE, NULL, NULL, (LPBYTE) g_szAmazonAssociate, &dwBufLen );

		dwBufLen = sizeof( g_szAmazonLocale );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_AMAZON_LOCALE, NULL, NULL, (LPBYTE) g_szAmazonLocale, &dwBufLen );

		dwBufLen = sizeof( g_intPlaylistBufferDelay );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_DELAY, NULL, NULL, (LPBYTE) &g_intPlaylistBufferDelay, &dwBufLen );

		dwBufLen = sizeof( g_intSkipShort );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_SKIPSHORT, NULL, NULL, (LPBYTE) &g_intSkipShort, &dwBufLen );

		dwBufLen = sizeof( g_szSkipKinds );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_SKIPKINDS, NULL, NULL, (LPBYTE) g_szSkipKinds, &dwBufLen );

		dwBufLen = sizeof( g_intPlaylistLength );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_PLAYLIST_LENGTH, NULL, NULL, (LPBYTE) &g_intPlaylistLength, &dwBufLen );

		dwBufLen = sizeof( g_intPublishStop );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_PUBLISH_STOP, NULL, NULL, (LPBYTE) &g_intPublishStop, &dwBufLen );

		dwBufLen = sizeof( g_intPlaylistClear );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_CLEAR_PLAYLIST, NULL, NULL, (LPBYTE) &g_intPlaylistClear, &dwBufLen );

		dwBufLen = sizeof( g_intUseXmlCData );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_XML_CDATA, NULL, NULL, (LPBYTE) &g_intUseXmlCData, &dwBufLen );

		dwBufLen = sizeof( g_szTrackBackUrl );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_TRACKBACK_URL, NULL, NULL, (LPBYTE) g_szTrackBackUrl, &dwBufLen );

		dwBufLen = sizeof( szTemp );
		if ( RegQueryValueEx( hKey, MY_REGISTRY_VALUE_TRACKBACK_PASSPHRASE, NULL, NULL, (LPBYTE) szTemp, &dwBufLen ) == ERROR_SUCCESS )
		{
			if ( strlen( szTemp ) > 0 )
			{
				DecryptString( szTemp, g_szTrackBackPassphrase, MAX_PATH );
			}
		}

		dwBufLen = sizeof( time_t );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_UPDATE_TIMESTAMP, NULL, NULL, (LPBYTE) &g_timeLastUpdateCheck, &dwBufLen );

		dwBufLen = sizeof( g_intAppleEnabled );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_APPLE_ENABLED, NULL, NULL, (LPBYTE) &g_intAppleEnabled, &dwBufLen );

		dwBufLen = sizeof( g_intAmazonEnabled );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_AMAZON_ENABLED, NULL, NULL, (LPBYTE) &g_intAmazonEnabled, &dwBufLen );

		dwBufLen = sizeof( g_intAmazonUseASIN );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_AMAZON_USE_ASIN, NULL, NULL, (LPBYTE) &g_intAmazonUseASIN, &dwBufLen );

		dwBufLen = sizeof( g_intArtworkUpload );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_ARTWORK_UPLOAD, NULL, NULL, (LPBYTE) &g_intArtworkUpload, &dwBufLen );

		dwBufLen = sizeof( g_intArtworkWidth );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_ARTWORK_WIDTH, NULL, NULL, (LPBYTE) &g_intArtworkWidth, &dwBufLen );

		dwBufLen = sizeof( g_intTwitterEnabled );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_TWITTER_ENABLED, NULL, NULL, (LPBYTE) &g_intTwitterEnabled, &dwBufLen );

		dwBufLen = sizeof( g_intTwitterRateLimitMinutes );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_TWITTER_RATE_LIMIT_MINUTES, NULL, NULL, (LPBYTE) &g_intTwitterRateLimitMinutes, &dwBufLen );

		ReadPreference( hKey, MY_REGISTRY_VALUE_TWITTER_MESSAGE, g_szTwitterMessage, sizeof( g_szTwitterMessage ) );

		dwBufLen = sizeof( g_szTwitterAuthKey );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_TWITTER_KEY, NULL, NULL, (LPBYTE) g_szTwitterAuthKey, &dwBufLen );

		dwBufLen = sizeof( g_szTwitterAuthSecret);
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_TWITTER_SECRET, NULL, NULL, (LPBYTE) g_szTwitterAuthSecret, &dwBufLen );

		dwBufLen = sizeof( g_timetTwitterLatest );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_TWITTER_LATEST, NULL, NULL, (LPBYTE) &g_timetTwitterLatest, &dwBufLen );

		dwBufLen = sizeof( g_intFacebookEnabled );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_FACEBOOK_ENABLED, NULL, NULL, (LPBYTE) &g_intFacebookEnabled, &dwBufLen );

		dwBufLen = sizeof( g_intFacebookRateLimitMinutes );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_FACEBOOK_RATE_LIMIT_MINUTES, NULL, NULL, (LPBYTE) &g_intFacebookRateLimitMinutes, &dwBufLen );

		ReadPreference( hKey, MY_REGISTRY_VALUE_FACEBOOK_MESSAGE, g_szFacebookMessage, sizeof( g_szFacebookMessage ) );
		ReadPreference( hKey, MY_REGISTRY_VALUE_FACEBOOK_ATTACHMENT_DESCRIPTION, g_szFacebookAttachmentDescription, sizeof( g_szFacebookAttachmentDescription ) );

		dwBufLen = sizeof( g_szFacebookSessionKey );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_FACEBOOK_SESSION_KEY, NULL, NULL, (LPBYTE) g_szFacebookSessionKey, &dwBufLen );

		dwBufLen = sizeof( g_timetFacebookLatest );
		RegQueryValueEx( hKey, MY_REGISTRY_VALUE_FACEBOOK_LATEST, NULL, NULL, (LPBYTE) &g_timetFacebookLatest, &dwBufLen );

		//
		// Remove old settings
		//

		RegDeleteValue( hKey, "Twitter Username" );
		RegDeleteValue( hKey, "Twitter Password" );

		RegCloseKey( hKey );

		//
		// Is valid license?
		//
		
		g_fLicensed = IsLicensed( MY_SECRET_KEY, g_szEmail, MY_REGISTRY_KEY, g_szLicenseKey );

		//
		// Default values
		//

		if ( strlen( g_szTwitterMessage ) == 0 )
		{
			StringCbCopy( g_szTwitterMessage, sizeof( g_szTwitterMessage ), TWITTER_MESSAGE_DEFAULT );
		}

		if ( strlen( g_szFacebookMessage ) == 0 )
		{
			StringCbCopy( g_szFacebookMessage, sizeof( g_szFacebookMessage ), FACEBOOK_MESSAGE_DEFAULT );
		}

		if ( strchr(g_szAmazonAssociate, '@') != NULL)
		{
			ZeroMemory( g_szAmazonAssociate, sizeof( g_szAmazonAssociate ) );
		}
	}

	//
	// Create GUID
	//

	if (strlen(g_szGuid) == 0)
	{
		unsigned char* lpszUuid = NULL;
		UUID uuid;

		ZeroMemory(&uuid, sizeof(UUID));

		UuidCreate(&uuid);
		UuidToString(&uuid, &lpszUuid);

		StringCbCopy(g_szGuid, sizeof(g_szGuid), (const char *) lpszUuid);

		SavePreference(MY_REGISTRY_VALUE_GUID, g_szGuid);

		if (lpszUuid != NULL)
		{
			RpcStringFree(&lpszUuid);
		}
	}

	if (IsFirstTime())
	{
		//
		// Show the configuration dialog once
		//

		DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_INSTALL);

		MessageBox( NULL, "It appears that this is the first time you have used the Now Playing plugin. The configuration window will appear now so you can set your preferences.", PRODUCT_NAME, MB_OK + MB_ICONINFORMATION );

		DoPropertySheet();
	}
	else
	{
		//
		// Check for updates
		//

		DoSoftwareUpdateCheck();
	}

	//
	// Log stuff now that we know the log level
	//

	AMLOGINFO( "Version = %s", g_szVersion );

	//
	// Create the playlist
	//

	g_paPlaylist = new CPlaylistItem[ g_intPlaylistLength ];

	if ( g_paPlaylist == NULL )
	{
		return false;
	}

	//
	// Load up a stored playlist
	//

	DoPlaylistLoad();

	DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_LAUNCH);

	AMLOGDEBUG( "End DoInit" );

	return true;
}
