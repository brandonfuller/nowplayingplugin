
#include <time.h>
#include <wininet.h>

#ifdef _DEBUG
#define TRACE Trace
#else
#define TRACE 1 ? (void) 0 : Trace
#endif

void Trace(LPCTSTR lpszFormat, ...);

LONG GetTimezoneInMinutes();
void FreeVisualImage();
bool ReadResponseBody(HINTERNET hRequest, char * lpszResponse, size_t cbResponse);

class CPlaylistItem
{

public:

	CPlaylistItem::CPlaylistItem()
	{
		m_fSet = false;

		ZeroMemory( &m_trackInfo, sizeof( m_trackInfo ) );
		ZeroMemory( m_szImageUrl, sizeof( m_szImageUrl ) );
		ZeroMemory( m_szImageLargeUrl, sizeof( m_szImageLargeUrl ) );
		ZeroMemory( m_szImageSmallUrl, sizeof( m_szImageSmallUrl ) );
		ZeroMemory( m_szUrlAmazon, sizeof( m_szUrlAmazon ) );
		ZeroMemory( m_szUrlApple, sizeof( m_szUrlApple ) );
		ZeroMemory( m_szTimestamp, sizeof( m_szTimestamp ) );
		ZeroMemory( m_szUrlSource, sizeof( m_szUrlSource ) );
		ZeroMemory( m_szArtworkID, sizeof( m_szArtworkID ) );
	}

	CPlaylistItem::CPlaylistItem(const ITTrackInfo* pTrackInfo)
	{
		//
		// Clear out the items not copied
		//

		ZeroMemory( m_szImageUrl, sizeof( m_szImageUrl ) );
		ZeroMemory( m_szImageLargeUrl, sizeof( m_szImageLargeUrl ) );
		ZeroMemory( m_szImageSmallUrl, sizeof( m_szImageSmallUrl ) );
		ZeroMemory( m_szUrlAmazon, sizeof( m_szUrlAmazon ) );
		ZeroMemory( m_szUrlApple, sizeof( m_szUrlApple ) );
		ZeroMemory( m_szUrlSource, sizeof( m_szUrlSource ) );
		ZeroMemory( m_szArtworkID, sizeof( m_szArtworkID ) );

		//
		// Copy all the data over
		//

		memcpy( &m_trackInfo, pTrackInfo, sizeof( ITTrackInfo ) );

		//
  		// Null terminate the strings for convienence
  		//
  
  		m_trackInfo.album[ min( m_trackInfo.album[0] + 1, 255 ) ] = '\0';
  		m_trackInfo.artist[ min( m_trackInfo.artist[0] + 1, 255 ) ] = '\0';
  		m_trackInfo.name[ min( m_trackInfo.name[0] + 1, 255 ) ] = '\0';
		m_trackInfo.fileName[ min( m_trackInfo.fileName[0] + 1, 255 ) ] = '\0';
		m_trackInfo.genre[ min( m_trackInfo.genre[0] + 1, 255 ) ] = '\0';
		m_trackInfo.kind[ min( m_trackInfo.kind[0] + 1, 255 ) ] = '\0';
		m_trackInfo.comments[ min( m_trackInfo.comments[0] + 1, 255 ) ] = '\0';
		m_trackInfo.composer[ min( m_trackInfo.composer[0] + 1, 255 ) ] = '\0';
		m_trackInfo.grouping[ min( m_trackInfo.grouping[0] + 1, 255 ) ] = '\0';

		//
		// Set the timestamp
		//

		SetTimestamp();

		//
		// Since we are copying something, assume its a good track
		//

		m_fSet = true;	
	}

	CPlaylistItem::operator=(const CPlaylistItem* pTrackInfo)
	{
		//
		// Clear out the items not copied
		//

		ZeroMemory( m_szImageUrl, sizeof( m_szImageUrl ) );
		ZeroMemory( m_szImageLargeUrl, sizeof( m_szImageLargeUrl ) );
		ZeroMemory( m_szImageSmallUrl, sizeof( m_szImageSmallUrl ) );
		ZeroMemory( m_szUrlAmazon, sizeof( m_szUrlAmazon ) );
		ZeroMemory( m_szUrlApple, sizeof( m_szUrlApple ) );
		ZeroMemory( m_szArtworkID, sizeof( m_szArtworkID ) );

		//
		// Copy all the data over
		//

		memcpy( &m_trackInfo, &pTrackInfo->m_trackInfo, sizeof( ITTrackInfo ) );
		StringCbCopy( m_szUrlSource, sizeof( m_szUrlSource ), pTrackInfo->m_szUrlSource );
		StringCbCopy( m_szArtworkID, sizeof( m_szArtworkID ), pTrackInfo->m_szArtworkID );

		//
  		// Null terminate the strings for convienence
  		//
  
  		m_trackInfo.album[ min( m_trackInfo.album[0] + 1, 255 ) ] = '\0';
  		m_trackInfo.artist[ min( m_trackInfo.artist[0] + 1, 255 ) ] = '\0';
  		m_trackInfo.name[ min( m_trackInfo.name[0] + 1, 255 ) ] = '\0';
		m_trackInfo.fileName[ min( m_trackInfo.fileName[0] + 1, 255 ) ] = '\0';
		m_trackInfo.genre[ min( m_trackInfo.genre[0] + 1, 255 ) ] = '\0';
		m_trackInfo.kind[ min( m_trackInfo.kind[0] + 1, 255 ) ] = '\0';
		m_trackInfo.comments[ min( m_trackInfo.comments[0] + 1, 255 ) ] = '\0';
		m_trackInfo.composer[ min( m_trackInfo.composer[0] + 1, 255 ) ] = '\0';
		m_trackInfo.grouping[ min( m_trackInfo.grouping[0] + 1, 255 ) ] = '\0';

		//
		// Set the timestamp
		//

		SetTimestamp();

		//
		// Since we are copying something, assume its a good track
		//

		m_fSet = true;
	}

	void CPlaylistItem::SetTimestamp()
	{
		//
		// Set the timestamp
		//

		time_t now_t;
		struct tm now;
		time( &now_t );
		now = *localtime( &now_t );
		LONG lTimezone = GetTimezoneInMinutes();

		if ( lTimezone == 0 )
		{
			StringCbPrintf( m_szTimestamp, sizeof( m_szTimestamp ), TIMEZONE_FORMAT_GMT, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec );
		}
		else
		{
			StringCbPrintf( m_szTimestamp, sizeof( m_szTimestamp ), TIMEZONE_FORMAT_ALL, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec, lTimezone < 0 ? "+" : "-", abs( lTimezone / 60 ), abs( lTimezone - ( lTimezone / 60 ) * 60 ) );
		}
	}

	CPlaylistItem::operator==(const CPlaylistItem* pTrackInfo)
	{
		if ( pTrackInfo )
		{	
			//
			// Do comparisons on some common fields
			//

			if ( memcmp( &pTrackInfo->m_trackInfo, &m_trackInfo, sizeof( m_trackInfo ) ) == 0 )
			{
				return true;
			}
		}

		return false;
	}

	//
	// Members
	//

	bool m_fSet;

	//
	// Standard iTunes track info
	//

	ITTrackInfo m_trackInfo;

	//
	// Extra data
	//

	TCHAR m_szImageUrl[MAX_PATH];
	TCHAR m_szImageLargeUrl[MAX_PATH];
	TCHAR m_szImageSmallUrl[MAX_PATH];
	TCHAR m_szUrlAmazon[1024];
	TCHAR m_szUrlApple[1024];	
	TCHAR m_szTimestamp[32];
	TCHAR m_szUrlSource[1024];
	TCHAR m_szArtworkID[128];
};

//
// My Globals
//

extern HBITMAP g_hBitmap;
extern char g_szAlbumArtUrl[1024];

//
// Helpers
//

HBITMAP LoadPicture(LPCTSTR pszPath);
void GetMyVersion(LPTSTR lpszDest, size_t cchDest);

//
// Hooks to implement within the plugin
//

bool DoInit(int intMediaPlayerType, LPCTSTR lpszPath);
void DoCleanup();

void DoPlay(CPlaylistItem* trackInfo);
void DoStop();

void DoPropertySheet(HWND hwndParent = NULL);

bool ExportArtwork(LPTSTR lpszFileLocal, size_t cbFileLocal, LPTSTR lpszFileRemote, size_t cbFileRemote);
