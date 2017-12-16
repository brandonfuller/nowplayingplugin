#include <windows.h>
#include <time.h>
#include <objbase.h>

#include <atlbase.h>
CComModule _Module; // dummy required to utilize atlcom.h
#include <atlcom.h>

#include "resource.h"
#include "iTunesVisualAPI.h"
#include "Defines.h"
#include "strsafe.h"
#include "Utilities.h"
#include "Date.h"
#include "AmLog.h"

#include "iTunesCOMInterface.h"
#include "iTunesCOMInterface_i.c"

#if TARGET_OS_WIN32
#define	MAIN iTunesPluginMain
#define IMPEXP	__declspec(dllexport)
#else
#define IMPEXP
#define	MAIN main
#endif

#define	kSampleVisualPluginCreator	'\?\?\?\?'

#define	kSampleVisualPluginMajorVersion		3
#define	kSampleVisualPluginMinorVersion		2
#define	kSampleVisualPluginReleaseStage		1
#define	kSampleVisualPluginNonFinalRelease	0

enum
{
	kSettingsDialogResID = 1000,
	
	kSettingsDialogOKButton	= 1,
	kSettingsDialogCancelButton,
	
	kSettingsDialogCheckBox1,
	kSettingsDialogCheckBox2,
	kSettingsDialogCheckBox3
};

struct VisualPluginData 
{
	void *				appCookie;
	ITAppProcPtr		appProc;

#if TARGET_OS_MAC
	ITFileSpec			pluginFileSpec;
#endif
	
	HWND				destPort;
	Rect				destRect;
	OptionBits			destOptions;
	UInt32				destBitDepth;

	RenderVisualData	renderData;
	UInt32				renderTimeStampID;

	ITTrackInfoV1		trackInfo;
	ITStreamInfoV1		streamInfo;

	Boolean				playing;
	Boolean				padding[3];
};

typedef struct VisualPluginData VisualPluginData;

static void ClearMemory (LogicalAddress dest, SInt32 length)
{
	register unsigned char	*ptr;

	ptr = (unsigned char *) dest;
	
	if( length > 16 )
	{
		register unsigned long	*longPtr;
		
		while( ((unsigned long) ptr & 3) != 0 )
		{
			*ptr++ = 0;
			--length;
		}
		
		longPtr = (unsigned long *) ptr;
		
		while( length >= 4 )
		{
			*longPtr++ 	= 0;
			length		-= 4;
		}
		
		ptr = (unsigned char *) longPtr;
	}
	
	while( --length >= 0 )
	{
		*ptr++ = 0;
	}
}

static void RenderVisualPort(VisualPluginData *visualPluginData, HWND destPort, const Rect *destRect, Boolean onlyUpdate)
{
	(void) visualPluginData;
	(void) onlyUpdate;
	
	if (destPort == nil)
		return;

	//
	// Get DC for window
	//
	
	HDC hdc = GetDC( destPort );

	//
	// Create an off-screen DC for double-buffering
	//

	HDC hdcMem = CreateCompatibleDC( hdc );
    HBITMAP hbmMem = CreateCompatibleBitmap( hdc, destRect->right - destRect->left, destRect->bottom - destRect->top );
    HANDLE hOld = SelectObject( hdcMem, hbmMem );

	//
	// Paint it black
	// It is not necessary to call DeleteObject to delete stock objects.
	//

	SelectObject( hdcMem, GetStockObject( BLACK_BRUSH ) );

	//
	// Get the cover art
	//	

	if ( !g_hBitmap && visualPluginData->playing && strlen( g_szAlbumArtUrl ) )
	{
		g_hBitmap = LoadPicture( g_szAlbumArtUrl );

		g_szAlbumArtUrl[0] = '\0';
	}
	
	//
	// Draw the cover art
	//	

	if ( g_hBitmap && visualPluginData->playing )
	{
		BITMAP bmp;
		GetObject( g_hBitmap, sizeof( bmp ), &bmp );

		//
		// Amazon return 1x1 images sometimes when nothing is found
		//

		if ( bmp.bmWidth > 1 && bmp.bmHeight > 1 )
		{
			HDC hdcMemBitmap = CreateCompatibleDC( hdc );
			HBITMAP hbmMemBitmap = CreateCompatibleBitmap( hdc, bmp.bmWidth, bmp.bmHeight );

			HANDLE hOldObjBitmap = SelectObject( hdcMemBitmap, g_hBitmap );

			BitBlt( hdcMem, destRect->left + ( ( destRect->right - destRect->left ) / 2 ) - ( bmp.bmWidth / 2 ), destRect->top + ( ( destRect->bottom - destRect->top ) / 2 ) - ( bmp.bmHeight / 2 ), bmp.bmWidth, bmp.bmHeight, hdcMemBitmap, 0, 0, SRCCOPY );

			SelectObject( hdcMemBitmap, hOldObjBitmap );
			DeleteObject( hbmMemBitmap );
			DeleteDC( hdcMemBitmap );
		}
	}
	
	//
	// Transfer the off-screen DC to the screen
	//

	BitBlt( hdc, 0, 0, destRect->right - destRect->left, destRect->bottom - destRect->top, hdcMem, 0, 0, SRCCOPY );
	ReleaseDC( destPort, hdc );

	//
	// Free-up the off-screen DC
	//

    SelectObject( hdcMem, hOld );
    DeleteObject( hbmMem );
    DeleteDC( hdcMem );
}

static OSStatus AllocateVisualData(VisualPluginData *visualPluginData, const Rect *destRect)
{
	OSStatus		status;

	(void) visualPluginData;
	(void) destRect;

	status = noErr;

	return status;
}

static Boolean RectanglesEqual(const Rect *r1, const Rect *r2)
{
	if (
		(r1->left == r2->left) &&
		(r1->top == r2->top) &&
		(r1->right == r2->right) &&
		(r1->bottom == r2->bottom)
		)
		return true;
	return false;
	
}

static OSStatus ChangeVisualPort(VisualPluginData *visualPluginData, HWND destPort, const Rect *destRect)
{
	OSStatus		status;
	Boolean			doAllocate;
	
	status = noErr;
	
	doAllocate		= false;
		
	if (destPort != nil)
	{
		if (visualPluginData->destPort != nil)
		{
			if (RectanglesEqual(destRect, &visualPluginData->destRect) == false)
			{
				doAllocate		= true;
			}
		}
		else
		{
			doAllocate = true;
		}
	}
	
	if (doAllocate)
		status = AllocateVisualData(visualPluginData, destRect);

	if (status != noErr)
		destPort = nil;

	visualPluginData->destPort = destPort;
	if (destRect != nil)
		visualPluginData->destRect = *destRect;

	return status;
}

static OSStatus VisualPluginHandler(OSType message, VisualPluginMessageInfo *messageInfo, void *refCon)
{
	int i = 0;
	OSStatus status;
	VisualPluginData* visualPluginData = NULL;
	DWORD dwBufLen = 0;
	DWORD dwThreadId = 0;
	DWORD dwThrdParam = 1;
	DWORD dwExitCode = 0;
	
	visualPluginData = (VisualPluginData *) refCon;
	
	status = noErr;

	switch ( message )
	{
		//
		// Sent when the visual plugin is registered.  The plugin should do minimal
		// memory allocations here.  The resource fork of the plugin is still available.
		//
		
		case kVisualPluginInitMessage:
		{
			visualPluginData = (VisualPluginData *) malloc( sizeof( VisualPluginData ) );
			if ( visualPluginData == nil )
			{
				status = memFullErr;
				break;
			}

			AMLOGDEBUG( "iTunes Player Version = %d.%d.%d.%d", messageInfo->u.initMessage.appVersion.majorRev, messageInfo->u.initMessage.appVersion.minorAndBugRev, messageInfo->u.initMessage.appVersion.stage, messageInfo->u.initMessage.appVersion.nonRelRev );

			visualPluginData->appCookie	= messageInfo->u.initMessage.appCookie;
			visualPluginData->appProc	= messageInfo->u.initMessage.appProc;

			//
			// Remember the file spec of our plugin file. We need this so we can open our resource fork during
			// the configuration message.
			//

			messageInfo->u.initMessage.refCon = (void *) visualPluginData;

			//
			// Get the installation path
			//

			HKEY hKey;
			TCHAR szPath[1024];
			DWORD dwBufLen = sizeof( szPath );

			if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\iTunes.exe", 0, KEY_QUERY_VALUE, &hKey ) == ERROR_SUCCESS )
			{
				if ( RegQueryValueEx( hKey, NULL, NULL, NULL, (LPBYTE) szPath, &dwBufLen ) != ERROR_SUCCESS )
				{
					RegCloseKey( hKey );

					return paramErr;
				}

				RegCloseKey( hKey );
			}
			else
			{
				return paramErr;
			}

			LPTSTR lpszStart = strstr( szPath, "iTunes.exe" );
			if ( lpszStart )
			{
				*lpszStart = '\0';
			}

			HRESULT hr = StringCbCat( szPath, sizeof( szPath ), "Plug-ins\\NowPlaying.dll" );

			if ( !DoInit( MEDIA_PLAYER_ITUNES, szPath ) )
			{
				return paramErr;
			}

			break;
		}
			
		//
		//	Sent when the visual plugin is unloaded
		//
		
		case kVisualPluginCleanupMessage:
		{
			DoCleanup();

			if ( visualPluginData != nil )
			{
				free( visualPluginData );
			}

			break;
		}
			
		//
		// Sent when the visual plugin is enabled.  iTunes currently enables all
		// loaded visual plugins.  The plugin should not do anything here.
		//

		case kVisualPluginEnableMessage:
		case kVisualPluginDisableMessage:
		{
			break;
		}

		//
		// Sent if the plugin requests idle messages.  Do this by setting the kVisualWantsIdleMessages
		// option in the PlayerRegisterVisualPluginMessage.options field.
		//

		case kVisualPluginIdleMessage:
		{
			RenderVisualPort( visualPluginData, visualPluginData->destPort, &visualPluginData->destRect, false );
		
			break;
		}

		//
		// Sent when iTunes is going to show the visual plugin in a port.  At
		// this point, the plugin should allocate any large buffers it needs.
		//

		case kVisualPluginShowWindowMessage:
		{
			visualPluginData->destOptions = messageInfo->u.showWindowMessage.options;

			status = ChangeVisualPort( visualPluginData, messageInfo->u.showWindowMessage.window, &messageInfo->u.showWindowMessage.drawRect );
			if ( status == noErr )
			{
				RenderVisualPort(visualPluginData, visualPluginData->destPort, &visualPluginData->destRect, true);
			}
		
			break;
		}
			
		//
		//	Sent when iTunes is no longer displayed.
		//

		case kVisualPluginHideWindowMessage:
		{
			ChangeVisualPort( visualPluginData, nil, nil );

			ClearMemory( &visualPluginData->trackInfo, sizeof(visualPluginData->trackInfo) );
			ClearMemory( &visualPluginData->streamInfo, sizeof(visualPluginData->streamInfo) );
			
			break;
		}
		
		//
		// Sent when iTunes needs to change the port or rectangle of the currently
		// displayed visual.
		//

		case kVisualPluginSetWindowMessage:
		{
			visualPluginData->destOptions = messageInfo->u.setWindowMessage.options;

			status = ChangeVisualPort(	visualPluginData, messageInfo->u.setWindowMessage.window, &messageInfo->u.setWindowMessage.drawRect );
			
			if ( status == noErr )
			{
				RenderVisualPort( visualPluginData, visualPluginData->destPort, &visualPluginData->destRect, true );
			}
		
			break;
		}
		
		//
		// Sent for the visual plugin to render a frame.
		//

		case kVisualPluginRenderMessage:
		{
			visualPluginData->renderTimeStampID	= messageInfo->u.renderMessage.timeStampID;
				
			RenderVisualPort( visualPluginData, visualPluginData->destPort, &visualPluginData->destRect, false );
		
			break;
		}
			
		//
		// Sent in response to an update event.  The visual plugin should update
		// into its remembered port.  This will only be sent if the plugin has been
		// previously given a ShowWindow message.
		//
		
		case kVisualPluginUpdateMessage:
		{
			RenderVisualPort( visualPluginData, visualPluginData->destPort, &visualPluginData->destRect, true );
		
			break;
		}
		
		//
		// Sent when the player starts.
		//

		case kVisualPluginPlayMessage:
		{
			AMLOGDEBUG( "VisualPluginHandler handling kVisualPluginPlayMessage" );

			FreeVisualImage();

			if ( messageInfo->u.playMessage.trackInfo != nil )
			{
				visualPluginData->trackInfo = *messageInfo->u.playMessage.trackInfo;				

				CPlaylistItem x;
				x = messageInfo->u.playMessage.trackInfoUnicode;
				DoPlay( &x );
			}
			else
			{
				ClearMemory( &visualPluginData->trackInfo, sizeof( visualPluginData->trackInfo ) );
			}

			if ( messageInfo->u.playMessage.streamInfo != nil )
			{
				visualPluginData->streamInfo = *messageInfo->u.playMessage.streamInfo;
			}
			else
			{
				ClearMemory( &visualPluginData->streamInfo, sizeof( visualPluginData->streamInfo ) );
			}
		
			visualPluginData->playing = true;
		
			break;
		}

		//
		// Sent when the player changes the current track information.  This
		// is used when the information about a track changes, or when the CD
		// moves onto the next track.  The visual plugin should update any displayed
		// information about the currently playing song.
		//

		case kVisualPluginChangeTrackMessage:
		{
			AMLOGDEBUG( "VisualPluginHandler handling kVisualPluginChangeTrackMessage" );

			FreeVisualImage();

			if ( messageInfo->u.changeTrackMessage.trackInfo != nil )
			{
				visualPluginData->trackInfo = *messageInfo->u.changeTrackMessage.trackInfo;

				CPlaylistItem x;
				x = messageInfo->u.changeTrackMessage.trackInfoUnicode;
				DoPlay( &x );
			}
			else
			{
				ClearMemory( &visualPluginData->trackInfo, sizeof(visualPluginData->trackInfo) );
			}

			if ( messageInfo->u.changeTrackMessage.streamInfo != nil )
			{
				visualPluginData->streamInfo = *messageInfo->u.changeTrackMessage.streamInfo;
			}
			else
			{
				ClearMemory( &visualPluginData->streamInfo, sizeof(visualPluginData->streamInfo) );
			}
		
			break;
		}

		//
		// Sent when the player stops.
		//

		case kVisualPluginStopMessage:
		{
			AMLOGDEBUG( "VisualPluginHandler handling kVisualPluginStopMessage" );

			visualPluginData->playing = false;
		
			RenderVisualPort( visualPluginData, visualPluginData->destPort, &visualPluginData->destRect, true );
			
			break;
		}
		
		//
		// Sent when the player changes the track position.
		//

		case kVisualPluginSetPositionMessage:
		{
			break;
		}

		//
		// Sent when the player pauses.  iTunes does not currently use pause or unpause.
		// A pause in iTunes is handled by stopping and remembering the position.
		//

		case kVisualPluginPauseMessage:
		{
			AMLOGDEBUG( "VisualPluginHandler handling kVisualPluginPauseMessage" );

			visualPluginData->playing = false;

			RenderVisualPort( visualPluginData, visualPluginData->destPort, &visualPluginData->destRect, true );
		
			break;
		}
			
		//
		// Sent when the player unpauses.  iTunes does not currently use pause or unpause.
		// A pause in iTunes is handled by stopping and remembering the position.
		//

		case kVisualPluginUnpauseMessage:
		{
			AMLOGDEBUG( "VisualPluginHandler handling kVisualPluginUnpauseMessage" );

			visualPluginData->playing = true;
		
			break;
		}
		
		//
		// Sent to the plugin in response to a MacOS event.  The plugin should return noErr
		// for any event it handles completely, or an error (unimpErr) if iTunes should handle it.
		//

		case kVisualPluginEventMessage:
		{
			status = unimpErr;
		
			break;
		}

		case kVisualPluginConfigureMessage:
		{
			//
			// Show our stuff
			//

			DoPropertySheet();

			break;
		}

		default:
		{
			status = unimpErr;
		
			break;
		}
	}

	return status;	
}

static OSStatus RegisterVisualPlugin(PluginMessageInfo *messageInfo)
{
	OSStatus			status;
	PlayerMessageInfo	playerMessageInfo;
		
	ClearMemory( &playerMessageInfo.u.registerVisualPluginMessage, sizeof( playerMessageInfo.u.registerVisualPluginMessage ) );

	TRACE( "iTunes API Version = %d.%d", messageInfo->u.initMessage.majorVersion, messageInfo->u.initMessage.minorVersion );
	
	// copy in name length byte first
	playerMessageInfo.u.registerVisualPluginMessage.name[0] = lstrlen( PRODUCT_NAME );

	// now copy in actual name
	memcpy( &playerMessageInfo.u.registerVisualPluginMessage.name[1], PRODUCT_NAME, lstrlen( PRODUCT_NAME ) );

	SetNumVersion( &playerMessageInfo.u.registerVisualPluginMessage.pluginVersion, kSampleVisualPluginMajorVersion, kSampleVisualPluginMinorVersion, kSampleVisualPluginReleaseStage, kSampleVisualPluginNonFinalRelease );

	playerMessageInfo.u.registerVisualPluginMessage.options					= kVisualWantsIdleMessages | kVisualWantsConfigure;
	playerMessageInfo.u.registerVisualPluginMessage.handler					= VisualPluginHandler;
	playerMessageInfo.u.registerVisualPluginMessage.registerRefCon			= 0;
	playerMessageInfo.u.registerVisualPluginMessage.creator					= kSampleVisualPluginCreator;
	
	playerMessageInfo.u.registerVisualPluginMessage.timeBetweenDataInMS		= 16; // 16 milliseconds = 1 Tick, 0xFFFFFFFF = Often as possible.
	playerMessageInfo.u.registerVisualPluginMessage.numWaveformChannels		= 0;
	playerMessageInfo.u.registerVisualPluginMessage.numSpectrumChannels		= 0;
	
	playerMessageInfo.u.registerVisualPluginMessage.minWidth				= 64;
	playerMessageInfo.u.registerVisualPluginMessage.minHeight				= 64;
	playerMessageInfo.u.registerVisualPluginMessage.maxWidth				= 32767;
	playerMessageInfo.u.registerVisualPluginMessage.maxHeight				= 32767;
	playerMessageInfo.u.registerVisualPluginMessage.minFullScreenBitDepth	= 0;
	playerMessageInfo.u.registerVisualPluginMessage.maxFullScreenBitDepth	= 0;
	playerMessageInfo.u.registerVisualPluginMessage.windowAlignmentInBytes	= 0;
	
	status = PlayerRegisterVisualPlugin( messageInfo->u.initMessage.appCookie, messageInfo->u.initMessage.appProc,&playerMessageInfo );
		
	return status;	
}

extern "C"
{
	IMPEXP OSStatus MAIN(OSType message, PluginMessageInfo *messageInfo, void *refCon)
	{
		OSStatus status;
		
		(void) refCon;
		
		switch ( message )
		{
			case kPluginInitMessage:
				status = RegisterVisualPlugin( messageInfo );
				break;
				
			case kPluginCleanupMessage:
				status = noErr;
				break;
				
			default:
				status = unimpErr;
				break;
		}
		
		return status;
	}
}

//
// Helpers
//

bool ExportArtwork(LPTSTR lpszFileLocal, size_t cbFileLocal, LPTSTR lpszFileRemote, size_t cbFileRemote)
{
	HRESULT hr = S_OK;
	bool fResult = false;
	CComPtr<IiTunes> pIiTunes = NULL;

	AMLOGDEBUG( "Begin ExportArtwork" );

	USES_CONVERSION;

	hr = ::CoCreateInstance( CLSID_iTunesApp, NULL, CLSCTX_LOCAL_SERVER, IID_IiTunes, (PVOID *) &pIiTunes );
	if ( SUCCEEDED( hr ) && pIiTunes )
	{
		CComPtr<IITTrack> pIITTrack = NULL;
		hr = pIiTunes->get_CurrentTrack( &pIITTrack );
		if ( SUCCEEDED( hr ) && pIITTrack )
		{		
			CComPtr<IITArtworkCollection> pIITArtworkCollection = NULL;
			hr = pIITTrack->get_Artwork( &pIITArtworkCollection );
			if ( SUCCEEDED( hr ) && pIITArtworkCollection )
			{
				long lngCount = 0;

				hr = pIITArtworkCollection->get_Count( &lngCount );
				if ( SUCCEEDED( hr ) )
				{
					AMLOGDEBUG( "Art Collection Count is %d", lngCount );

					if ( lngCount > 0 )
					{
						CComPtr<IITArtwork> pIITArtwork = NULL;
						hr = pIITArtworkCollection->get_Item( 1, &pIITArtwork );
						if ( SUCCEEDED( hr ) && pIITArtwork )
						{
							ITArtworkFormat theFormat = ITArtworkFormatUnknown;
							hr = pIITArtwork->get_Format( &theFormat );
							if ( SUCCEEDED( hr ) && theFormat != ITArtworkFormatUnknown )
							{
								CComBSTR bstrFileNameLocal;
								CComBSTR bstrFileNameRemote;

								bstrFileNameLocal = lpszFileLocal;
								bstrFileNameRemote = lpszFileRemote;
								
								if ( theFormat == ITArtworkFormatJPEG )
								{
									bstrFileNameLocal += ".jpg";
									bstrFileNameRemote += ".jpg";
								}
								else if ( theFormat == ITArtworkFormatPNG )
								{
									bstrFileNameLocal += ".png";
									bstrFileNameRemote += ".png";
								}
								else if ( theFormat == ITArtworkFormatBMP )
								{
									bstrFileNameLocal += ".bmp";
									bstrFileNameRemote += ".bmp";
								}

								hr = pIITArtwork->SaveArtworkToFile( bstrFileNameLocal );
								if ( SUCCEEDED( hr )  )
								{
									AMLOGDEBUG( "Saved artwork to %s", OLE2T( bstrFileNameLocal ) );
									
									StringCbCopy( lpszFileLocal, cbFileLocal, OLE2T( bstrFileNameLocal ) );
									StringCbCopy( lpszFileRemote, cbFileRemote, OLE2T( bstrFileNameRemote ) );

									fResult = true;
								}
							}
						}
					}
				}
			}
		}
	}

	AMLOGDEBUG( "End ExportArtwork" );

	return fResult;
}