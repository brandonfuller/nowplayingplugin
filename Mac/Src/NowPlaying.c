/*
	File:		NowPlaying.c

	Contains:	visual plug-in for iTunes on Mac OS X
        
	Written by: 	Developer Technical Support

	Copyright:	Copyright © 2001 by Apple Computer, Inc., All Rights Reserved.

			You may incorporate this Apple sample source code into your program(s) without
			restriction. This Apple sample source code has been provided "AS IS" and the
			responsibility for its operation is yours. You are not permitted to redistribute
			this Apple sample source code as "Apple sample source code" after having made
			changes. If you're going to re-distribute the source, we require that you make
			it clear in the source that the code was descended from Apple sample source
			code, but that you've made changes.

	Change History (most recent first):
                        6/6/01	   KG	   moved to project builder on Mac OS X
                        4/17/01    DTS     first checked in.
*/

/**\
|**|	conditional compilation directives
\**/

#define QDMP_FLAG	1 	// 0 != use direct pixmap access

/**\
|**|	includes
\**/

#include "QDMP.h"
#include "iTunesVisualAPI.h"
#include "Defines.h"
#include "License.h"
#include "Encrypt.h"
#include "ScriptSupport.h"
#include "Global.h"
#include "MD5.h"
#include "b64.h"
#include "sha2.h"
#include "hmac_sha256.h"
#include "xmalloc.h"
#include "oauth.h"
#include <stdio.h>

/**\
|**|	typedef's, struct's, enum's, etc.
\**/

#define kTVisualPluginName              "Now Playing"
#define	kTVisualPluginCreator			'hook'

#define	kTVisualPluginMajorVersion		1
#define	kTVisualPluginMinorVersion		0
#define	kTVisualPluginReleaseStage		finalStage
#define	kTVisualPluginNonFinalRelease	0

#define kBundleName "name.brandon.fuller.nowplaying.mac.itunes"

#define MY_ENCODING_DEFAULT kCFStringEncodingUTF8
#define MY_REGISTRY_KEY "SOFTWARE\\Brandon Fuller\\Now Playing\\iTunes\\Mac"
#define URL_DOWNLOAD_UPDATE "http://brandon.fuller.name/downloads/nowplaying/NowPlaying.dmg"
#define UPDATE_URL "http://brandon.fuller.name/downloads/nowplaying/feed-update-mac.xml"
#define FACEBOOK_TAGS_SUPPORTED "<title> <artist> <album> <genre> <kind> <track> <numTracks> <year> <comments> <time> <bitrate> <rating> <disc> <numDiscs> <playCount> <composer> <grouping> <file> <image> <imageSmall> <imageLarge> <urlAmazon> <hasAmazon> </hasAmazon>"
#define TWITTER_TAGS_SUPPORTED "<title> <artist> <album> <genre> <kind> <track> <numTracks> <year> <comments> <time> <bitrate> <rating> <disc> <numDiscs> <playCount> <composer> <grouping> <file> <image> <imageSmall> <imageLarge> <urlAmazon> <hasAmazon> </hasAmazon>"

#define AMAZON_LOCALE_US 5

#define LOGGING_NONE 1
#define LOGGING_ERROR 2
#define LOGGING_INFO 3
#define LOGGING_DEBUG 4

#define DEFAULT_LOG_LEVEL LOGGING_ERROR

enum {kTabMasterSig = 'PRTT', kTabMasterID = 1000, kTabPaneSig = 'PRTB', kPrefControlsSig = 'PREF'};
enum {kDummyValue = 0, kMaxNumTabs = 10};

enum
{
    kNewSong = (1<<0),
    kUnlockedHandleSighted = (1<<1),
    kResEditDetected = (1<<2),
    kSelfDestruct = (1<<3),
	kCompleted = (1<<4)
};

typedef struct tagPlaylistItem
{
	bool fSet;
	ITTrackInfo track;
	bool fPlay;
	CFStringRef strDetailPageURL;
	CFStringRef strSmallImageUrl;
	CFStringRef strMediumImageUrl;
	CFStringRef strLargeImageUrl;
	CFStringRef strAppleURL;
	CFStringRef strArtworkId;
}
PlaylistItem;

bool g_fLicensed = false;
int g_intOnce = 0;
int g_intUseXmlCData = 1;

#define MAX_PATH 256

int g_intLogging = DEFAULT_LOG_LEVEL;
char g_szVersion[MAX_PATH] = {0};
char g_szGuid[MAX_PATH] = {0};
char g_szEmail[MAX_PATH] = {0};
char g_szLicenseKey[MAX_PATH] = {0};
char g_szKey[MAX_PATH] = {0};
char g_szOutputFile[MAX_PATH] = {0};
char g_szSkipKinds[MAX_PATH] = {0};
int g_intXmlEncoding = 1;
int g_intUploadProtocol = 0;
int g_intPlaylistLength = DEFAULT_PLAYLIST_LENGTH;
char g_szFTPHost[MAX_PATH] = {0};
char g_szFTPUser[MAX_PATH] = {0};
char g_szFTPPassword[MAX_PATH] = {0};
char g_szFTPPath[MAX_PATH] = {0};
int g_intArtworkExport = DEFAULT_ARTWORK_EXPORT;
int g_intArtworkUpload = DEFAULT_ARTWORK_UPLOAD;
int g_intArtworkWidth = DEFAULT_ARTWORK_WIDTH;
int g_intPublishStop = DEFAULT_PUBLISH_STOP;
int g_intSkipShort = DEFAULT_SKIPSHORT_SECONDS;
char g_szTrackBackUrl[MAX_PATH] = {0};
char g_szPingExtraInfo[MAX_PATH] = {0};
int g_intAmazonLookup = DEFAULT_AMAZON_ENABLED;
int g_intAmazonLocale = AMAZON_LOCALE_US;
char g_szAmazonAssociate[MAX_PATH];
int g_intAmazonUseASIN = DEFAULT_AMAZON_USE_ASIN;
int g_intAppleLookup = DEFAULT_APPLE_ENABLED;
char g_szAppleAssociate[MAX_PATH] = {0};
PlaylistItem* g_paPlaylist = NULL;
time_t g_timetChanged = 0;
time_t g_timeLastUpdateCheck = 0;
MPEventID g_hSignalPlayEvent;
MPEventID g_hSignalCleanupEvent;
int g_intNext = 0;
int g_intPlaylistBufferDelay = 0;
int g_intFacebookEnabled = 0;
char g_szFacebookMessage[1024] = {0};
char g_szFacebookAttachmentDescription[1024] = {0};
char g_szFacebookAuthToken[MAX_PATH] = {0};
char g_szFacebookSessionKey[MAX_PATH] = {0};
int g_intFacebookRateLimitMinutes = FACEBOOK_RATE_LIMIT_MINUTES_DEFAULT;
time_t g_timetFacebookLatest = 0;
int g_intTwitterEnabled = 0;
char g_szTwitterMessage[1024] = {0};
time_t g_timetExportedMarker = 0;
char g_szTwitterAuthKey[1024] = {0};
char g_szTwitterAuthSecret[1024] = {0};
int g_intTwitterRateLimitMinutes = TWITTER_RATE_LIMIT_MINUTES_DEFAULT;
time_t g_timetTwitterLatest = 0;

typedef struct VisualPluginData
{
	void *				appCookie;
	ITAppProcPtr		appProc;

	ITFileSpec			pluginFileSpec;
	
	CGrafPtr			destPort;
	Rect				destRect;
	OptionBits			destOptions;
	UInt32				destBitDepth;

	RenderVisualData	renderData;
	UInt32				renderTimeStampID;
	
	ITTrackInfo			trackInfo;
	ITStreamInfo		streamInfo;

	Boolean				playing;
	Boolean				padding[3];

} 
VisualPluginData;

void CacheMySettings(bool fFirst);
void DoUpdate(ITTrackInfo* x, bool fPlay);
void PrintTrackInfoNumberToFile(FILE * f, const char * lpszNodeName, int intNodeValue);
void PrintTrackInfoStringToFile(FILE* f, char* lpszNodeName, ITUniStr255 NodeValue, bool fUseCData);
void PrintTrackInfoCFStringToFile(FILE* f, char* lpszNodeName, CFStringRef strValue, bool fUseCData);
bool LoadMyPreferenceString(WindowRef wr, CFStringRef strPrefName, OSType theSig, SInt32 theNum, CFStringRef strPrefValueDefault, bool fEncrypted);
ControlRef GrabCRef(WindowRef theWindow, OSType theSig, SInt32 theNum);
void LoadSettingString(const char * lpszPrefName, char * lpszValue, size_t cbValue, const char * lpszDefault);
void DoGoogleAnalytics(char * lpszAction);

#if QDMP_FLAG
#define SET_PIXEL(pm,h,v,c) \
			QDMP_Set_Pixel(pm,h,v,c);
#else
#define SET_PIXEL(pm,h,v,c) \
			{ RGBForeColor(c);MoveTo(h,v);Line(1,0);}
#endif

void StringCbCopy(char* pszDest, size_t cchDest, const char* pszSrc)
{	
	while (cchDest && (*pszSrc != '\0'))
	{
		*pszDest++= *pszSrc++;
		cchDest--;
	}
	
	if (cchDest == 0)
	{
		pszDest--;
	}
	
	*pszDest= '\0';
}

int GetSelectedEncoding()
{
	if ( g_intXmlEncoding == 1 )
	{
		return kCFStringEncodingISOLatin1;
	}

	return kCFStringEncodingUTF8;
}

void EscapeQuotes(CFMutableStringRef string)
{
	CFRange range;												
	range.location = 0;
	range.length = CFStringGetLength(string);
	
	CFStringFindAndReplace(string, CFSTR("\""), CFSTR("\\\""), range, 0);
}

void OutputCFStringToStdErr(const char * lpszPrefix, CFStringRef cfString)
{
	if ( cfString )
	{
		const char * lpszValue = CFStringGetCStringPtr( cfString, kCFStringEncodingMacRoman );
		if ( lpszValue )
		{
			fprintf( stderr, "%s%s\n", lpszPrefix, lpszValue );
		}
		else
		{
			CFIndex size = CFStringGetMaximumSizeForEncoding( CFStringGetLength( cfString ), kCFStringEncodingMacRoman );
			char * lpszBuf = (char *) malloc( sizeof( char ) * size );
			if ( lpszBuf )
			{
				if ( CFStringGetCString( cfString, lpszBuf, size, kCFStringEncodingMacRoman ) )
				{       
					fprintf( stderr, "%s%s\n", lpszPrefix, lpszBuf );
				}

				free( lpszBuf );
			}
		}
	}
}

void LOGDEBUG(const char * lpszPrefix, CFStringRef cfString)
{
	if (g_intLogging == LOGGING_DEBUG)
	{
		OutputCFStringToStdErr(lpszPrefix, cfString);
	}
}

void AMLOGDEBUG(const char * format, ...)
{
	if (g_intLogging == LOGGING_DEBUG)
	{
		va_list args;
		va_start(args, format);
		vfprintf(stderr, format, args);
		va_end(args);
	}
}

void MessageBox(AlertType alertType, CFStringRef message)
{
	DialogRef dialog;
	CreateStandardAlert(alertType, message, NULL, NULL, &dialog);
	RunStandardAlert(dialog, NULL, NULL);
}

void MyPathRemoveFileSpec(char * lpszPath)
{
	char * lpszFound = NULL;

	if ( lpszPath )
	{
		lpszFound = strrchr( lpszPath, '/' );
		if ( lpszFound )
		{
			lpszFound++;
			*lpszFound = '\0';
		}
	}
}

void UrlEncode(char * lpszDest, size_t cbDest, const char * lpszSrc1)
{
	static char safechars[] = URL_ENCODE_SAFE_CHARS_RFC3986;
	size_t cbRemaining = cbDest;

	CFStringRef strNodeValue = CFStringCreateWithCString( kCFAllocatorDefault, lpszSrc1, kCFStringEncodingUTF8 );
	if ( strNodeValue )
	{
		char szBuffer[1024] = {0};
		char * lpszSrc = &szBuffer[0];

		if ( CFStringGetCString( strNodeValue, &szBuffer[0], sizeof( szBuffer ), kCFStringEncodingUTF8 ) )
		{
			while ( *lpszSrc != '\0' && cbRemaining > 1 )
			{
				if ( strchr( safechars, *lpszSrc ) != NULL )
				{
					*lpszDest = *lpszSrc;
					cbDest++;
				}
				else
				{
					sprintf( lpszDest, "%%%02X", (unsigned char) *lpszSrc );
					lpszDest += 2;
					cbDest += 2;
					cbRemaining -= 2;
				}

				++lpszSrc;
				++lpszDest;
				--cbRemaining;
			}

			*lpszDest = '\0';
		}
				
		CFRelease( strNodeValue );
	}
}

CFStringRef UrlDecode(CFStringRef originalString)
{
	return CFURLCreateStringByReplacingPercentEscapes(kCFAllocatorDefault, originalString, CFSTR(""));
}

void UrlEncodeAndAppend(CFMutableStringRef strData, ITUniStr255 value)
{
	CFStringRef strValue = CFStringCreateWithBytes(kCFAllocatorDefault, (UInt8 *) &value[1], value[0] * sizeof(UniChar), kCFStringEncodingUnicode, false);
	if (strValue)
	{
		CFIndex size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(strValue), kCFStringEncodingUTF8) + sizeof(char);
		char * lpszBuffer = malloc( size );
		if (lpszBuffer)
		{
			if (CFStringGetCString(strValue, lpszBuffer, size, kCFStringEncodingUTF8))
			{
				char * lpszBufferEncoded = malloc(size * 3);
				if (lpszBufferEncoded)
				{
					UrlEncode(lpszBufferEncoded, size * 3, lpszBuffer);
					CFStringAppendCString(strData, lpszBufferEncoded, kCFStringEncodingUTF8);
					
					free(lpszBufferEncoded);
				}
			}
			
			free(lpszBuffer);
		}
		
		CFRelease(strValue);
	}
}

void UrlEncodeAndConcat(char * lpszDest, size_t cbDest, const char * lpszSrc)
{
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
			sprintf( lpszDest, "%%%02X", (unsigned char) *lpszSrc );
			lpszDest += 2;
			cbDest += 2;
			cbRemaining -= 2;
		}

		++lpszSrc;
		++lpszDest;
		--cbRemaining;
	}

	*lpszDest = '\0';
}

void EscapeForExpect(char * lpszDest, size_t cbDest, const char * lpszSrc)
{
	static char badchars[] = "${}";
	size_t cbRemaining = cbDest;

	//
	// Advance to the end of the existing string
	//
	
	lpszDest += strlen( lpszDest );
	
	while ( *lpszSrc != '\0' && cbRemaining > 1 )
	{
		if ( strchr( badchars, *lpszSrc ) != NULL )
		{
			*lpszDest = '\\';
			cbDest++;
			++lpszDest;
			--cbRemaining;
		}

		// Copy the character
		*lpszDest = *lpszSrc;
		cbDest++;
		
		++lpszSrc;
		++lpszDest;
		--cbRemaining;
	}
	
	*lpszDest = '\0';
}

CFStringRef GetRandomHTMLChunk(CFStringRef strData, CFStringRef strBegin, CFStringRef strEnd)
{
	CFStringRef strResult = NULL;

	CFRange rangeBegin = CFStringFind(strData, strBegin, 0);		
	if (rangeBegin.location != kCFNotFound)
	{
		CFRange rangeEndFind;

		rangeEndFind.location = rangeBegin.location + rangeBegin.length;
		rangeEndFind.length = CFStringGetLength( strData ) - rangeEndFind.location;
		
		CFRange rangeEnd;

		if (CFStringFindWithOptions( strData, strEnd, rangeEndFind, 0, &rangeEnd))
		{
			if (rangeEnd.location != kCFNotFound)
			{
				CFRange rangeSub;

				rangeSub.location = rangeBegin.location + rangeBegin.length;
				rangeSub.length = rangeEnd.location - rangeSub.location;

				strResult = CFStringCreateWithSubstring(kCFAllocatorDefault, strData, rangeSub);
				
				LOGDEBUG("GetRandomHTMLChunk Found = ", strResult);
			}
		}
		else
		{
			LOGDEBUG("GetRandomHTMLChunk failed to find end = ", strEnd);
		}		
	}
	else
	{
		LOGDEBUG("GetRandomHTMLChunk failed to find begin = ", strBegin);
	}

	return strResult;
}

CFStringRef GetXMLTagContents(CFStringRef strData, CFStringRef strTagName)
{
	CFStringRef strResult = NULL;
	CFStringRef strTagBegin = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("<%@"), strTagName);
	CFStringRef strTagEnd = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("</%@>"), strTagName);

	CFRange rangeBegin = CFStringFind(strData, strTagBegin, 0);
	if (rangeBegin.location != kCFNotFound)
	{
		CFRange rangeBeginEndFind;

		rangeBeginEndFind.location = rangeBegin.location + rangeBegin.length;
		rangeBeginEndFind.length = CFStringGetLength( strData ) - rangeBeginEndFind.location;
		
		CFRange rangeBeginEndTag;

		if (CFStringFindWithOptions( strData, CFSTR(">"), rangeBeginEndFind, 0, &rangeBeginEndTag))
		{
			CFRange rangeEndFind;

			rangeEndFind.location = rangeBeginEndTag.location + rangeBeginEndTag.length;
			rangeEndFind.length = CFStringGetLength( strData ) - rangeEndFind.location;
						
			CFRange rangeEnd;

			if (CFStringFindWithOptions( strData, strTagEnd, rangeEndFind, 0, &rangeEnd))
			{
				if (rangeEnd.location != kCFNotFound)
				{
					CFRange rangeSub;

					rangeSub.location = rangeEndFind.location;
					rangeSub.length = rangeEnd.location - rangeSub.location;

					strResult = CFStringCreateWithSubstring( kCFAllocatorDefault, strData, rangeSub );
					
					if (g_intLogging == LOGGING_DEBUG)					
					{
						OutputCFStringToStdErr("Got XML Tag Contents = ", strResult);
					}
				}
			}
		}
	}
	else
	{
		LOGDEBUG("GetXMLTagContents failed to find tag = ", strTagName);
	}

	CFRelease( strTagBegin );
	CFRelease( strTagEnd );

	return strResult;
}

OSStatus LaunchURL(const char * lpszUrl)
{
	OSStatus err;
    ICInstance inst;
    long startSel = 0;
    long endSel = 0;
    err = ICStart( &inst, 'udog' );
    if ( err == noErr )
	{
		startSel = 0;
		endSel = strlen( lpszUrl );
		err = ICLaunchURL( inst, "\p", lpszUrl, strlen( lpszUrl ), &startSel, &endSel );

		ICStop( inst );
    }

    return err;
}

CFMutableStringRef GetUrlContents(CFStringRef strUrl, CFStringRef strMethod, CFStringRef strPostData)
{
	CFMutableStringRef strData = CFStringCreateMutable(kCFAllocatorDefault, 0);
	if (strData)
	{									
		LOGDEBUG("URL: ", strUrl);

		CFURLRef urlRef = CFURLCreateWithString(kCFAllocatorDefault, strUrl, NULL);
		if (urlRef)
		{
			CFHTTPMessageRef myRequest = CFHTTPMessageCreateRequest( kCFAllocatorDefault, strMethod, urlRef, kCFHTTPVersion1_1 );
			if (myRequest)
			{
				if (CFStringCompare(strMethod, CFSTR("POST"), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
				{
					CFHTTPMessageSetHeaderFieldValue(myRequest, CFSTR("Content-Type"), CFSTR("application/x-www-form-urlencoded; charset=UTF-8"));
					if (strData)
					{
						CFIndex lenData = CFStringGetLength(strPostData);
						UInt8* buffer = CFAllocatorAllocate(kCFAllocatorDefault, lenData, 0);
						if (buffer)
						{
							CFStringGetBytes(strPostData, CFRangeMake(0, lenData), kCFStringEncodingASCII, 0, FALSE, buffer, lenData, NULL);
		 
							CFDataRef postData = CFDataCreate(kCFAllocatorDefault, buffer, lenData);
							if (postData)
							{
								CFHTTPMessageSetBody(myRequest, postData);

								LOGDEBUG("POST Data: ", strPostData);								

								CFRelease(postData);
							}

							CFAllocatorDeallocate(NULL, buffer);
						}
					}
				}

				CFReadStreamRef myReadStream = CFReadStreamCreateForHTTPRequest(kCFAllocatorDefault, myRequest);
				if ( myReadStream )
				{
					if ( CFReadStreamOpen( myReadStream ) )
					{
						bool done = false;

						while ( !done )
						{
							UInt8 buf[1024];
							CFIndex bytesRead = CFReadStreamRead( myReadStream, buf, sizeof( buf ) );
							if (bytesRead < 0)
							{
								CFStreamError error = CFReadStreamGetError( myReadStream );
								fprintf( stderr, "Stream read error, domain = %d, error = %ld\n", error.domain, error.error );
								done = true;
							}
							else if (bytesRead == 0)
							{
								if ( CFReadStreamGetStatus( myReadStream ) == kCFStreamStatusAtEnd )
								{
									done = true;
								}
							}
							else
							{
								buf[bytesRead] = '\0';
								CFStringAppendCString( strData, (char *) buf, kCFStringEncodingUTF8 );
							}
						}
						
						LOGDEBUG("Stream Returned: ", strData);

						CFHTTPMessageRef myResponse = CFReadStreamCopyProperty( myReadStream, kCFStreamPropertyHTTPResponseHeader );
						if ( myResponse )
						{
							UInt32 myErrCode = CFHTTPMessageGetResponseStatusCode( myResponse );						
							if ( myErrCode == 302 )
							{
								CFStringRef strLocation = CFHTTPMessageCopyHeaderFieldValue( myResponse, CFSTR("Location") );
								if ( strLocation )
								{
									CFRelease( strData );
									strData = GetUrlContents( strLocation, CFSTR("GET"), NULL );
									CFRelease( strLocation );
								}
							}
						}
					}
					else
					{
						CFStreamError myErr = CFReadStreamGetError( myReadStream );
						if ( myErr.error != 0 )
						{
							if ( myErr.domain == kCFStreamErrorDomainPOSIX )
							{
							}
							else if ( myErr.domain == kCFStreamErrorDomainMacOSStatus )
							{
							}
							
							fprintf( stderr, "HTTP error %lu\n", myErr.error );
						}
					}
						
					CFReadStreamClose( myReadStream );
					CFRelease( myReadStream );
				}
					
				CFRelease( myRequest );
			}

			CFRelease( urlRef );
		}
	}
	
	return strData;
}

void DoSoftwareUpdateCheck()
{
	time_t timeNow = 0;
	CFMutableStringRef strData;

	time( &timeNow );

	if ( timeNow > g_timeLastUpdateCheck + SOFTWARE_UPDATE_CHECK_INTERVAL_DAYS * 60 * 60 * 24 )
	{
		//
		// Update the timestamp
		//

		g_timeLastUpdateCheck = timeNow;

		//
		// Store the value
		//

		CFNumberRef numPrefValue = CFNumberCreate( NULL, kCFNumberLongType, &g_timeLastUpdateCheck );
		if ( numPrefValue )
		{
			CFPreferencesSetAppValue( CFSTR( MY_REGISTRY_VALUE_UPDATE_TIMESTAMP ), numPrefValue, CFSTR( kBundleName ) );
			CFRelease( numPrefValue );
			
			CFPreferencesAppSynchronize( CFSTR( kBundleName ) );
		}		

		//
		// Download the RSS feed
		//
		
		strData = GetUrlContents( CFSTR( UPDATE_URL ), CFSTR("GET"), NULL );
		if ( strData )
		{			
			CFStringRef strVersionRss = GetXMLTagContents( strData, CFSTR("su:version") );
			if ( strVersionRss )
			{
				CFBundleRef theBundle = CFBundleGetBundleWithIdentifier( CFSTR( kBundleName ) );
				if ( theBundle )
				{
					CFStringRef strVersionMy = CFBundleGetValueForInfoDictionaryKey( theBundle, CFSTR("CFBundleShortVersionString") );
					if ( strVersionMy )
					{
						CFRetain( strVersionMy );

						if ( CFStringCompare( strVersionRss, strVersionMy, 0 ) > 0 )
						{
							DialogRef alertDialog;
							DialogItemIndex outItemHit;
							AlertStdCFStringAlertParamRec param;
								
							GetStandardAlertDefaultParams( &param, kStdCFStringAlertVersionOne );
							param.defaultText = CFSTR("Yes");
							param.cancelText = CFSTR("No");

							CreateStandardAlert( kAlertNoteAlert, CFSTR("A software update is available for the Now Playing plugin!\n\nWould you like to download it now?"), NULL, &param, &alertDialog );
							RunStandardAlert( alertDialog, NULL, &outItemHit );

							// Did they press Yes?
							if ( outItemHit == 1 )
							{
								LaunchURL( URL_DOWNLOAD_UPDATE );
							}
						}
					
						CFRelease( strVersionMy );
					}
				}
				
				CFRelease( strVersionRss );
			}
		
			CFRelease( strData );
		}
	}
}

void DoAppleLookup(ITUniStr255 artist, ITUniStr255 album, ITUniStr255 title, CFStringRef* strAppleURL)
{
	CFMutableStringRef strData;

	if ( g_intLogging == LOGGING_DEBUG )
	{
		fprintf( stderr, "Begin DoAppleLookup\n" );	
	}
	
	CFMutableStringRef strArtist = CFStringCreateMutable(kCFAllocatorDefault, 0);
	if (artist != NULL)
	{
		UrlEncodeAndAppend(strArtist, artist);
	}
	
	CFMutableStringRef strAlbum = CFStringCreateMutable(kCFAllocatorDefault, 0);
	if (album != NULL)
	{
		UrlEncodeAndAppend(strAlbum, album);
	}
	
	CFMutableStringRef strTitle = CFStringCreateMutable(kCFAllocatorDefault, 0);
	if (title != NULL)
	{
		UrlEncodeAndAppend(strTitle, title);
	}

	strData = CFStringCreateMutable(kCFAllocatorDefault, 0);
	if (strData)
	{
		CFStringRef strUrl = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("http://%s%s%@+%@+%@"), APPLE_LOOKUP_HOSTNAME, APPLE_LOOKUP_PATH, strTitle, strAlbum, strArtist);
		if (strUrl)
		{
			LOGDEBUG("Apple Lookup URL: ", strUrl);
			
			CFRange range;

			range.location = 0;
			range.length = CFStringGetLength( strUrl );
	
			CFMutableStringRef strUrl1 = CFStringCreateMutableCopy( kCFAllocatorDefault, 0, strUrl );
			
			CFStringFindAndReplace( strUrl1, CFSTR(" "), CFSTR("%20"), range, 0 );

			CFURLRef urlRef = CFURLCreateWithString( kCFAllocatorDefault, strUrl1, NULL );
			CFRelease( strUrl1 );			

			if ( urlRef )
			{			
				CFHTTPMessageRef myRequest = CFHTTPMessageCreateRequest( kCFAllocatorDefault, CFSTR("GET"), urlRef, kCFHTTPVersion1_1 );
				if ( myRequest )
				{
					CFReadStreamRef myReadStream = CFReadStreamCreateForHTTPRequest( kCFAllocatorDefault, myRequest );
					if ( myReadStream )
					{
/*					
						CFMutableDictionaryRef sslDict = CFDictionaryCreateMutable( kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks );
						if ( sslDict )
						{
							CFDictionaryAddValue( sslDict, kCFStreamSSLAllowsExpiredCertificates, kCFBooleanTrue );
							CFDictionaryAddValue( sslDict, kCFStreamSSLAllowsExpiredRoots, kCFBooleanTrue );
							CFDictionaryAddValue( sslDict, kCFStreamSSLAllowsAnyRoot, kCFBooleanTrue );

							CFReadStreamSetProperty( myReadStream, kCFStreamPropertySSLSettings, sslDict );

							CFRelease( sslDict );
						}						
*/
						if ( CFReadStreamOpen( myReadStream ) )
						{
							bool done = false;

							while ( !done )
							{
								UInt8 buf[1024];
								CFIndex bytesRead = CFReadStreamRead( myReadStream, buf, sizeof( buf ) );
								if ( bytesRead < 0 )
								{
									CFStreamError error = CFReadStreamGetError( myReadStream );
									fprintf( stderr, "Stream read error, domain = %d, error = %ld\n", error.domain, error.error );
									done = true;
								}
								else if ( bytesRead == 0 )
								{
									if ( CFReadStreamGetStatus( myReadStream ) == kCFStreamStatusAtEnd )
									{
										done = true;
									}
								}
								else
								{
									buf[bytesRead] = '\0';
									CFStringAppendCString( strData, (char *) buf, kCFStringEncodingUTF8 );
								}
							}
							
							CFHTTPMessageRef myResponse = CFReadStreamCopyProperty( myReadStream, kCFStreamPropertyHTTPResponseHeader );
							if ( myResponse )
							{
								//CFStringRef myStatusLine = CFHTTPMessageCopyResponseStatusLine( myResponse );
								UInt32 myErrCode = CFHTTPMessageGetResponseStatusCode( myResponse );

								fprintf( stderr, "HTTP Status Code: %lu\n", myErrCode );
							}
						}
						else
						{
							CFStreamError myErr = CFReadStreamGetError( myReadStream );
							if ( myErr.error != 0 )
							{
								if ( myErr.domain == kCFStreamErrorDomainPOSIX )
								{
								}
								else if ( myErr.domain == kCFStreamErrorDomainMacOSStatus )
								{
								}
								
								fprintf( stderr, "HTTP error %lu", myErr.error );
							}
						}
							
						CFReadStreamClose( myReadStream );
						CFRelease( myReadStream );
					}
						
					CFRelease( myRequest );
				}

				CFRelease( urlRef );
			}

			CFRelease( strUrl );
		}

		CFStringRef strAlbumId = GetRandomHTMLChunk(strData, CFSTR(MARKER_APPLE_ALBUM_START), CFSTR(MARKER_APPLE_ALBUM_STOP));
		CFStringRef strSongId = GetRandomHTMLChunk(strData, CFSTR(MARKER_APPLE_SONG_START), CFSTR(MARKER_APPLE_SONG_STOP));

		if (strAlbumId && strSongId)
		{
			char szBufAlbum[1024] = {0};
			CFStringGetCString( strAlbumId, &szBufAlbum[0], sizeof( szBufAlbum ), kCFStringEncodingUTF8 );

			char szBufSong[1024] = {0};
			CFStringGetCString( strSongId, &szBufSong[0], sizeof( szBufSong ), kCFStringEncodingUTF8 );

			*strAppleURL = CFStringCreateWithFormat( kCFAllocatorDefault, NULL, CFSTR(URL_APPLE_STORE), strlen( g_szAppleAssociate ) ? g_szAppleAssociate : DEFAULT_APPLE_ASSOCIATE, szBufSong, szBufAlbum );
			
			LOGDEBUG("Apple URL: ", *strAppleURL);
			
			DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_APPLE);
		}

		if ( strAlbumId )
		{
			CFRelease( strAlbumId );
		}

		if ( strSongId )
		{
			CFRelease( strSongId );
		}

		CFRelease( strData );
	}
	
	if (strTitle != NULL)
	{
		CFRelease(strTitle);
	}
	
	if (strArtist != NULL)
	{
		CFRelease(strArtist);
	}
	
	if (strAlbum != NULL)
	{
		CFRelease(strAlbum);
	}
	
	if ( g_intLogging == LOGGING_DEBUG )
	{
		fprintf( stderr, "End DoAppleLookup\n" );	
	}
}

void DoAmazonLookup(ITUniStr255 grouping, ITUniStr255 artist, ITUniStr255 album, CFStringRef* strDetailPageURL, CFStringRef* strSmallImageUrl, CFStringRef* strMediumImageUrl, CFStringRef* strLargeImageUrl)
{
	CFMutableStringRef strData;
	char szValue[MAX_PATH] = {0};
	
	if ( g_intLogging == LOGGING_DEBUG )
	{
		fprintf( stderr, "Begin DoAmazonLookup\n" );	
	}

	CFMutableStringRef strGrouping = CFStringCreateMutable(kCFAllocatorDefault, 0);
	if (grouping != NULL)
	{
		UrlEncodeAndAppend(strGrouping, grouping);
	}

	CFMutableStringRef strArtist = CFStringCreateMutable(kCFAllocatorDefault, 0);
	if (artist != NULL)
	{
		UrlEncodeAndAppend(strArtist, artist);
	}

	CFMutableStringRef strAlbum = CFStringCreateMutable(kCFAllocatorDefault, 0);
	if (album != NULL)
	{
		UrlEncodeAndAppend(strAlbum, album);
	}

	CFStringRef strDomain = NULL;

	if ( g_intAmazonLocale == 1 )
	{
		strDomain = CFSTR("ca");
	}
	else if ( g_intAmazonLocale == 2 )
	{
		strDomain = CFSTR("de");
	}
	else if ( g_intAmazonLocale == 3 )
	{
		strDomain = CFSTR("fr");
	}
	else if ( g_intAmazonLocale == 4 )
	{
		strDomain = CFSTR("co.jp");
	}
	else if ( g_intAmazonLocale == 6 )
	{
		strDomain = CFSTR("co.uk");
	}
	else
	{
		strDomain = CFSTR("com");
	}

	strData = CFStringCreateMutable( kCFAllocatorDefault, 0 );
	if ( strData )
	{
		CFStringRef strUrl;
		
		time_t now_t = time( NULL );
		struct tm now;
		now = *gmtime( &now_t );			
		char szTimestamp[MAX_PATH] = {0};			
		sprintf( szTimestamp, TIMEZONE_FORMAT_GMT_AMAZON, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec );		
		
		if (g_intAmazonUseASIN > 0 && CFStringGetLength(strGrouping) == 10)
		{
			strUrl = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("http://webservices.amazon.%@/onca/xml?AWSAccessKeyId=%s&AssociateTag=%s&ItemId=%@&Operation=ItemLookup&ResponseGroup=Medium&Service=AWSECommerceService&Timestamp=%s"), strDomain, MY_AMAZON_ACCESS_KEY_ID, g_szAmazonAssociate, strGrouping, szTimestamp);
		}
		else
		{
			strUrl = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("http://webservices.amazon.%@/onca/xml?AWSAccessKeyId=%s&Artist=%@&AssociateTag=%s&Operation=ItemSearch&ResponseGroup=Medium&SearchIndex=Music&Service=AWSECommerceService&Timestamp=%s&Title=%@"), strDomain, MY_AMAZON_ACCESS_KEY_ID, strArtist, g_szAmazonAssociate, szTimestamp, strAlbum);
		}
		
		if ( strUrl )
		{		
			CFRange range;

			range.location = 0;
			range.length = CFStringGetLength( strUrl );
	
			CFMutableStringRef strUrl1 = CFStringCreateMutableCopy( kCFAllocatorDefault, 0, strUrl );
			
			CFStringFindAndReplace( strUrl1, CFSTR(" "), CFSTR("%20"), range, 0 );
			
			// Get the parameter string
			CFRange rangeFind = CFStringFind( strUrl, CFSTR("?"), 0 );
			rangeFind.location++;
			rangeFind.length = CFStringGetLength( strUrl ) - rangeFind.location;
								
			CFStringRef strParameters = CFStringCreateWithSubstring( kCFAllocatorDefault, strUrl1, rangeFind );
	
			CFMutableStringRef strSigData = CFStringCreateMutable( kCFAllocatorDefault, 0 );
			CFStringAppend( strSigData, CFSTR("GET\nwebservices.amazon.") );
			CFStringAppend( strSigData, strDomain );
			CFStringAppend( strSigData, CFSTR("\n/onca/xml\n") );
			CFStringAppend( strSigData, strParameters );
			
			// Calculate the signature
			char szSignature[MAX_PATH] = {0};
			char szSigData[1024 * 2] = {0};
			CFStringGetCString( strSigData, &szSigData[0], sizeof( szSigData ), kCFStringEncodingUTF8 );
			OAuthSignHMACSHA256( szSigData, MY_AMAZON_SECRET_KEY, szSignature, sizeof( szSignature ) );
				
			// Add the signature to the post data
			CFStringAppend( strUrl1, CFSTR("&Signature=") );
			UrlEncode( &szValue[0], sizeof( szValue ), szSignature );
			CFStringAppendCString( strUrl1, szValue, kCFStringEncodingUTF8 );

			LOGDEBUG("Amazon URL: ", strUrl1);
			
			CFURLRef urlRef = CFURLCreateWithString( kCFAllocatorDefault, strUrl1, NULL );
			CFRelease( strUrl1 );
			
			CFRelease(strParameters);
			CFRelease(strSigData);

			if ( urlRef )
			{			
				CFHTTPMessageRef myRequest = CFHTTPMessageCreateRequest( kCFAllocatorDefault, CFSTR("GET"), urlRef, kCFHTTPVersion1_1 );
				if ( myRequest )
				{
					CFReadStreamRef myReadStream = CFReadStreamCreateForHTTPRequest( kCFAllocatorDefault, myRequest );
					if ( myReadStream )
					{
						if ( CFReadStreamOpen( myReadStream ) )
						{
							bool done = false;

							while ( !done )
							{
								UInt8 buf[1024];
								CFIndex bytesRead = CFReadStreamRead( myReadStream, buf, sizeof( buf ) );
								if ( bytesRead < 0 )
								{
									CFStreamError error = CFReadStreamGetError( myReadStream );
									fprintf( stderr, "Stream read error, domain = %d, error = %ld\n", error.domain, error.error );
									done = true;
								}
								else if ( bytesRead == 0 )
								{
									if ( CFReadStreamGetStatus( myReadStream ) == kCFStreamStatusAtEnd )
									{
										done = true;
									}
								}
								else
								{
									buf[bytesRead] = '\0';
									CFStringAppendCString( strData, buf, kCFStringEncodingUTF8 );
								}
							}
							
							CFHTTPMessageRef myResponse = CFReadStreamCopyProperty( myReadStream, kCFStreamPropertyHTTPResponseHeader );
							if ( myResponse )
							{
								//CFStringRef myStatusLine = CFHTTPMessageCopyResponseStatusLine( myResponse );
								UInt32 myErrCode = CFHTTPMessageGetResponseStatusCode( myResponse );

								fprintf( stderr, "HTTP Status Code: %lu\n", myErrCode );
							}
						}
						else
						{
							CFStreamError myErr = CFReadStreamGetError( myReadStream );
							if ( myErr.error != 0 )
							{
								if ( myErr.domain == kCFStreamErrorDomainPOSIX )
								{
								}
								else if ( myErr.domain == kCFStreamErrorDomainMacOSStatus )
								{
									//OSStatus macError = (OSStatus) myErr.error;
								}
							}
						}
							
						CFReadStreamClose( myReadStream );
						CFRelease( myReadStream );
					}
						
					CFRelease( myRequest );
				}

				CFRelease( urlRef );
			}

			CFRelease( strUrl );
		}
		
		bool output = false;

		CFStringRef strSmallImage = GetXMLTagContents( strData, CFSTR("SmallImage") );
		if ( strSmallImage )
		{
			*strSmallImageUrl = GetXMLTagContents( strSmallImage, CFSTR("URL") );

			CFRelease( strSmallImage );
		}
		else
		{
			output = true;
		}

		CFStringRef strMediumImage = GetXMLTagContents( strData, CFSTR("MediumImage") );
		if ( strMediumImage )
		{
			*strMediumImageUrl = GetXMLTagContents( strMediumImage, CFSTR("URL") );

			CFRelease( strMediumImage );
		}
		else
		{
			output = true;
		}
		
		CFStringRef strLargeImage = GetXMLTagContents( strData, CFSTR("LargeImage") );
		if ( strLargeImage )
		{
			*strLargeImageUrl = GetXMLTagContents( strLargeImage, CFSTR("URL") );

			CFRelease( strLargeImage );

			DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_AMAZON);
		}
		else
		{
			output = true;
		}
		
		*strDetailPageURL = GetXMLTagContents( strData, CFSTR("DetailPageURL") );

		if (output)
		{
			LOGDEBUG("Response = ", strData);
		}

		CFRelease( strData );
	}

	if (strGrouping != NULL)
	{
		CFRelease(strGrouping);
	}

	if (strArtist != NULL)
	{
		CFRelease(strArtist);
	}

	if (strAlbum != NULL)
	{
		CFRelease(strAlbum);
	}
	
	if ( g_intLogging == LOGGING_DEBUG )
	{
		fprintf( stderr, "End DoAmazonLookup\n" );
	}
}

void DoTrackInfo(FILE* f, int i, int intOrder)
{	
	if ( g_intPlaylistLength > 1 )
	{
		fprintf( f, "\t<song order=\"%d\">\n", intOrder );
	}
	else
	{
		fprintf( f, "\t<song>\n" );
	}
	
	PrintTrackInfoStringToFile( f, "title", g_paPlaylist[i].track.name, g_intUseXmlCData > 0 ? true : false );
	PrintTrackInfoStringToFile( f, "artist", g_paPlaylist[i].track.artist, g_intUseXmlCData > 0 ? true : false );
	PrintTrackInfoStringToFile( f, "album", g_paPlaylist[i].track.album, g_intUseXmlCData > 0 ? true : false );
	PrintTrackInfoStringToFile( f, "genre", g_paPlaylist[i].track.genre, g_intUseXmlCData > 0 ? true : false );
	PrintTrackInfoStringToFile( f, "kind", g_paPlaylist[i].track.kind, false );
	PrintTrackInfoNumberToFile( f, "track", g_paPlaylist[i].track.trackNumber );
	PrintTrackInfoNumberToFile( f, "numTracks", g_paPlaylist[i].track.numTracks );
	PrintTrackInfoNumberToFile( f, "year", g_paPlaylist[i].track.year );
	PrintTrackInfoStringToFile( f, "comments", g_paPlaylist[i].track.comments, g_intUseXmlCData > 0 ? true : false  );
	PrintTrackInfoNumberToFile( f, "time", g_paPlaylist[i].track.totalTimeInMS / 1000 );
	PrintTrackInfoNumberToFile( f, "bitrate", g_paPlaylist[i].track.bitRate );
	PrintTrackInfoNumberToFile( f, "rating", g_paPlaylist[i].track.trackRating / 20 );
	PrintTrackInfoNumberToFile( f, "disc", g_paPlaylist[i].track.discNumber );
	PrintTrackInfoNumberToFile( f, "numDiscs", g_paPlaylist[i].track.numDiscs );
	PrintTrackInfoNumberToFile( f, "playCount", g_paPlaylist[i].track.playCount );
	PrintTrackInfoCFStringToFile( f, TAG_COMPILATION, g_paPlaylist[i].track.isCompilationTrack == TRUE ? CFSTR("Yes") : CFSTR("No"), false );
	PrintTrackInfoCFStringToFile( f, "urlAmazon", g_paPlaylist[i].strDetailPageURL, false );
	PrintTrackInfoCFStringToFile( f, "urlApple", g_paPlaylist[i].strAppleURL, false );
	PrintTrackInfoCFStringToFile( f, "imageSmall", g_paPlaylist[i].strSmallImageUrl, false );
	PrintTrackInfoCFStringToFile( f, "image", g_paPlaylist[i].strMediumImageUrl, false );
	PrintTrackInfoCFStringToFile( f, "imageLarge", g_paPlaylist[i].strLargeImageUrl, false );
	PrintTrackInfoStringToFile( f, "composer", g_paPlaylist[i].track.composer, g_intUseXmlCData > 0 ? true : false  );
	PrintTrackInfoStringToFile( f, "grouping", g_paPlaylist[i].track.grouping, g_intUseXmlCData > 0 ? true : false  );
	PrintTrackInfoStringToFile( f, "file", g_paPlaylist[i].track.fileName, g_intUseXmlCData > 0 ? true : false  );
	PrintTrackInfoCFStringToFile( f, "artworkID", g_paPlaylist[i].strArtworkId, false );

	fprintf( f, "\t</song>\n" );
}

void WriteXML(bool fPlay, int intNext)
{
	FILE* f = NULL;
	int i = 0;
	int intOrder = 1;

	if ( g_intLogging == LOGGING_DEBUG )
	{
		fprintf( stderr, "Begin WriteXML\n" );
	}

	f = fopen( g_szOutputFile, "w" );
	if ( f )
	{
		if ( GetSelectedEncoding() == kCFStringEncodingUTF8 )		
		{
			fprintf( f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n" );
		}
		else
		{
			fprintf( f, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\n" );
		}

		if ( fPlay )
		{
			time_t now_t = time( NULL );
			struct tm now;
			now = *gmtime( &now_t );			
			char szTimestamp[MAX_PATH] = {0};			
			sprintf( szTimestamp, TIMEZONE_FORMAT_GMT, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec );

			fprintf( f, "<now_playing playing=\"1\" timestamp=\"%s\">\n", szTimestamp );

			if ( g_intPlaylistLength > 1 )
			{
				for ( i = intNext; i >= 0; i-- )
				{
					if ( g_paPlaylist && g_paPlaylist[ i ].fSet )
					{
						DoTrackInfo( f, i, intOrder );
						intOrder++;
					}
				}
				
				for ( i = g_intPlaylistLength - 1; i > intNext; i-- )
				{
					if ( g_paPlaylist && g_paPlaylist[ i ].fSet )
					{
						DoTrackInfo( f, i, intOrder );
						intOrder++;
					}
				}
			}
			else if ( g_paPlaylist && g_paPlaylist[ i ].fSet )
			{
				DoTrackInfo( f, i, intOrder );
			}
			
		}
		else
		{
			fprintf( f, "<now_playing playing=\"0\">\n" );
		}

		fprintf( f, "</now_playing>\n" );

		fclose( f );
	}
	
	if ( g_intLogging == LOGGING_DEBUG )
	{
		fprintf( stderr, "End WriteXML\n" );
	}
}

void MyDeleteFile(const char * lpszFile)
{
	FSRef fileRef;

	if ( FSPathMakeRef( (const unsigned char *) lpszFile, &fileRef, false ) == noErr )
	{
		if ( FSDeleteObject( &fileRef ) == noErr )
		{
			fprintf( stderr, "Deleted file = %s\n", lpszFile );
		}
	}
}

OSStatus script_exportArtwork(LoadedScriptInfoPtr scriptInfo, AEDesc* theResult)
{
	OSStatus err = noErr;
    AEDescList theParameters;
	AEDesc theMessage;
	AEDesc theDir;
	
	if ( g_intLogging == LOGGING_DEBUG )
	{
		fprintf( stderr, "Begin exportArtwork (%d)\n", g_intArtworkWidth );	
	}

	err = LongToAEDesc( g_intArtworkWidth, &theMessage );
	if ( noErr == err )
	{
		char szPath[MAX_PATH] = {0};

		strcpy( szPath, g_szOutputFile );
		MyPathRemoveFileSpec( szPath );
		
		CFStringRef cfOutputDir = CFStringCreateWithCString( kCFAllocatorDefault, szPath, 0 );
		if ( cfOutputDir )
		{
			err = CFStringToAEDesc( cfOutputDir, &theDir );
			if ( noErr == err )
			{
				err = AEBuildDesc( &theParameters, NULL, "[@,@]", &theMessage, &theDir );
				if ( noErr == err )
				{
					err = CallScriptSubroutine( scriptInfo, "export_artwork", &theParameters, theResult, NULL );

					AEDisposeDesc( &theParameters );
				}
				
				AEDisposeDesc( &theDir );
			}
			
			CFRelease( cfOutputDir );
		}

		AEDisposeDesc( &theMessage );
	}

	if ( g_intLogging == LOGGING_DEBUG )
	{
		fprintf( stderr, "End exportArtwork\n" );
	}
	
	return err;
}

void PrintTrackInfoStringToFile(FILE* f, char* lpszNodeName, ITUniStr255 NodeValue, bool fUseCData)
{
	if ( f && lpszNodeName && NodeValue )
	{
		//
		// Read it into a CFString using the encoding we were given so we can output as UTF-8
		//
		
		CFStringRef strNodeValue = CFStringCreateWithBytes(kCFAllocatorDefault, (UInt8 *) &NodeValue[1], NodeValue[0] * sizeof(UniChar), kCFStringEncodingUnicode, false);
		if ( strNodeValue )
		{
			char szBuffer[1024] = {0};

			if ( CFStringGetCString( strNodeValue, &szBuffer[0], sizeof( szBuffer ), GetSelectedEncoding() ) )
			{
				if ( fUseCData )
				{
					fprintf( f, "\t\t<%s><![CDATA[%s]]></%s>\n", lpszNodeName, &szBuffer[0], lpszNodeName );
					if ( g_intLogging == LOGGING_DEBUG )
					{
						fprintf( stderr, "<%s><![CDATA[%s]]></%s>\n", lpszNodeName, &szBuffer[0], lpszNodeName );
					}

				}
				else
				{
					fprintf( f, "\t\t<%s>%s</%s>\n", lpszNodeName, &szBuffer[0], lpszNodeName );
					if ( g_intLogging == LOGGING_DEBUG )
					{
						fprintf( stderr, "<%s>%s</%s>\n", lpszNodeName, &szBuffer[0], lpszNodeName );
					}
				}
			}
			
			CFRelease( strNodeValue );
		}
	}
}

char * CFStringToCString(CFStringRef input, CFStringEncoding encoding)
{
	if (input)
	{
		CFIndex size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(input), encoding) + 1;	
		char * output = NewPtrClear(size);
		if (output)
		{		
			if (CFStringGetCString(input, output, size, encoding))
			{
				return output;
			}
		}
	}

	return NULL;
}

CFStringRef ITUniStr255ToCFString(ITUniStr255 value)
{
	return CFStringCreateWithBytes(kCFAllocatorDefault, (UInt8 *) &value[1], value[0] * sizeof(UniChar), kCFStringEncodingUnicode, false);
}

void PrintTrackInfoCFStringToFile(FILE* f, char* lpszNodeName, CFStringRef strValue, bool fUseCData)
{
	if ( f && lpszNodeName )
	{
		if ( strValue )
		{
			char szBuffer[1024] = {0};

			if ( CFStringGetCString( strValue, &szBuffer[0], sizeof( szBuffer ), GetSelectedEncoding() ) )
			{
				if ( fUseCData )
				{
					fprintf( f, "\t\t<%s><![CDATA[%s]]></%s>\n", lpszNodeName, &szBuffer[0], lpszNodeName );
				}
				else
				{
					fprintf( f, "\t\t<%s>%s</%s>\n", lpszNodeName, &szBuffer[0], lpszNodeName );
				}
			}
		}
		else
		{
			fprintf( f, "\t\t<%s/>\n", lpszNodeName );
		}
	}		
}

void PrintTrackInfoNumberToFile(FILE * f, const char * lpszNodeName, int intNodeValue)
{
	if ( f && lpszNodeName )
	{                       
		if ( intNodeValue > 0 ) 
		{             
			fprintf( f, "\t\t<%s>%d</%s>\n", lpszNodeName, intNodeValue, lpszNodeName );
		}
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

void DoPing(bool fPlay, int intNext)
{
	if ( g_intLogging == LOGGING_DEBUG )
	{
		fprintf( stderr, "Begin DoPing\n" );
	}

	if ( strlen( &g_szTrackBackUrl[0] ) > 0 )
	{
		//
		// Create post data
		//

		CFMutableStringRef strData = CFStringCreateMutable( kCFAllocatorDefault, 0 );
		if ( strData )
		{
			if (fPlay)
			{
				char szValue[1024] = {0};
				char szValueEncoded[1024] = {0};
				CFStringRef strValue = NULL;

				CFStringAppend(strData, CFSTR(TAG_TITLE));
				CFStringAppend(strData, CFSTR("="));
				UrlEncodeAndAppend(strData, g_paPlaylist[intNext].track.name);

				CFStringAppend(strData, CFSTR("&"));
				CFStringAppend(strData, CFSTR(TAG_ARTIST));
				CFStringAppend(strData, CFSTR("="));
				UrlEncodeAndAppend(strData, g_paPlaylist[intNext].track.artist);

				CFStringAppend(strData, CFSTR("&"));
				CFStringAppend(strData, CFSTR(TAG_ALBUM));
				CFStringAppend(strData, CFSTR("="));
				UrlEncodeAndAppend(strData, g_paPlaylist[intNext].track.album);

				CFStringAppend(strData, CFSTR("&"));
				CFStringAppend(strData, CFSTR(TAG_GENRE));
				CFStringAppend(strData, CFSTR("="));
				UrlEncodeAndAppend(strData, g_paPlaylist[intNext].track.genre);

				CFStringAppend(strData, CFSTR("&"));
				CFStringAppend(strData, CFSTR(TAG_KIND));
				CFStringAppend(strData, CFSTR("="));
				UrlEncodeAndAppend(strData, g_paPlaylist[intNext].track.kind);

				CFStringAppend( strData, CFSTR("&") );
				CFStringAppend( strData, CFSTR(TAG_TRACK) );
				CFStringAppend( strData, CFSTR("=") );
				strValue = CFStringCreateWithFormat( NULL, NULL, CFSTR("%d"), g_paPlaylist[intNext].track.trackNumber );
				if ( strValue )
				{
					CFStringAppend( strData, strValue );
					CFRelease( strValue );
				}
		
				CFStringAppend( strData, CFSTR("&") );
				CFStringAppend( strData, CFSTR(TAG_NUMTRACKS) );
				CFStringAppend( strData, CFSTR("=") );
				strValue = CFStringCreateWithFormat( NULL, NULL, CFSTR("%d"), g_paPlaylist[intNext].track.numTracks );
				if ( strValue )
				{
					CFStringAppend( strData, strValue );
					CFRelease( strValue );
				}

				CFStringAppend( strData, CFSTR("&") );
				CFStringAppend( strData, CFSTR(TAG_YEAR) );
				CFStringAppend( strData, CFSTR("=") );
				strValue = CFStringCreateWithFormat( NULL, NULL, CFSTR("%d"), g_paPlaylist[intNext].track.year );
				if ( strValue )
				{
					CFStringAppend( strData, strValue );
					CFRelease( strValue );
				}

				CFStringAppend(strData, CFSTR("&"));
				CFStringAppend(strData, CFSTR(TAG_COMMENTS));
				CFStringAppend(strData, CFSTR("="));
				UrlEncodeAndAppend(strData, g_paPlaylist[intNext].track.comments);

				CFStringAppend( strData, CFSTR("&") );
				CFStringAppend( strData, CFSTR(TAG_TIME) );
				CFStringAppend( strData, CFSTR("=") );
				strValue = CFStringCreateWithFormat( NULL, NULL, CFSTR("%d"), g_paPlaylist[intNext].track.totalTimeInMS / 1000 );
				if ( strValue )
				{
					CFStringAppend( strData, strValue );
					CFRelease( strValue );
				}

				CFStringAppend( strData, CFSTR("&") );
				CFStringAppend( strData, CFSTR(TAG_BITRATE) );
				CFStringAppend( strData, CFSTR("=") );
				strValue = CFStringCreateWithFormat( NULL, NULL, CFSTR("%d"), g_paPlaylist[intNext].track.bitRate );
				if ( strValue )
				{
					CFStringAppend( strData, strValue );
					CFRelease( strValue );
				}

				CFStringAppend( strData, CFSTR("&") );
				CFStringAppend( strData, CFSTR(TAG_RATING) );
				CFStringAppend( strData, CFSTR("=") );
				strValue = CFStringCreateWithFormat( NULL, NULL, CFSTR("%d"), g_paPlaylist[intNext].track.trackRating / 20 );
				if ( strValue )
				{
					CFStringAppend( strData, strValue );
					CFRelease( strValue );
				}

				CFStringAppend( strData, CFSTR("&") );
				CFStringAppend( strData, CFSTR(TAG_DISC) );
				CFStringAppend( strData, CFSTR("=") );
				strValue = CFStringCreateWithFormat( NULL, NULL, CFSTR("%d"), g_paPlaylist[intNext].track.discNumber );
				if ( strValue )
				{
					CFStringAppend( strData, strValue );
					CFRelease( strValue );
				}

				CFStringAppend( strData, CFSTR("&") );
				CFStringAppend( strData, CFSTR(TAG_NUMDISCS) );
				CFStringAppend( strData, CFSTR("=") );
				strValue = CFStringCreateWithFormat( NULL, NULL, CFSTR("%d"), g_paPlaylist[intNext].track.numDiscs );
				if ( strValue )
				{
					CFStringAppend( strData, strValue );
					CFRelease( strValue );
				}

				CFStringAppend( strData, CFSTR("&") );
				CFStringAppend( strData, CFSTR(TAG_PLAYCOUNT) );
				CFStringAppend( strData, CFSTR("=") );
				strValue = CFStringCreateWithFormat( NULL, NULL, CFSTR("%d"), g_paPlaylist[intNext].track.playCount );
				if ( strValue )
				{
					CFStringAppend( strData, strValue );
					CFRelease( strValue );
				}
				
				CFStringAppend( strData, CFSTR("&") );
				CFStringAppend( strData, CFSTR(TAG_COMPILATION) );
				CFStringAppend( strData, CFSTR("=") );
				CFStringAppend( strData, g_paPlaylist[ intNext ].track.isCompilationTrack == TRUE ? CFSTR("Yes") : CFSTR("No") );			
							   
				CFStringAppend(strData, CFSTR("&"));
				CFStringAppend(strData, CFSTR(TAG_COMPOSER));
				CFStringAppend(strData, CFSTR("="));
				UrlEncodeAndAppend(strData, g_paPlaylist[intNext].track.composer);

				CFStringAppend(strData, CFSTR("&"));
				CFStringAppend(strData, CFSTR(TAG_GROUPING));
				CFStringAppend(strData, CFSTR("="));
				UrlEncodeAndAppend(strData, g_paPlaylist[intNext].track.grouping);

				CFStringAppend(strData, CFSTR("&"));
				CFStringAppend(strData, CFSTR(TAG_FILE));
				CFStringAppend(strData, CFSTR("="));
				UrlEncodeAndAppend(strData, g_paPlaylist[intNext].track.fileName);
							   
				if ( g_intAmazonLookup )
				{
					CFStringAppend( strData, CFSTR("&") );
					CFStringAppend( strData, CFSTR(TAG_URLAMAZON) );
					CFStringAppend( strData, CFSTR("=") );
					if ( g_paPlaylist[intNext].strDetailPageURL )
					{
						if ( CFStringGetCString( g_paPlaylist[intNext].strDetailPageURL, &szValue[0], sizeof( szValue ), 0 ) )
						{
							UrlEncode( &szValueEncoded[0], sizeof( szValueEncoded ), &szValue[0] );
							CFStringAppendCString( strData, szValueEncoded, kCFStringEncodingUTF8 );
						}
					}
				}

				if ( g_intAppleLookup )
				{
					CFStringAppend( strData, CFSTR("&") );
					CFStringAppend( strData, CFSTR(TAG_URLAPPLE) );
					CFStringAppend( strData, CFSTR("=") );
					if ( g_paPlaylist[intNext].strAppleURL )
					{
						if ( CFStringGetCString( g_paPlaylist[intNext].strAppleURL, &szValue[0], sizeof( szValue ), 0 ) )
						{
							UrlEncode( &szValueEncoded[0], sizeof( szValueEncoded ), &szValue[0] );
							CFStringAppendCString( strData, szValueEncoded, kCFStringEncodingUTF8 );
						}
					}
				}

				if ( g_intAmazonLookup )
				{
					CFStringAppend( strData, CFSTR("&") );
					CFStringAppend( strData, CFSTR(TAG_IMAGESMALL) );
					CFStringAppend( strData, CFSTR("=") );
					if ( g_paPlaylist[intNext].strSmallImageUrl )
					{
						if ( CFStringGetCString( g_paPlaylist[intNext].strSmallImageUrl, &szValue[0], sizeof( szValue ), 0 ) )
						{
							UrlEncode( &szValueEncoded[0], sizeof( szValueEncoded ), &szValue[0] );
							CFStringAppendCString( strData, szValueEncoded, kCFStringEncodingUTF8 );
						}
					}

					CFStringAppend( strData, CFSTR("&") );
					CFStringAppend( strData, CFSTR(TAG_IMAGE) );
					CFStringAppend( strData, CFSTR("=") );
					if ( g_paPlaylist[intNext].strMediumImageUrl )
					{
						if ( CFStringGetCString( g_paPlaylist[intNext].strMediumImageUrl, &szValue[0], sizeof( szValue ), 0 ) )
						{
							UrlEncode( &szValueEncoded[0], sizeof( szValueEncoded ), &szValue[0] );
							CFStringAppendCString( strData, szValueEncoded, kCFStringEncodingUTF8 );
						}
					}
					
					CFStringAppend( strData, CFSTR("&") );
					CFStringAppend( strData, CFSTR(TAG_IMAGELARGE) );
					CFStringAppend( strData, CFSTR("=") );
					if ( g_paPlaylist[intNext].strLargeImageUrl )
					{
						if ( CFStringGetCString( g_paPlaylist[intNext].strLargeImageUrl, &szValue[0], sizeof( szValue ), 0 ) )
						{
							UrlEncode( &szValueEncoded[0], sizeof( szValueEncoded ), &szValue[0] );
							CFStringAppendCString( strData, szValueEncoded, kCFStringEncodingUTF8 );
						}
					}
				}

				CFStringAppend( strData, CFSTR("&") );
				CFStringAppend( strData, CFSTR(TAG_FILE) );
				CFStringAppend( strData, CFSTR("=") );
				UrlEncode( &szValue[0], sizeof( szValue ), (char *) &g_paPlaylist[intNext].track.fileName[1] );
				CFStringAppendCString( strData, szValue, kCFStringEncodingUTF8 );			

				CFStringAppend( strData, CFSTR("&") );
				CFStringAppend( strData, CFSTR(TAG_ARTWORKID) );
				CFStringAppend( strData, CFSTR("=") );
				if ( g_paPlaylist[intNext].strArtworkId )
				{
					CFStringAppend( strData, g_paPlaylist[intNext].strArtworkId );
				}
			}

			//
			// Ping
			//

			CFStringRef strUrl = CFStringCreateWithCString( kCFAllocatorDefault, &g_szTrackBackUrl[0], 0 );
			if ( strUrl )
			{
				CFURLRef urlRef = CFURLCreateWithString( kCFAllocatorDefault, strUrl, NULL );
				if ( urlRef )
				{
					CFIndex lenData = CFStringGetLength( strData );					
					UInt8* buffer = CFAllocatorAllocate( kCFAllocatorDefault, fPlay ? lenData : 1, 0 );
					if ( buffer )
					{
						CFStringGetBytes( strData, CFRangeMake( 0, lenData ), kCFStringEncodingASCII, 0, FALSE, buffer, lenData, NULL );
	 
						CFDataRef postData = CFDataCreate( kCFAllocatorDefault, buffer, lenData );
						if ( postData )
						{
							CFHTTPMessageRef myRequest = CFHTTPMessageCreateRequest( kCFAllocatorDefault, CFSTR("POST"), urlRef, kCFHTTPVersion1_1 );
							if ( myRequest )
							{
								CFHTTPMessageSetHeaderFieldValue( myRequest, CFSTR("Content-Type"), CFSTR("application/x-www-form-urlencoded; charset=UTF-8") );
								
								//
								// Set the custom header if extra info is present
								//

								if ( strlen( &g_szPingExtraInfo[0] ) > 0 )
								{
									CFStringRef strPingExtraInfo = CFStringCreateWithCString( kCFAllocatorDefault, &g_szPingExtraInfo[0], 0 );
									if ( strPingExtraInfo )
									{
										CFHTTPMessageSetHeaderFieldValue( myRequest, CFSTR("X-NowPlaying"), strPingExtraInfo );
										CFRelease( strPingExtraInfo );
									}
								}
								
								CFHTTPMessageSetBody( myRequest, postData );

								CFReadStreamRef myReadStream = CFReadStreamCreateForHTTPRequest( kCFAllocatorDefault, myRequest );
								if ( myReadStream )
								{
									if ( CFReadStreamOpen( myReadStream ) )
									{
										bool done = false;

										while ( !done )
										{
											UInt8 buf[1024];
											CFIndex bytesRead = CFReadStreamRead( myReadStream, buf, sizeof( buf ) );
											if ( bytesRead < 0 )
											{
												CFStreamError error = CFReadStreamGetError( myReadStream );
												fprintf( stderr, "Stream read error, domain = %d, error = %ld\n", error.domain, error.error );
												done = true;
											}
											else if ( bytesRead == 0 )
											{
												if ( CFReadStreamGetStatus( myReadStream ) == kCFStreamStatusAtEnd )
												{
													done = true;
												}
											}
											else
											{
											}
										}
									
										CFHTTPMessageRef myResponse = CFReadStreamCopyProperty( myReadStream, kCFStreamPropertyHTTPResponseHeader );
										if ( myResponse )
										{
											//CFStringRef myStatusLine = CFHTTPMessageCopyResponseStatusLine( myResponse );
											UInt32 myErrCode = CFHTTPMessageGetResponseStatusCode( myResponse );
											
											fprintf( stderr, "HTTP Status Code: %lu\n", myErrCode );
										}
										
										DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_PING);
									}
									else
									{
										CFStreamError myErr = CFReadStreamGetError( myReadStream );
										if ( myErr.error != 0 )
										{
											if ( myErr.domain == kCFStreamErrorDomainPOSIX )
											{
											}
											else if ( myErr.domain == kCFStreamErrorDomainMacOSStatus )
											{
												//OSStatus macError = (OSStatus) myErr.error;
											}
										}
									}
									
									CFReadStreamClose( myReadStream );
									CFRelease( myReadStream );
								}
								
								CFRelease( myRequest );
							}

							CFRelease( postData );
						}

						CFAllocatorDeallocate( NULL, buffer );
					}

					CFRelease( urlRef );
				}

				CFRelease( strUrl );
			}

			CFRelease( strData );
		}
	}

	if ( g_intLogging == LOGGING_DEBUG )
	{	
		fprintf( stderr, "End DoPing\n" );
	}
}

void SubBlock(CFMutableStringRef strMessage, CFStringRef strTagStart, CFStringRef strTagStop, bool show)
{
	if (strMessage && strTagStart && strTagStop)
	{
		bool found = true;

		do
		{
			CFRange range;
			range.location = 0;
			range.length = CFStringGetLength(strMessage);

			CFRange rangeStartTag;
			if (CFStringFindWithOptions(strMessage, strTagStart, range, 0, &rangeStartTag))
			{
				CFRange rangeEndFind;
				rangeEndFind.location = rangeStartTag.location + rangeStartTag.length;
				rangeEndFind.length = CFStringGetLength(strMessage) - rangeStartTag.location;

				CFRange rangeStopTag;
				if (CFStringFindWithOptions(strMessage, strTagStop, rangeEndFind, 0, &rangeStopTag))
				{
					if (show)
					{
						CFStringDelete(strMessage, rangeStopTag);
						CFStringDelete(strMessage, rangeStartTag);
					}
					else
					{
						CFRange rangeAll;
						rangeAll.location = rangeStartTag.location;
						rangeAll.length = rangeStopTag.location - rangeStartTag.location + rangeStopTag.length;

						CFStringDelete(strMessage, rangeAll);
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

void SubParameterCFString(CFMutableStringRef strMessage, CFStringRef strTag, CFStringRef strValue)
{
	if (strMessage)
	{
		CFRange range;
		range.location = 0;
		range.length = CFStringGetLength(strMessage);

		CFStringFindAndReplace(strMessage, strTag, strValue ? strValue : CFSTR(""), range, 0);
	}
}

void SubParameterString(CFMutableStringRef strMessage, CFStringRef strTag, ITUniStr255 value)
{
	if (strMessage)
	{
		CFStringRef strValue = CFStringCreateWithBytes(kCFAllocatorDefault, (UInt8 *) &value[1], value[0] * sizeof(UniChar), kCFStringEncodingUnicode, false);
		if (strValue)
		{
			CFRange range;
			range.location = 0;
			range.length = CFStringGetLength(strMessage);
		
			CFStringFindAndReplace(strMessage, strTag, strValue, range, 0);

			CFRelease(strValue);
		}
	}
}

void SubParameterNumber(CFMutableStringRef strMessage, CFStringRef strTag, CFStringRef strFormat, int value)
{
	if (strMessage)
	{
		CFStringRef strValue = CFStringCreateWithFormat(NULL, NULL, strFormat, value);
		if (strValue)
		{
			CFRange range;
			range.location = 0;
			range.length = CFStringGetLength(strMessage);
		
			CFStringFindAndReplace(strMessage, strTag, strValue, range, 0);

			CFRelease(strValue);
		}
	}
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

			if (g_intLogging == LOGGING_DEBUG)
			{
				fprintf(stderr, "Found \"%s\" with value of \"%s\"\n", name, *value);
			}
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
	strcpy(g_szTwitterAuthKey, "");
	strcpy(g_szTwitterAuthSecret, "");
	g_timetTwitterLatest = 0;
	
	CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_TWITTER_KEY), CFSTR(""), CFSTR(kBundleName));											
	CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_TWITTER_SECRET), CFSTR(""), CFSTR(kBundleName));
	CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_TWITTER_SCREENNAME), CFSTR(""), CFSTR(kBundleName));
	
	CFNumberRef numPrefValue = CFNumberCreate(NULL, kCFNumberLongType, &g_timetTwitterLatest);
	if (numPrefValue != NULL)
	{
		CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_TWITTER_LATEST), numPrefValue, CFSTR(kBundleName));
		CFRelease(numPrefValue);
	}

	CFPreferencesAppSynchronize(CFSTR(kBundleName));
}

void DoTwitterAuthorize()
{
	char * req_url = oauth_sign_url2(TWITTER_REQUEST_TOKEN_URL, NULL, OA_HMAC, NULL, TWITTER_CONSUMER_KEY, TWITTER_SECRET_KEY, NULL, NULL);
	if (req_url)
	{	
		CFStringRef strUrl = CFStringCreateWithCString(kCFAllocatorDefault, req_url, kCFStringEncodingUTF8);
		if (strUrl)
		{
			CFStringRef strResponse = GetUrlContents(strUrl, CFSTR("GET"), NULL);
			if (strResponse)
			{		
				char * lpszResponse = CFStringToCString(strResponse, kCFStringEncodingUTF8);
				if (lpszResponse)
				{
					char * t_key = NULL;
					char * t_secret = NULL;

					if (parse_reply(lpszResponse, &t_key, &t_secret) == 0)
					{
						strcpy(&g_szTwitterAuthKey[0], t_key);
						strcpy(&g_szTwitterAuthSecret[0], t_secret);

						MessageBox(kAlertNoteAlert, CFSTR(TWITTER_AUTHORIZE_MESSAGE));
					
						CFStringRef strUrlLogin = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%s?oauth_token=%s&oauth_callback=oob"), TWITTER_AUTHORIZE_URL, t_key);
						if (strUrlLogin)
						{
							LOGDEBUG("Twitter Login URL: ", strUrlLogin);
						
							char szUrlLogin[MAX_PATH] = {0};
							if (CFStringGetCString(strUrlLogin, szUrlLogin, sizeof(szUrlLogin), kCFStringEncodingUTF8))
							{
								LaunchURL(&szUrlLogin[0]);
							}
						
							CFRelease(strUrlLogin);
						}
						
						free(t_key);
						free(t_secret);
					}
					else
					{
						MessageBox(kAlertStopAlert, CFSTR(TWITTER_MESSAGE_AUTHORIZATION_FAILED));
					}

					DisposePtr(lpszResponse);
				}
				
				CFRelease(strResponse);
			}
			
			CFRelease(strUrl);
		}

		free(req_url);
	}
}

void DoTwitterVerify(CFStringRef strPin)
{	
	if (strPin)
	{
		CFStringRef strUrlDesired = CFStringCreateWithFormat(NULL, NULL, CFSTR("%s?oauth_verifier=%@"), TWITTER_ACCESS_TOKEN_URL, strPin);
		if (strUrlDesired)
		{
			char * lpszUrl = CFStringToCString(strUrlDesired, kCFStringEncodingUTF8);
			if (lpszUrl)
			{
				char * postarg = NULL;
				char * req_url = oauth_sign_url2(lpszUrl, &postarg, OA_HMAC, NULL, TWITTER_CONSUMER_KEY, TWITTER_SECRET_KEY, g_szTwitterAuthKey, g_szTwitterAuthSecret);
				if (req_url)
				{
					CFStringRef strUrlToSend = CFStringCreateWithCString(kCFAllocatorDefault, req_url, kCFStringEncodingUTF8);
					if (strUrlToSend)
					{
						CFStringRef strPostData = CFStringCreateWithCString(kCFAllocatorDefault, postarg, kCFStringEncodingUTF8);
						if (strPostData)
						{
							CFStringRef strResponse = GetUrlContents(strUrlToSend, CFSTR("POST"), strPostData);
							if (strResponse)
							{
								char * lpszResponse = CFStringToCString(strResponse, kCFStringEncodingUTF8);
								if (lpszResponse);
								{								
									char *t_key = NULL;
									char *t_secret = NULL;
									
									if (parse_reply(lpszResponse, &t_key, &t_secret) == 0)
									{						
										CFStringRef strKey = CFStringCreateWithCString(kCFAllocatorDefault, t_key, kCFStringEncodingUTF8);
										if (strKey)
										{
											CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_TWITTER_KEY), strKey, CFSTR(kBundleName));											
											CFRelease(strKey);
											
											strcpy(g_szTwitterAuthKey, t_key);
										}
										
										CFStringRef strSecret = CFStringCreateWithCString(kCFAllocatorDefault, t_secret, kCFStringEncodingUTF8);
										if (strSecret)
										{										
											CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_TWITTER_SECRET), strSecret, CFSTR(kBundleName));
											CFRelease(strSecret);
											
											strcpy(g_szTwitterAuthSecret, t_secret);
										}

										char * lpszScreenName = NULL;
										if (GetTokenFromReply(lpszResponse, TWITTER_TOKEN_SCREENNAME, &lpszScreenName))
										{
											CFStringRef strScreenName = CFStringCreateWithCString(kCFAllocatorDefault, lpszScreenName, kCFStringEncodingUTF8);
											if (strScreenName)
											{
												CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_TWITTER_SCREENNAME), strScreenName, CFSTR(kBundleName));											
												CFRelease(strScreenName);
											}
											
											free(lpszScreenName);
										}

										CFPreferencesAppSynchronize(CFSTR(kBundleName));
										
										MessageBox(kAlertNoteAlert, CFSTR(TWITTER_MESSAGE_SUCCESS));

										free(t_key);
										free(t_secret);
									}
									else
									{
										MessageBox(kAlertStopAlert, CFSTR(TWITTER_MESSAGE_ACCESS_FAILED));
									}
									
									DisposePtr(lpszResponse);
								}
								
								CFRelease(strResponse);
							}
							
							CFRelease(strPostData);
						}
						
						CFRelease(strUrlToSend);
					}
					
					free(postarg);
					free(req_url);
				}
				
				DisposePtr(lpszUrl);
			}

			CFRelease(strUrlDesired);
		}
	}
}

void DoTwitterTimestamp()
{
	// Get new timestamp
	g_timetTwitterLatest = time(NULL);
	
	// Save it in case we exit the media player
	CFNumberRef numPrefValue = CFNumberCreate(NULL, kCFNumberLongType, &g_timetTwitterLatest);
	if (numPrefValue != NULL)
	{
		CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_TWITTER_LATEST), numPrefValue, CFSTR(kBundleName));
		CFPreferencesAppSynchronize(CFSTR(kBundleName));
		CFRelease(numPrefValue);
	}
}

void DoTwitter(int intNext)
{
	if (g_intLogging == LOGGING_DEBUG)
	{	
		fprintf( stderr, "Begin DoTwitter\n" );
	}

	CFMutableStringRef strMessage = CFStringCreateMutable(kCFAllocatorDefault, 0);
	if (strMessage)
	{
		CFStringAppendCString(strMessage, strlen(g_szTwitterMessage) ? g_szTwitterMessage : TWITTER_MESSAGE_DEFAULT, kCFStringEncodingUTF8);
		
		SubParameterString(strMessage, CFSTR(TWITTER_TAG_ARTIST), g_paPlaylist[intNext].track.artist);
		SubParameterString(strMessage, CFSTR(TWITTER_TAG_TITLE), g_paPlaylist[intNext].track.name);
		SubParameterString(strMessage, CFSTR(TWITTER_TAG_ALBUM), g_paPlaylist[intNext].track.album);
		SubParameterString(strMessage, CFSTR(TWITTER_TAG_GENRE), g_paPlaylist[intNext].track.genre);
		SubParameterString(strMessage, CFSTR(TWITTER_TAG_KIND), g_paPlaylist[intNext].track.kind);
		SubParameterNumber(strMessage, CFSTR(TWITTER_TAG_TRACK), CFSTR("%d"), g_paPlaylist[intNext].track.trackNumber);
		SubParameterNumber(strMessage, CFSTR(TWITTER_TAG_NUMTRACKS), CFSTR("%d"), g_paPlaylist[intNext].track.numTracks);
		SubParameterNumber(strMessage, CFSTR(TWITTER_TAG_YEAR), CFSTR("%d"), g_paPlaylist[intNext].track.year);
		SubParameterString(strMessage, CFSTR(TWITTER_TAG_COMMENTS), g_paPlaylist[intNext].track.comments);
		SubParameterNumber(strMessage, CFSTR(TWITTER_TAG_TIME), CFSTR("%d"), g_paPlaylist[intNext].track.totalTimeInMS / 1000);
		SubParameterNumber(strMessage, CFSTR(TWITTER_TAG_BITRATE), CFSTR("%d"), g_paPlaylist[intNext].track.bitRate);
		SubParameterNumber(strMessage, CFSTR(TWITTER_TAG_RATING), CFSTR("%d"), g_paPlaylist[intNext].track.trackRating / 20);
		SubParameterNumber(strMessage, CFSTR(TWITTER_TAG_DISC), CFSTR("%d"), g_paPlaylist[intNext].track.discNumber);
		SubParameterNumber(strMessage, CFSTR(TWITTER_TAG_NUMDISCS), CFSTR("%d"), g_paPlaylist[intNext].track.numDiscs);
		SubParameterNumber(strMessage, CFSTR(TWITTER_TAG_PLAYCOUNT), CFSTR("%d"), g_paPlaylist[intNext].track.playCount);
		SubParameterString(strMessage, CFSTR(TWITTER_TAG_COMPOSER), g_paPlaylist[intNext].track.composer);
		SubParameterString(strMessage, CFSTR(TWITTER_TAG_GROUPING), g_paPlaylist[intNext].track.grouping);
		SubParameterString(strMessage, CFSTR(TWITTER_TAG_FILE), g_paPlaylist[intNext].track.fileName);
		SubParameterCFString(strMessage, CFSTR(TWITTER_TAG_IMAGESMALL), g_paPlaylist[intNext].strSmallImageUrl);
		SubParameterCFString(strMessage, CFSTR(TWITTER_TAG_IMAGE), g_paPlaylist[intNext].strMediumImageUrl);
		SubParameterCFString(strMessage, CFSTR(TWITTER_TAG_IMAGELARGE), g_paPlaylist[intNext].strLargeImageUrl);
		SubParameterCFString(strMessage, CFSTR(TWITTER_TAG_URLAMAZON), g_paPlaylist[intNext].strDetailPageURL);
		
		SubBlock(strMessage, CFSTR(TWITTER_TAG_AMAZONMATCH_START), CFSTR(TWITTER_TAG_AMAZONMATCH_STOP), g_paPlaylist[intNext].strDetailPageURL != NULL);
		
		LOGDEBUG("Twitter Message: ", strMessage);
		
		//
		// Do the Ping
		//
		
		CFStringRef strUrlDesired = CFStringCreateWithFormat(NULL, NULL, CFSTR("%s?%s=%@"), TWITTER_UPDATE_URL, TWITTER_POST_VALUE, strMessage);
		if (strUrlDesired)
		{
			char * lpszUrl = CFStringToCString(strUrlDesired, kCFStringEncodingUTF8);
			if (lpszUrl)
			{
				char * postarg = NULL;
				char * req_url = oauth_sign_url2(lpszUrl, &postarg, OA_HMAC, NULL, TWITTER_CONSUMER_KEY, TWITTER_SECRET_KEY, g_szTwitterAuthKey, g_szTwitterAuthSecret);
				if (req_url)
				{
					CFStringRef strUrlToSend = CFStringCreateWithCString(kCFAllocatorDefault, req_url, kCFStringEncodingUTF8);
					if (strUrlToSend)
					{
						CFStringRef strPostData = CFStringCreateWithCString(kCFAllocatorDefault, postarg, kCFStringEncodingUTF8);
						if (strPostData)
						{
							CFStringRef strResponse = GetUrlContents(strUrlToSend, CFSTR("POST"), strPostData);
							if (strResponse)
							{
								CFStringRef strTweetId = GetXMLTagContents(strResponse, CFSTR("id"));
								if (strTweetId)
								{
									DoTwitterTimestamp();
									
									DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_TWEET);
									
									CFRelease(strTweetId);
								}
								
								CFRelease(strResponse);
							}
							
							CFRelease(strPostData);
						}
						
						CFRelease(strUrlToSend);
					}
					
					free(postarg);
					free(req_url);
				}
				
				DisposePtr(lpszUrl);
			}
			
			CFRelease(strUrlDesired);
		}
		
		CFRelease(strMessage);
	}
	
	if (g_intLogging == LOGGING_DEBUG)
	{	
		fprintf( stderr, "End DoTwitter\n" );
	}
}

void DoFacebookUserId()
{
	CFStringRef strCallId = CFStringCreateWithFormat(NULL, NULL, CFSTR("%lu"), time(NULL));
	if (strCallId)
	{
		CFMutableStringRef strPostData = CFStringCreateMutable(kCFAllocatorDefault, 0);
		if (strPostData)
		{
			CFStringAppend(strPostData, CFSTR("&call_id="));
			CFStringAppend(strPostData, strCallId);
			CFStringAppend(strPostData, CFSTR("&format=XML"));
			CFStringAppend(strPostData, CFSTR("&access_token="));
			CFStringAppendCString(strPostData, g_szFacebookSessionKey, kCFStringEncodingUTF8);
			
			//
			// Do the Ping
			//

			CFStringRef strUrl = CFStringCreateWithFormat(kCFAllocatorDefault, 0, CFSTR("%s://%s%s%s"), FACEBOOK_PROTOCOL, FACEBOOK_HOSTNAME, FACEBOOK_PATH, FACEBOOK_ACTION_USER);
			if (strUrl)
			{
				CFStringRef strData = GetUrlContents(strUrl, CFSTR("POST"), strPostData);
				if (strData)
				{
					CFStringRef strName = GetXMLTagContents(strData, CFSTR(FACEBOOK_RESPONSE_UID));
					if (strName)
					{					
						CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_FACEBOOK_UID), strName, CFSTR(kBundleName));
						CFPreferencesAppSynchronize(CFSTR(kBundleName));
						
						CFRelease(strName);
					}
					
					CFRelease(strData);
				}
				
				CFRelease(strUrl);
			}
			
			CFRelease(strPostData);
		}
		
		CFRelease(strCallId);
	}
}	


void DoFacebookInfo()
{
	CFStringRef strCallId = CFStringCreateWithFormat(NULL, NULL, CFSTR("%lu"), time(NULL));
	if (strCallId)
	{
		CFMutableStringRef strPostData = CFStringCreateMutable(kCFAllocatorDefault, 0);
		if (strPostData)
		{
			char szUid[MAX_PATH] = {0};

			LoadSettingString(MY_REGISTRY_VALUE_FACEBOOK_UID, szUid, sizeof(szUid), "");
			if (strlen(szUid) > 0)
			{
				CFStringAppend(strPostData, CFSTR("&call_id="));
				CFStringAppend(strPostData, strCallId);
				CFStringAppend(strPostData, CFSTR("&fields=name"));
				CFStringAppend(strPostData, CFSTR("&format=XML"));
				CFStringAppend(strPostData, CFSTR("&access_token="));
				CFStringAppendCString(strPostData, g_szFacebookSessionKey, kCFStringEncodingUTF8);
				CFStringAppend(strPostData, CFSTR("&uids="));
				CFStringAppendCString(strPostData, szUid, kCFStringEncodingUTF8);			
			}
			
			//
			// Do the Ping
			//
			
			CFStringRef strUrl = CFStringCreateWithFormat(kCFAllocatorDefault, 0, CFSTR("%s://%s%s%s"), FACEBOOK_PROTOCOL, FACEBOOK_HOSTNAME, FACEBOOK_PATH, FACEBOOK_ACTION_INFO);
			if (strUrl)
			{
				CFStringRef strData = GetUrlContents(strUrl, CFSTR("POST"), strPostData);
				if (strData)
				{
					CFStringRef strName = GetXMLTagContents(strData, CFSTR(FACEBOOK_RESPONSE_NAME));
					if (strName)
					{					
						CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_FACEBOOK_SCREENNAME), strName, CFSTR(kBundleName));
						CFPreferencesAppSynchronize(CFSTR(kBundleName));
						
						CFRelease(strName);
					}
					
					CFRelease(strData);
				}
				
				CFRelease(strUrl);
			}
			
			CFRelease(strPostData);
		}
		
		CFRelease(strCallId);
	}
}	

void DoFacebookReset()
{
	strcpy(g_szFacebookSessionKey, "");
	g_timetFacebookLatest = 0;
	
	CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_FACEBOOK_SESSION_KEY), CFSTR(""), CFSTR(kBundleName));											
	CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_FACEBOOK_UID), CFSTR(""), CFSTR(kBundleName));
	CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_FACEBOOK_SCREENNAME), CFSTR(""), CFSTR(kBundleName));
	
	CFNumberRef numPrefValue = CFNumberCreate(NULL, kCFNumberLongType, &g_timetFacebookLatest);
	if (numPrefValue != NULL)
	{
		CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_FACEBOOK_LATEST), numPrefValue, CFSTR(kBundleName));
		CFRelease(numPrefValue);
	}
	
	CFPreferencesAppSynchronize(CFSTR(kBundleName));
}

void DoFacebookTimestamp()
{
	// Get new timestamp
	g_timetFacebookLatest = time(NULL);
	
	// Save it in case we exit the media player
	CFNumberRef numPrefValue = CFNumberCreate(NULL, kCFNumberLongType, &g_timetFacebookLatest);
	if (numPrefValue != NULL)
	{
		CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_FACEBOOK_LATEST), numPrefValue, CFSTR(kBundleName));
		CFPreferencesAppSynchronize(CFSTR(kBundleName));
		CFRelease(numPrefValue);
	}
}

void DoFacebookUiState(WindowRef wr)
{
	if (LoadMyPreferenceString(wr, CFSTR(MY_REGISTRY_VALUE_FACEBOOK_SCREENNAME), 'face', 31, CFSTR(""), false))
	{
		ShowControl(GrabCRef(wr, 'face', 30));
		ShowControl(GrabCRef(wr, 'face', 31));
		ShowControl(GrabCRef(wr, 'face', 32));
		
		HideControl(GrabCRef(wr, 'face', 1));
		//HideControl(GrabCRef(wr, 'face', 2));
		HideControl(GrabCRef(wr, 'face', 3));
	}
	else
	{
		HideControl(GrabCRef(wr, 'face', 30));
		HideControl(GrabCRef(wr, 'face', 31));
		HideControl(GrabCRef(wr, 'face', 32));
		
		ShowControl(GrabCRef(wr, 'face', 1));
		//ShowControl(GrabCRef(wr, 'face', 2));
		ShowControl(GrabCRef(wr, 'face', 3));
	}
	
	SInt16 nValue = GetControlValue(GrabCRef(wr, 'face', 40));
	if (nValue == 2)
	{
		DeactivateControl(GrabCRef(wr, 'face', 45));
	}
	else
	{
		ActivateControl(GrabCRef(wr, 'face', 45));
	}
}

void DoFacebookSubs(CFMutableStringRef strMessage, int intNext)
{
	SubParameterString(strMessage, CFSTR(TWITTER_TAG_ARTIST), g_paPlaylist[intNext].track.artist);
	SubParameterString(strMessage, CFSTR(TWITTER_TAG_TITLE), g_paPlaylist[intNext].track.name);
	SubParameterString(strMessage, CFSTR(TWITTER_TAG_ALBUM), g_paPlaylist[intNext].track.album);
	SubParameterString(strMessage, CFSTR(TWITTER_TAG_GENRE), g_paPlaylist[intNext].track.genre);
	SubParameterString(strMessage, CFSTR(TWITTER_TAG_KIND), g_paPlaylist[intNext].track.kind);
	SubParameterNumber(strMessage, CFSTR(TWITTER_TAG_TRACK), CFSTR("%d"), g_paPlaylist[intNext].track.trackNumber);
	SubParameterNumber(strMessage, CFSTR(TWITTER_TAG_NUMTRACKS), CFSTR("%d"), g_paPlaylist[intNext].track.numTracks);
	SubParameterNumber(strMessage, CFSTR(TWITTER_TAG_YEAR), CFSTR("%d"), g_paPlaylist[intNext].track.year);
	SubParameterString(strMessage, CFSTR(TWITTER_TAG_COMMENTS), g_paPlaylist[intNext].track.comments);
	SubParameterNumber(strMessage, CFSTR(TWITTER_TAG_TIME), CFSTR("%d"), g_paPlaylist[intNext].track.totalTimeInMS / 1000);
	SubParameterNumber(strMessage, CFSTR(TWITTER_TAG_BITRATE), CFSTR("%d"), g_paPlaylist[intNext].track.bitRate);
	SubParameterNumber(strMessage, CFSTR(TWITTER_TAG_RATING), CFSTR("%d"), g_paPlaylist[intNext].track.trackRating / 20);
	SubParameterNumber(strMessage, CFSTR(TWITTER_TAG_DISC), CFSTR("%d"), g_paPlaylist[intNext].track.discNumber);
	SubParameterNumber(strMessage, CFSTR(TWITTER_TAG_NUMDISCS), CFSTR("%d"), g_paPlaylist[intNext].track.numDiscs);
	SubParameterNumber(strMessage, CFSTR(TWITTER_TAG_PLAYCOUNT), CFSTR("%d"), g_paPlaylist[intNext].track.playCount);
	SubParameterString(strMessage, CFSTR(TWITTER_TAG_COMPOSER), g_paPlaylist[intNext].track.composer);
	SubParameterString(strMessage, CFSTR(TWITTER_TAG_GROUPING), g_paPlaylist[intNext].track.grouping);
	SubParameterString(strMessage, CFSTR(TWITTER_TAG_FILE), g_paPlaylist[intNext].track.fileName);
	SubParameterCFString(strMessage, CFSTR(TWITTER_TAG_IMAGESMALL), g_paPlaylist[intNext].strSmallImageUrl);
	SubParameterCFString(strMessage, CFSTR(TWITTER_TAG_IMAGE), g_paPlaylist[intNext].strMediumImageUrl);
	SubParameterCFString(strMessage, CFSTR(TWITTER_TAG_IMAGELARGE), g_paPlaylist[intNext].strLargeImageUrl);
	SubParameterCFString(strMessage, CFSTR(TWITTER_TAG_URLAMAZON), g_paPlaylist[intNext].strDetailPageURL);
	
	SubBlock(strMessage, CFSTR(TWITTER_TAG_AMAZONMATCH_START), CFSTR(TWITTER_TAG_AMAZONMATCH_STOP), g_paPlaylist[intNext].strDetailPageURL != NULL);
}

void DoFacebookStream(int intNext)
{
	if (g_intLogging == LOGGING_DEBUG)
	{	
		fprintf( stderr, "Begin DoFacebookStream\n" );
	}
	
	if (g_intAmazonLookup == 0 || (g_paPlaylist[intNext].strDetailPageURL && CFStringGetLength(g_paPlaylist[intNext].strDetailPageURL) > 0 && g_paPlaylist[intNext].strSmallImageUrl && CFStringGetLength(g_paPlaylist[intNext].strSmallImageUrl) > 0))
	{
		CFStringRef strCallId = CFStringCreateWithFormat(NULL, NULL, CFSTR("%lu"), time(NULL));
		if (strCallId)
		{
			CFMutableStringRef strPostData = CFStringCreateMutable(kCFAllocatorDefault, 0);
			if (strPostData)
			{
				CFMutableStringRef strMessage = CFStringCreateMutable(kCFAllocatorDefault, 0);
				if (strMessage)
				{
					CFStringAppendCString(strMessage, strlen(g_szFacebookMessage) ? g_szFacebookMessage : FACEBOOK_MESSAGE_DEFAULT, kCFStringEncodingUTF8);
					
					DoFacebookSubs(strMessage, intNext);
					
					LOGDEBUG("Facebook Message: ", strMessage);
					
					CFIndex size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(strMessage), kCFStringEncodingUTF8);
					
					char * lpszMessage = (char *) malloc(sizeof(char) * size);
					if (lpszMessage)
					{			
						if (CFStringGetCString(strMessage, lpszMessage, size, kCFStringEncodingUTF8))
						{
							char * lpszMessage1 = (char *) malloc(sizeof(char) * size * 3);
							if (lpszMessage1)
							{
								*lpszMessage1 = '\0';
								
								UrlEncodeAndConcat(lpszMessage1, sizeof(char) * size * 3, lpszMessage);
								
								//
								// Generate attachment
								//
								
								CFMutableStringRef strAttachment = CFStringCreateMutable(kCFAllocatorDefault, 0);
								if (strAttachment)
								{
									CFMutableStringRef strActionLinks = CFStringCreateMutable(kCFAllocatorDefault, 0);
									if (strActionLinks)
									{
										char szAttachments1[5 * 1024] = {0};
										char szActionLinks1[5 * 1024] = {0};
										
										CFMutableStringRef strProperties = CFStringCreateMutable(kCFAllocatorDefault, 0);
										if (strProperties)
										{								
											bool fHasProp = false;
																						
											CFStringRef strName = ITUniStr255ToCFString(g_paPlaylist[intNext].track.name);
											CFStringRef strArtist = ITUniStr255ToCFString(g_paPlaylist[intNext].track.artist);
											CFStringRef strAlbum = ITUniStr255ToCFString(g_paPlaylist[intNext].track.album);
											
											CFStringAppend(strProperties, CFSTR("\"properties\":{"));

											if (strAlbum && CFStringGetLength(strAlbum) > 0)
											{
												if (fHasProp) CFStringAppend(strProperties, CFSTR(","));
												CFStringAppend(strProperties, CFSTR("\"Album\":\""));
												CFStringAppend(strProperties, strAlbum);
												CFStringAppend(strProperties, CFSTR("\""));

												fHasProp = true;
											}
											
											if (g_paPlaylist[intNext].track.year > 0)
											{
												CFStringRef strYear = CFStringCreateWithFormat(kCFAllocatorDefault, 0, CFSTR("%d"), g_paPlaylist[intNext].track.year);
												if (strYear)
												{
													if (fHasProp) CFStringAppend(strProperties, CFSTR(","));
													CFStringAppend(strProperties, CFSTR( "\"Year\":\""));
													CFStringAppend(strProperties, strYear);
													CFStringAppend(strProperties, CFSTR("\""));
													
													fHasProp = true;
													
													CFRelease(strYear);
												}
											}
											
											CFStringAppend(strProperties, CFSTR("},"));
											
											//
											// Description
											//

											CFMutableStringRef strDescription = CFStringCreateMutable(kCFAllocatorDefault, 0);
											if (strDescription)
											{
												CFStringAppendCString(strDescription, strlen(g_szFacebookAttachmentDescription) ? g_szFacebookAttachmentDescription : FACEBOOK_ATTACHMENT_DESCRIPTION_DEFAULT, kCFStringEncodingUTF8);
																								
												DoFacebookSubs(strDescription, intNext);
												
												EscapeQuotes(strDescription);

												LOGDEBUG("Facebook Description: ", strDescription);

												if (g_paPlaylist[intNext].strDetailPageURL && CFStringGetLength(g_paPlaylist[intNext].strDetailPageURL) > 0 && g_paPlaylist[intNext].strSmallImageUrl && CFStringGetLength(g_paPlaylist[intNext].strSmallImageUrl) > 0)
												{
													CFStringRef strSmallImageUrl = UrlDecode(g_paPlaylist[intNext].strSmallImageUrl);
													if (strSmallImageUrl)
													{												
														CFStringAppendFormat(strAttachment, 0, CFSTR("{\"name\":\"%@\",\"href\":\"%@\",\"caption\":\"%@\",\"description\":\"%@\",%@\"media\":[{\"type\":\"image\",\"src\":\"%@\",\"href\":\"%@\"}]}"), strName, g_paPlaylist[intNext].strDetailPageURL, strMessage, strDescription, strProperties, strSmallImageUrl, g_paPlaylist[intNext].strDetailPageURL);																								

														CFRelease(strSmallImageUrl);
													}
												}
												else
												{
													CFStringAppendFormat(strAttachment, 0, CFSTR("{\"name\":\"%@\",\"caption\":\"%@\",\"description\":\"%@\",%@}"), strName, strMessage, strDescription, strProperties);
												}

												LOGDEBUG("Facebook Attachment: ", strAttachment);

												char * lpszValue1 = CFStringToCString(strAttachment, kCFStringEncodingUTF8);
												if (lpszValue1)
												{											
													UrlEncodeAndConcat(szAttachments1, sizeof(szAttachments1), lpszValue1);
													free(lpszValue1);
												}
												
												CFRelease(strDescription);
											}
											
											//
											// Action Links
											//
											
											CFStringAppendFormat(strActionLinks, 0, CFSTR("[{\"text\":\"Artist Bio\",\"href\":\"http://en.wikipedia.org/w/index.php?search=%@\"}]"), strArtist);
											char * lpszValue2 = CFStringToCString(strActionLinks, kCFStringEncodingUTF8);
											if (lpszValue2)
											{												
												UrlEncodeAndConcat(szActionLinks1, sizeof(szActionLinks1), lpszValue2);
												free(lpszValue2);
											}

											if (strName)
											{
												CFRelease(strName);
											}

											if (strArtist)
											{
												CFRelease(strArtist);
											}
											
											if (strAlbum)
											{
												CFRelease(strAlbum);
											}
											
											CFRelease(strProperties);
										}
													
										
										//
										// Setup params in alphabetical order
										//

										CFStringAppend(strPostData, CFSTR("&"));
										CFStringAppend(strPostData, CFSTR(FACEBOOK_PARAMETER_ACTIONLINK));
										CFStringAppend(strPostData, CFSTR("="));
										CFStringAppendCString(strPostData, szActionLinks1, kCFStringEncodingUTF8);
										CFStringAppend(strPostData, CFSTR("&"));							
										CFStringAppend(strPostData, CFSTR(FACEBOOK_PARAMETER_ATTACHMENT));
										CFStringAppend(strPostData, CFSTR("="));
										CFStringAppendCString(strPostData, szAttachments1, kCFStringEncodingUTF8);
										CFStringAppend(strPostData, CFSTR("&call_id="));
										CFStringAppend(strPostData, strCallId);
										CFStringAppend(strPostData, CFSTR("&format=XML"));
	//									CFStringAppend(strPostData, CFSTR("&"));
	//									CFStringAppend(strPostData, CFSTR(FACEBOOK_PARAMETER_MESSAGE));
	//									CFStringAppend(strPostData, CFSTR("="));
	//									CFStringAppendCString(strPostData, lpszMessage1, kCFStringEncodingUTF8);
										CFStringAppend(strPostData, CFSTR("&access_token="));
										CFStringAppendCString(strPostData, g_szFacebookSessionKey, kCFStringEncodingUTF8);

										//LOGDEBUG("POST: ", strPostData);
										
										CFRelease(strActionLinks);
									}

									CFRelease(strAttachment);
								}
									
								free(lpszMessage1);
							}
						}
									 
						free(lpszMessage);
					}
									 
					CFRelease(strMessage);
				}
									 
				//
				// Do the Ping
				//
				 
				CFStringRef strUrl = CFStringCreateWithFormat(kCFAllocatorDefault, 0, CFSTR("%s://%s%s%s"), FACEBOOK_PROTOCOL, FACEBOOK_HOSTNAME, FACEBOOK_PATH, FACEBOOK_ACTION_STREAM);
				if (strUrl)
				{
					CFStringRef strData = GetUrlContents(strUrl, CFSTR(HTTP_METHOD_POST), strPostData);
					if (strData)
					{
						CFStringRef strResponse = GetXMLTagContents(strData, CFSTR(FACEBOOK_RESPONSE_STREAM_PUBLISH));
						if (strResponse)
						{
							LOGDEBUG("ID: ", strResponse);

							DoFacebookTimestamp();
							
							DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_FACEBOOK);
							
							CFRelease(strResponse);
						}
						else
						{												
							CFStringRef strErrorMsg = GetXMLTagContents(strData, CFSTR(FACEBOOK_RESPONSE_ERROR_MESSAGE));
							if (strErrorMsg)
							{					
								LOGDEBUG("Error: ", strErrorMsg);
								
								CFRelease(strErrorMsg);
							}						
						}
						 
						CFRelease(strData);
					 }

					 CFRelease(strUrl);
				 }
				 
				 CFRelease(strPostData);
			}
			 
			CFRelease(strCallId);
		}
		else
		{
			AMLOGDEBUG("Skipping publish because Amazon lookup enabled but song not found\n");
		}
	}
		 
	AMLOGDEBUG("End DoFacebookStream\n");
}
																			 
void DoFacebookAuthorize()
{
	bool fResult = false;
	
	if (strlen(g_szFacebookAuthToken) > 0)
	{
		CFStringRef strUrl = CFStringCreateWithFormat(kCFAllocatorDefault, 0, CFSTR("http://%s%s%s"), FACEBOOK_AUTHORIZE_HOSTNAME, FACEBOOK_AUTHORIZE_PATH, g_szFacebookAuthToken);
		if (strUrl)
		{
			CFStringRef strData = GetUrlContents(strUrl, CFSTR(HTTP_METHOD_GET), NULL);
			if (strData)
			{
				LOGDEBUG("Facebook Authorize Result: ", strData);

				CFStringRef strSessionKey = GetXMLTagContents(strData, CFSTR(FACEBOOK_RESPONSE_SESSION_KEY));
				if (strSessionKey)
				{
					if (CFStringGetCString(strSessionKey, g_szFacebookSessionKey, sizeof(g_szFacebookSessionKey), MY_ENCODING_DEFAULT))
					{
						CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_FACEBOOK_SESSION_KEY), strSessionKey, CFSTR(kBundleName));
						CFPreferencesAppSynchronize(CFSTR(kBundleName));
							
						DoFacebookUserId();

						DoFacebookInfo();

						MessageBox(kAlertNoteAlert, CFSTR(FACEBOOK_MESSAGE_VERIFY_SUCCESS));

						fResult = true;							
					}

					CFRelease(strSessionKey);
				}

				CFRelease(strData);
			}

			CFRelease(strUrl);
		}
		
		
		if (!fResult)
		{
			MessageBox(kAlertStopAlert, CFSTR("Facebook authorization was not successful."));
		}
	}
	else
	{
		MessageBox(kAlertStopAlert, CFSTR("Step 1 must be completed first."));
	}
}

void DoFacebookAdd()
{
	MessageBox(kAlertNoteAlert, CFSTR(FACEBOOK_MESSAGE_ADD));
	
	CFUUIDRef uuid = CFUUIDCreate(kCFAllocatorDefault);
	if (uuid)
	{
		CFStringRef strUrl = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%s%@&version=%s"), FACEBOOK_URL_ADD, uuid, g_szVersion);
		if (strUrl)
		{
			char szUrl[MAX_PATH] = {0};
			if (CFStringGetCString(strUrl, szUrl, sizeof(szUrl), kCFStringEncodingUTF8))
			{
				LaunchURL(&szUrl[0]);
				
				CFStringRef strUuid = CFUUIDCreateString(kCFAllocatorDefault, uuid);
				if (strUuid)
				{
					CFStringGetCString(strUuid, g_szFacebookAuthToken, sizeof(g_szFacebookAuthToken), kCFStringEncodingUTF8);
					
					CFRelease(strUuid);
				}
			}

			CFRelease(strUrl);
		}
		
		CFRelease(uuid);
	}
}

void DoGoogleAnalytics(char * lpszAction)
{
	char szVisitor[MAX_PATH] = {0};
	
 	sprintf(szVisitor, "%s%s", g_szGuid, GOOGLE_ANALYTICS_UTMAC);
	
	MD5_CTX udtMD5Context;
	unsigned char szDigest[16];
	
	MD5Init(&udtMD5Context);
	MD5Update(&udtMD5Context, (unsigned char *) szVisitor, strlen(szVisitor));
	MD5Final(szDigest, &udtMD5Context);
	
	char szDigestHex[MAX_PATH] = {0};
	sprintf(szDigestHex, "0x%02x%02x%02x%02x%02x%02x%02x%02x", szDigest[0], szDigest[1], szDigest[2], szDigest[3], szDigest[4], szDigest[5], szDigest[6], szDigest[7]);
		
	CFStringRef strUrl = CFStringCreateWithFormat(kCFAllocatorDefault, 0, 
												  CFSTR("%s://%s%s?utmac=%s&utmwv=%s&utmhn=%s&utmp=%s&utmcc=%s&utmn=%lu&utmvid=%s&utmr=-&utmt=event&utme=5(%s*%s*%s)"), 
												  GOOGLE_ANALYTICS_PROTOCOL,
												  GOOGLE_ANALYTICS_HOSTNAME, 
												  GOOGLE_ANALYTICS_PATH, 
												  GOOGLE_ANALYTICS_UTMAC, 
												  GOOGLE_ANALYTICS_UTMWV,
												  GOOGLE_ANALYTICS_UTMHN,
												  GOOGLE_ANALYTICS_UTMP,
												  GOOGLE_ANALYTICS_UTMCC,
												  time(NULL),
												  szDigestHex,
												  GOOGLE_ANALYTICS_CATEGORY_MAC,
												  lpszAction,
												  g_szVersion);
	if (strUrl)
	{
		CFStringRef strData = GetUrlContents(strUrl, CFSTR(HTTP_METHOD_GET), NULL);
		if (strData)
		{
			CFRelease(strData);
		}
		
		CFRelease(strUrl);
	}
}

void GetArtworkID(ITUniStr255 artist, ITUniStr255 album, char * lpszArtworkID)
{
	CFMutableStringRef strArtworkString = CFStringCreateMutable(kCFAllocatorDefault, 0);
	if (strArtworkString)
	{	
		CFStringRef strArtist = CFStringCreateWithBytes(kCFAllocatorDefault, (UInt8 *) &artist[1], artist[0] * sizeof(UniChar), kCFStringEncodingUnicode, false);
		if (strArtist)
		{
			CFStringRef strAlbum = CFStringCreateWithBytes(kCFAllocatorDefault, (UInt8 *) &album[1], album[0] * sizeof(UniChar), kCFStringEncodingUnicode, false);
			if (strAlbum)
			{			
				CFStringAppend(strArtworkString, strArtist);
				CFStringAppend(strArtworkString, strAlbum);
				
				LOGDEBUG("Artwork ID is MD5 of: ", strArtworkString);

				CFIndex size = CFStringGetMaximumSizeForEncoding(CFStringGetLength( strArtworkString ), kCFStringEncodingUTF8);
				char * lpszBuf = (char *) malloc( sizeof( char ) * size);
				if (lpszBuf) 
				{
					if (CFStringGetCString( strArtworkString, lpszBuf, size, kCFStringEncodingUTF8))
					{
						MD5_CTX udtMD5Context;
						unsigned char szDigest[16];

						MD5Init(&udtMD5Context);
						MD5Update(&udtMD5Context, (unsigned char *) lpszBuf, strlen(lpszBuf));
						MD5Final(szDigest, &udtMD5Context);

						sprintf(lpszArtworkID,	"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
												szDigest[0], szDigest[1], szDigest[2], szDigest[3], szDigest[4], szDigest[5],
												szDigest[6], szDigest[7], szDigest[8], szDigest[9], szDigest[10], szDigest[11],
												szDigest[12], szDigest[13], szDigest[14], szDigest[15]);
					}

					free(lpszBuf);
				}
				
				CFRelease(strAlbum);
			}
		
			CFRelease(strArtist);
		}

		CFRelease(strArtworkString);
	}
}

void DoUpdate(ITTrackInfo* pTrack, bool fPlay)
{
    OSStatus err = noErr;

	if ( g_intLogging == LOGGING_DEBUG )
	{
		fprintf( stderr, "Begin DoUpdate\n" );	
	}

	if ( !g_fLicensed && fPlay )
	{
		if ( g_intOnce > TRIAL_LIMIT )
		{		
			if ( g_intLogging == LOGGING_DEBUG )
			{
				fprintf( stderr, "Exiting DoUpdate -- Over Trial Limit\n" );	
			}

			return;
		}
		else if ( g_intOnce == TRIAL_LIMIT )
		{
			MessageBox(kAlertStopAlert, CFSTR("TRIAL VERSION\n\nPurchase the licensed version today! This trial version will only work for the five songs you play each session."));
		}
	}

	//
	// Skip any short songs if configured
	//

	if ( g_intSkipShort > 0 && pTrack->totalTimeInMS > 0 && g_intSkipShort * 1000 > pTrack->totalTimeInMS )
	{
		fprintf( stderr, "Duration %ld ms; skipping", pTrack->totalTimeInMS );

		return;
	}
	
	//
	// Skip any songs that match kinds
	//

	if (strlen(g_szSkipKinds) > 0)
	{
		bool skip = false;
		
		CFStringRef strKind = ITUniStr255ToCFString(pTrack->kind);
		if (strKind)
		{
			char * lpszKind = CFStringToCString(strKind, 0);
			if (lpszKind)
			{			
				char * lpszTokens = strdup(g_szSkipKinds);
				if (lpszTokens)
				{
					char * lpszToken = strtok(lpszTokens, ",");
					while (lpszToken != NULL && !skip)
					{					
						if (strcmp(lpszToken, lpszKind) == 0)
						{
							fprintf(stderr, "Kind matched \"%s\"; skipping\n", lpszKind);
							
							skip = true;
						}
						else
						{
							lpszToken = strtok(NULL, ",");
						}
					}
					
					free(lpszTokens);
				}
				
				DisposePtr(lpszKind);
			}
			
			CFRelease(strKind);
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
	
	memcpy( &g_paPlaylist[ g_intNext ].track, pTrack, sizeof( ITTrackInfo ) );

	g_paPlaylist[ g_intNext ].fSet = true;
	g_paPlaylist[ g_intNext ].fPlay = true;
	
	if ( g_paPlaylist[ g_intNext ].strDetailPageURL )
	{
		CFRelease( g_paPlaylist[ g_intNext ].strDetailPageURL );
		g_paPlaylist[ g_intNext ].strDetailPageURL = NULL;
	}

	if ( g_paPlaylist[ g_intNext ].strSmallImageUrl )
	{
		CFRelease( g_paPlaylist[ g_intNext ].strSmallImageUrl );
		g_paPlaylist[ g_intNext ].strSmallImageUrl = NULL;
	}

	if ( g_paPlaylist[ g_intNext ].strMediumImageUrl )
	{
		CFRelease( g_paPlaylist[ g_intNext ].strMediumImageUrl );
		g_paPlaylist[ g_intNext ].strMediumImageUrl = NULL;
	}

	if ( g_paPlaylist[ g_intNext ].strLargeImageUrl )
	{
		CFRelease( g_paPlaylist[ g_intNext ].strLargeImageUrl );
		g_paPlaylist[ g_intNext ].strLargeImageUrl = NULL;
	}

	if ( g_paPlaylist[ g_intNext ].strAppleURL )
	{
		CFRelease( g_paPlaylist[ g_intNext ].strAppleURL );
		g_paPlaylist[ g_intNext ].strAppleURL = NULL;
	}

	if ( g_paPlaylist[ g_intNext ].strArtworkId )
	{
		CFRelease( g_paPlaylist[ g_intNext ].strArtworkId );
		g_paPlaylist[ g_intNext ].strArtworkId = NULL;
	}

	//
	// Alert worker thread that it has stuff to process
	//

	err = MPSetEvent( g_hSignalPlayEvent, kNewSong );
	if ( err != noErr )
	{
		if ( g_intLogging == LOGGING_DEBUG )
		{
			fprintf( stderr, "MPSetEvent failed with %lu!\n", err );	
		}
	}

	//
	// Count for the trial
	//			

	if ( fPlay )
	{
		g_intOnce++;
	}

	if ( g_intLogging == LOGGING_DEBUG )
	{
		fprintf( stderr, "End DoUpdate\n" );	
	}
}

//	MemClear
static void MemClear(LogicalAddress dest,SInt32 length)
{
	register unsigned char * ptr;

	ptr = (unsigned char *) dest;
	
	while (length-- > 0)
		*ptr++ = 0;
}

// A little utility routine for grabbing control references
// takes a little setup and teardown work and puts it in one place.
ControlRef GrabCRef(WindowRef theWindow, OSType theSig, SInt32 theNum)
{
	ControlID contID;
	ControlRef theRef = NULL;
    contID.signature = theSig;
	contID.id = theNum;
	GetControlByID( theWindow, &contID, &theRef );
	return theRef;
}

// Set up the tab initial state.  In this case we just make pane 1 the visible pane
// in a real situation, you would want to cache the last place a user visited and 
// reset to that when the window is re-opened
static void SetInitialTabState(WindowRef theWindow)
{
    int tabList[] = { kTabMasterID + 0, kTabMasterID + 1, kTabMasterID + 2, kTabMasterID + 3, kTabMasterID + 4, kTabMasterID + 5, kTabMasterID + 6, kTabMasterID + 7, kTabMasterID + 8, kTabMasterID + 9}; 
    short qq = 0;

    // If we just run without setting the initial state, then the tab control
    // will have both (or all) sets of controls overlapping each other.
    // So we'll fix that by making one pane active right off the bat.
    
    // First pass, turn every pane invisible
    for ( qq = 0; qq < kMaxNumTabs; qq++ )
	{
		SetControlVisibility( GrabCRef( theWindow, kTabPaneSig, tabList[qq] ), false, true );
	}
    
    // Set the tab control itself to have a value of 1, the first pane of the tab set
    SetControlValue( GrabCRef( theWindow, kTabMasterSig, kTabMasterID ), 1 );

    // This is the important bit, of course.  We're setting the currently selected pane
    // to be visible, which makes the associated controls in the pane visible.
    SetControlVisibility( GrabCRef( theWindow, kTabPaneSig, tabList[0] ), true, true );
}

// Handler for the prefs tabs
// Switches between the 3 panes we have in this sample
static pascal OSStatus PrefsTabEventHandlerProc( EventHandlerCallRef inCallRef, EventRef inEvent, void* inUserData )
{
    static UInt16 lastPaneSelected = 1;	// static, to keep track of it long term (in a more complex application
                                        // you might store this in a data structure in the window refCon)                                            
    WindowRef theWindow = (WindowRef) inUserData;  // get the windowRef, passed around as userData    
    short controlValue = 0;

    //  Get the new value of the tab control now that the user has picked it    
    controlValue = GetControlValue( GrabCRef( theWindow, kTabMasterSig, kTabMasterID ) );

    // same as last ?
    if ( controlValue != lastPaneSelected )
    {
		// different from last time.
        // Hide the current pane and make the user selected pane the active one
        // our 3 tab pane IDs.  Put a dummy in so we can index without subtracting 1 (this array is zero based, 
        // control values are 1 based).
        int tabList[] = {kDummyValue, kTabMasterID + 0, kTabMasterID + 1, kTabMasterID + 2, kTabMasterID + 3, kTabMasterID + 4, kTabMasterID + 5, kTabMasterID + 6, kTabMasterID + 7, kTabMasterID + 8, kTabMasterID + 9 };
                                                                                    
        // Hide the current one, and set the new one
        SetControlVisibility( GrabCRef( theWindow, kTabPaneSig, tabList[lastPaneSelected] ), false, true );
        SetControlVisibility( GrabCRef( theWindow, kTabPaneSig, tabList[controlValue] ), true, true );    

        // make sure the new configuration is drawn correctly by redrawing the Tab control itself        
        Draw1Control( GrabCRef( theWindow, kTabMasterSig, kTabMasterID ) );		

        // and update our tracking
        lastPaneSelected = controlValue;
    }
    
    return eventNotHandledErr;
}

void SaveMyPreference(WindowRef wr, CFStringRef strPrefName, OSType theSig, SInt32 theNum, bool fText, bool fEncrypted)
{
	char szKey[128];
	
	CFStringGetCString( strPrefName, &szKey[0], 128, 0 );

	if ( fText )
	{
		CFStringRef strPrefValue;
		Size actualSize = 0;

		GetControlData( GrabCRef( wr, theSig, theNum ), kControlEditTextPart, kControlEditTextCFStringTag, sizeof( strPrefValue ), &strPrefValue, &actualSize );

		char szValue[MAX_PATH] = {0};
		CFStringGetCString( strPrefValue, &szValue[0], sizeof( szValue ), MY_ENCODING_DEFAULT);

		if ( fEncrypted && actualSize > 0 )
		{
			char szTemp[MAX_PATH] = {0};
	
			EncryptString( szValue, szTemp, MAX_PATH );
			
			strPrefValue = CFStringCreateWithCString( NULL, szTemp, MY_ENCODING_DEFAULT);
		}
		
		CFPreferencesSetAppValue( strPrefName, strPrefValue, CFSTR( kBundleName ) );

		//fprintf( stderr, "Saving Preference: Key = %s, Value = %s\n", szKey, szValue ); 
		
		if ( strPrefValue != NULL )
		{
			CFRelease( strPrefValue );
		}
	}
	else
	{
		SInt16 nValue = GetControlValue( GrabCRef( wr, theSig, theNum ) );
		CFNumberRef numPrefValue = CFNumberCreate( NULL, kCFNumberSInt16Type, &nValue );

		CFPreferencesSetAppValue( strPrefName, numPrefValue, CFSTR( kBundleName ) );

		//fprintf( stderr, "Saving Preference: Key = %s, Value = %d\n", szKey, nValue ); 

		if ( numPrefValue != NULL )
		{
			CFRelease( numPrefValue );
		}		
	}
}

void SaveMyPreferenceNumberInTextbox(WindowRef wr, CFStringRef strPrefName, OSType theSig, SInt32 theNum)
{
	CFStringRef strPrefValue;
	Size actualSize = 0;
	
	GetControlData( GrabCRef( wr, theSig, theNum ), kControlEditTextPart, kControlEditTextCFStringTag, sizeof( strPrefValue ), &strPrefValue, &actualSize );

	SInt32 nValue = CFStringGetIntValue ( strPrefValue );

	CFNumberRef numPrefValue = CFNumberCreate( NULL, kCFNumberSInt32Type, &nValue );

	CFPreferencesSetAppValue( strPrefName, numPrefValue, CFSTR( kBundleName ) );
	
	if ( numPrefValue )
	{
		CFRelease( numPrefValue );
	}
}

void SaveMyPreferences(WindowRef wr)
{
	// Set as configured
	SInt16 nValue = 1;
	CFNumberRef numPrefValue = CFNumberCreate( NULL, kCFNumberSInt16Type, &nValue );
	CFPreferencesSetAppValue( CFSTR( MY_REGISTRY_VALUE_CONFIGURED ), numPrefValue, CFSTR( kBundleName ) );
	CFRelease( numPrefValue );
	
	// Options
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_PUBLISH_STOP), 'opti', 1, false, false );
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_ARTWORK_EXPORT), 'opti', 2, false, false );
	SaveMyPreferenceNumberInTextbox( wr, CFSTR(MY_REGISTRY_VALUE_ARTWORK_WIDTH), 'opti', 3 );
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_LOGGING), 'opti', 4, false, false );
	SaveMyPreferenceNumberInTextbox( wr, CFSTR(MY_REGISTRY_VALUE_SKIPSHORT), 'opti', 5 );
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_SKIPKINDS), 'opti', 6, true, false ); 

	// XML
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_FILE), 'xmlx', 1, true, false );
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_XML_CDATA), 'xmlx', 2, false, false );
	SaveMyPreferenceNumberInTextbox( wr, CFSTR(MY_REGISTRY_VALUE_PLAYLIST_LENGTH), 'xmlx', 3 );
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_XML_ENCODING), 'xmlx', 4, false, false );
	
	// Upload
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_PROTOCOL), 'uplo', 1, false, false );
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_HOST), 'uplo', 2, true, false );
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_USER), 'uplo', 3, true, false );
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_PASSWORD), 'uplo', 4, true, true );
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_PATH), 'uplo', 5, true, false );
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_ARTWORK_UPLOAD), 'uplo', 6, false, false );
	
	// Ping
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_TRACKBACK_URL), 'ping', 1, true, false );
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_PING_EXTRA_INFO), 'ping', 2, true, false );

	// Twitter
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_TWITTER_ENABLED), 'twit', 4, false, false );
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_TWITTER_MESSAGE), 'twit', 0, true, false );
	SaveMyPreferenceNumberInTextbox( wr, CFSTR(MY_REGISTRY_VALUE_TWITTER_RATE_LIMIT_MINUTES), 'twit', 20 );

	// Facebook
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_FACEBOOK_ENABLED), 'face', 4, false, false );
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_FACEBOOK_MESSAGE), 'face', 0, true, false );
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_FACEBOOK_ATTACHMENT_DESCRIPTION), 'face', 45, true, false );
	SaveMyPreferenceNumberInTextbox( wr, CFSTR(MY_REGISTRY_VALUE_FACEBOOK_RATE_LIMIT_MINUTES), 'face', 10 );
	//SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_FACEBOOK_POST_TO), 'face', 40, false, false );
	
	// Amazon
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_AMAZON_ENABLED), 'amaz', 1, false, false );
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_AMAZON_LOCALE), 'amaz', 2, false, false );
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_AMAZON_ASSOCIATE), 'amaz', 3, true, false );
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_AMAZON_USE_ASIN), 'amaz', 4, false, false );

	// Apple
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_APPLE_ENABLED), 'appl', 1, false, false );
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_APPLE_ASSOCIATE), 'appl', 2, true, false );

	// License
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_EMAIL), 'lice', 1, true, false );
	SaveMyPreference( wr, CFSTR(MY_REGISTRY_VALUE_LICENSE_KEY), 'lice', 2, true, false );		

	// Flush
	CFPreferencesAppSynchronize( CFSTR( kBundleName ) );
	
	// Reload
	CacheMySettings( false );
}

bool LoadMyPreferenceString(WindowRef wr, CFStringRef strPrefName, OSType theSig, SInt32 theNum, CFStringRef strPrefValueDefault, bool fEncrypted)
{
	bool hasValue = false;
	bool fRelease = false;
 	char szKey[128];

	CFStringGetCString( strPrefName, &szKey[0], 128, 0 );

	char szValue[MAX_PATH] = {0};
	
	CFStringRef strPrefValue = (CFStringRef) CFPreferencesCopyAppValue( strPrefName, CFSTR( kBundleName ) );
	if ( strPrefValue == NULL )
	{
		strPrefValue = strPrefValueDefault;
		CFStringGetCString( strPrefValue, &szValue[0], sizeof( szValue ), MY_ENCODING_DEFAULT );
	}
	else
	{
		fRelease = true;
		
		CFStringGetCString( strPrefValue, &szValue[0], sizeof( szValue ), MY_ENCODING_DEFAULT );
						
		if ( fEncrypted )
		{
			char szTemp[MAX_PATH] = {0};
			CFRelease( strPrefValue );
			DecryptString( szValue, szTemp, sizeof( szTemp ) );
			strPrefValue = CFStringCreateWithCString( NULL, szTemp, MY_ENCODING_DEFAULT ); 
		}
	}
	
	hasValue = CFStringGetLength(strPrefValue) > 0;

	SetControlData( GrabCRef( wr, theSig, theNum ), kControlEditTextPart, kControlEditTextCFStringTag, sizeof( strPrefValue ), &strPrefValue );

	if ( fRelease )
	{
		CFRelease( strPrefValue );
	}
	
	return hasValue;
}

void LoadMyPreferenceNumber(WindowRef wr, CFStringRef strPrefName, OSType theSig, SInt32 theNum, int intPrefValueDefault)
{
	bool fRelease = false;
 	char szKey[128];

	CFStringGetCString( strPrefName, &szKey[0], 128, 0 );

	SInt16 nValue = 1;
	CFNumberRef numPrefValue = (CFNumberRef) CFPreferencesCopyAppValue( strPrefName, CFSTR( kBundleName ) );
	if ( numPrefValue == NULL )
	{
		nValue = intPrefValueDefault;
	}
	else
	{
		CFNumberGetValue( numPrefValue, kCFNumberSInt16Type, &nValue );
		fRelease = true;
	}
	
	SetControlValue( GrabCRef( wr, theSig, theNum ), nValue );
	
	if ( fRelease && numPrefValue != NULL )
	{
		CFRelease( numPrefValue );
	}
}

void LoadMyPreferenceNumberInTextbox(WindowRef wr, CFStringRef strPrefName, OSType theSig, SInt32 theNum, int intPrefValueDefault)
{
	CFNumberRef numPrefValue = (CFNumberRef) CFPreferencesCopyAppValue( strPrefName, CFSTR( kBundleName ) );
	if ( numPrefValue == NULL )
	{
		numPrefValue = CFNumberCreate( NULL, kCFNumberIntType, &intPrefValueDefault );
	}
	
	CFStringRef strPrefValue = CFStringCreateWithFormat( NULL, NULL, CFSTR("%@"), numPrefValue );
	if ( strPrefValue )
	{
		SetControlData( GrabCRef( wr, theSig, theNum ), kControlEditTextPart, kControlEditTextCFStringTag, sizeof( strPrefValue ), &strPrefValue );
	
		CFRelease( strPrefValue );
	}
	
	CFRelease( numPrefValue );
}

void LoadMyPreferences(WindowRef wr)
{
	// Options
	LoadMyPreferenceNumber( wr, CFSTR(MY_REGISTRY_VALUE_PUBLISH_STOP), 'opti', 1, DEFAULT_PUBLISH_STOP );
	LoadMyPreferenceNumber( wr, CFSTR(MY_REGISTRY_VALUE_ARTWORK_EXPORT), 'opti', 2, DEFAULT_ARTWORK_EXPORT );
	LoadMyPreferenceNumberInTextbox( wr, CFSTR(MY_REGISTRY_VALUE_ARTWORK_WIDTH), 'opti', 3, DEFAULT_ARTWORK_WIDTH );
	LoadMyPreferenceNumber( wr, CFSTR(MY_REGISTRY_VALUE_LOGGING), 'opti', 4, 2 );
	LoadMyPreferenceNumberInTextbox( wr, CFSTR(MY_REGISTRY_VALUE_SKIPSHORT), 'opti', 5, DEFAULT_SKIPSHORT_SECONDS );
	LoadMyPreferenceString( wr, CFSTR(MY_REGISTRY_VALUE_SKIPKINDS), 'opti', 6, CFSTR(""), false );

	// XML
	LoadMyPreferenceString( wr, CFSTR(MY_REGISTRY_VALUE_FILE), 'xmlx', 1, CFSTR("/now_playing.xml"), false );
	LoadMyPreferenceNumber( wr, CFSTR(MY_REGISTRY_VALUE_XML_CDATA), 'xmlx', 2, 1 );
	LoadMyPreferenceNumberInTextbox( wr, CFSTR(MY_REGISTRY_VALUE_PLAYLIST_LENGTH), 'xmlx', 3, DEFAULT_PLAYLIST_LENGTH );
	LoadMyPreferenceNumber( wr, CFSTR(MY_REGISTRY_VALUE_XML_ENCODING), 'xmlx', 4, 1 );

	// Upload
	LoadMyPreferenceNumber( wr, CFSTR(MY_REGISTRY_VALUE_PROTOCOL), 'uplo', 1, 1 );
	LoadMyPreferenceString( wr, CFSTR(MY_REGISTRY_VALUE_HOST), 'uplo', 2, CFSTR(""), false );
	LoadMyPreferenceString( wr, CFSTR(MY_REGISTRY_VALUE_USER), 'uplo', 3, CFSTR(""), false );
	LoadMyPreferenceString( wr, CFSTR(MY_REGISTRY_VALUE_PASSWORD), 'uplo', 4, CFSTR(""), true );
	LoadMyPreferenceString( wr, CFSTR(MY_REGISTRY_VALUE_PATH), 'uplo', 5, CFSTR(""), false );
	LoadMyPreferenceNumber( wr, CFSTR(MY_REGISTRY_VALUE_ARTWORK_UPLOAD), 'uplo', 6, DEFAULT_ARTWORK_UPLOAD );
	
	// Ping
	LoadMyPreferenceString( wr, CFSTR(MY_REGISTRY_VALUE_TRACKBACK_URL), 'ping', 1, CFSTR(""), false );
	LoadMyPreferenceString( wr, CFSTR(MY_REGISTRY_VALUE_PING_EXTRA_INFO), 'ping', 2, CFSTR(""), false );
	
	// Twitter
	LoadMyPreferenceNumber( wr, CFSTR(MY_REGISTRY_VALUE_TWITTER_ENABLED), 'twit', 4, 0 );
	LoadMyPreferenceString( wr, CFSTR(MY_REGISTRY_VALUE_TWITTER_MESSAGE), 'twit', 0, CFSTR(""), false );
	LoadMyPreferenceString( wr, CFSTR(MY_REGISTRY_VALUE_TWITTER_SCREENNAME), 'twit', 9, CFSTR(""), false );
	CFStringRef strTwitterTagsSupported = CFSTR(TWITTER_TAGS_SUPPORTED);
	SetControlData( GrabCRef( wr, 'twit', 5 ), kControlEditTextPart, kControlEditTextCFStringTag, sizeof( strTwitterTagsSupported ), &strTwitterTagsSupported );
	LoadMyPreferenceNumberInTextbox( wr, CFSTR(MY_REGISTRY_VALUE_TWITTER_RATE_LIMIT_MINUTES), 'twit', 20, TWITTER_RATE_LIMIT_MINUTES_DEFAULT );
	
	// Facebook
	LoadMyPreferenceNumber( wr, CFSTR(MY_REGISTRY_VALUE_FACEBOOK_ENABLED), 'face', 4, 0 );
	LoadMyPreferenceString( wr, CFSTR(MY_REGISTRY_VALUE_FACEBOOK_MESSAGE), 'face', 0, CFSTR(""), false );
	LoadMyPreferenceString( wr, CFSTR(MY_REGISTRY_VALUE_FACEBOOK_ATTACHMENT_DESCRIPTION), 'face', 45, CFSTR(""), false );
	CFStringRef strFacebookTagsSupported = CFSTR(FACEBOOK_TAGS_SUPPORTED);
	SetControlData( GrabCRef( wr, 'face', 5 ), kControlEditTextPart, kControlEditTextCFStringTag, sizeof( strFacebookTagsSupported ), &strFacebookTagsSupported );
	LoadMyPreferenceNumberInTextbox( wr, CFSTR(MY_REGISTRY_VALUE_FACEBOOK_RATE_LIMIT_MINUTES), 'face', 10, FACEBOOK_RATE_LIMIT_MINUTES_DEFAULT );

	// Amazon
	LoadMyPreferenceNumber( wr, CFSTR(MY_REGISTRY_VALUE_AMAZON_ENABLED), 'amaz', 1, DEFAULT_AMAZON_ENABLED );
	LoadMyPreferenceNumber( wr, CFSTR(MY_REGISTRY_VALUE_AMAZON_LOCALE), 'amaz', 2, AMAZON_LOCALE_US );
	LoadMyPreferenceString( wr, CFSTR(MY_REGISTRY_VALUE_AMAZON_ASSOCIATE), 'amaz', 3, CFSTR(""), false );
	LoadMyPreferenceNumber( wr, CFSTR(MY_REGISTRY_VALUE_AMAZON_USE_ASIN), 'amaz', 4, DEFAULT_AMAZON_USE_ASIN );

	// Apple
	LoadMyPreferenceNumber( wr, CFSTR(MY_REGISTRY_VALUE_APPLE_ENABLED), 'appl', 1, DEFAULT_APPLE_ENABLED );
	LoadMyPreferenceString( wr, CFSTR(MY_REGISTRY_VALUE_APPLE_ASSOCIATE), 'appl', 2, CFSTR(""), false );

	// License
	LoadMyPreferenceString( wr, CFSTR(MY_REGISTRY_VALUE_EMAIL), 'lice', 1, CFSTR(""), false );
	LoadMyPreferenceString( wr, CFSTR(MY_REGISTRY_VALUE_LICENSE_KEY), 'lice', 2, CFSTR(""), false );
}

void LoadSettingNumber(const char * lpszPrefName, int* pintValue, int intDefault)
{
	bool fRelease = false;

	CFStringRef strPrefName = CFStringCreateWithCString(kCFAllocatorDefault, lpszPrefName, MY_ENCODING_DEFAULT);

	//
	// Look for the preference
	//

	SInt16 nValue = 0;
	CFNumberRef numPrefValue = (CFNumberRef) CFPreferencesCopyAppValue( strPrefName, CFSTR( kBundleName ) );
	if ( numPrefValue == NULL )
	{
		nValue = intDefault;
	}
	else
	{
		CFNumberGetValue( numPrefValue, kCFNumberSInt16Type, &nValue );
		fRelease = true;
	}

	//fprintf( stderr, "Reading Preference: Key = %s, Value = %d\n", lpszPrefName, nValue );
		
	*pintValue = nValue;

	if ( fRelease && numPrefValue != NULL )
	{
		CFRelease( numPrefValue );
	}
		
	if ( strPrefName != NULL )
	{
		CFRelease( strPrefName );
	}
}

void LoadSettingNumberLong(const char * lpszPrefName, long* pValue, long intDefault)
{
	bool fRelease = false;

	CFStringRef strPrefName = CFStringCreateWithCString(kCFAllocatorDefault, lpszPrefName, MY_ENCODING_DEFAULT);

	//
	// Look for the preference
	//

	long nValue = 0;
	CFNumberRef numPrefValue = (CFNumberRef) CFPreferencesCopyAppValue( strPrefName, CFSTR( kBundleName ) );
	if ( numPrefValue == NULL )
	{
		nValue = intDefault;
	}
	else
	{
		CFNumberGetValue( numPrefValue, kCFNumberLongType, &nValue );
		fRelease = true;
	}

	AMLOGDEBUG("Reading Preference: Key = %s, Value = %lu\n", lpszPrefName, nValue);
		
	*pValue = nValue;

	if ( fRelease && numPrefValue != NULL )
	{
		CFRelease( numPrefValue );
	}

	if ( strPrefName != NULL )
	{
		CFRelease( strPrefName );
	}
}

void LoadSettingString(const char * lpszPrefName, char * lpszValue, size_t cbValue, const char * lpszDefault)
{
	CFStringRef strPrefName = CFStringCreateWithCString(kCFAllocatorDefault, lpszPrefName, MY_ENCODING_DEFAULT);
	if (strPrefName)
	{
		CFStringRef strPrefValue = (CFStringRef) CFPreferencesCopyAppValue(strPrefName, CFSTR(kBundleName));
		if (!strPrefValue || CFStringGetLength(strPrefValue) == 0)
		{
			StringCbCopy(lpszValue, cbValue, lpszDefault);
		}
		else
		{
			CFStringGetCString(strPrefValue, lpszValue, cbValue, MY_ENCODING_DEFAULT);
		}
		
		AMLOGDEBUG("Reading Preference: Key = %s, Value = %s\n", lpszPrefName, lpszValue);

		if (strPrefValue)
		{
			CFRelease(strPrefValue);
		}

		CFRelease(strPrefName);
	}
}

void DoTwitterUiState(WindowRef wr)
{
	if (LoadMyPreferenceString(wr, CFSTR(MY_REGISTRY_VALUE_TWITTER_SCREENNAME), 'twit', 9, CFSTR(""), false))
	{
		ShowControl(GrabCRef(wr, 'twit', 8));
		ShowControl(GrabCRef(wr, 'twit', 9));
		ShowControl(GrabCRef(wr, 'twit', 11));
		
		HideControl(GrabCRef(wr, 'twit', 1));
		HideControl(GrabCRef(wr, 'twit', 2));
		HideControl(GrabCRef(wr, 'twit', 7));
		HideControl(GrabCRef(wr, 'twit', 10));
	}
	else
	{
		HideControl(GrabCRef(wr, 'twit', 8));
		HideControl(GrabCRef(wr, 'twit', 9));
		HideControl(GrabCRef(wr, 'twit', 11));
		
		ShowControl(GrabCRef(wr, 'twit', 1));
		ShowControl(GrabCRef(wr, 'twit', 2));
		ShowControl(GrabCRef(wr, 'twit', 7));
		ShowControl(GrabCRef(wr, 'twit', 10));
	}
}

bool IsExported(const char * lpszPrefName, time_t timetMarker)
{
	CFStringRef strPrefName = CFStringCreateWithFormat(NULL, NULL, CFSTR("Exported-%s"), lpszPrefName);
	if (strPrefName != NULL)
	{
		time_t timetValue = 0;
		CFNumberRef numPrefValue = (CFNumberRef) CFPreferencesCopyAppValue(strPrefName, CFSTR(kBundleName));
		if (numPrefValue != NULL)
		{
			CFNumberGetValue(numPrefValue, kCFNumberLongType, &timetValue);
			CFRelease(numPrefValue);

			if (timetValue > timetMarker)
			{
				if (g_intLogging == LOGGING_DEBUG)
				{
					fprintf(stderr, "IsExported %s at %lu vs. %lu -- true\n", lpszPrefName, timetValue, timetMarker);
				}

				return true;
			}
		}
	}

	fprintf(stderr, "IsExported %s -- false\n", lpszPrefName);
	
	return false;
}

void MarkExported(const char * lpszPrefName)
{
	CFStringRef strPrefName = CFStringCreateWithFormat(NULL, NULL, CFSTR("Exported-%s"), lpszPrefName);
	if (strPrefName != NULL)
	{
		time_t timetValue = time(NULL);
		CFNumberRef numPrefValue = CFNumberCreate(NULL, kCFNumberLongType, &timetValue);
		if (numPrefValue != NULL)
		{
			CFPreferencesSetAppValue(strPrefName, numPrefValue, CFSTR(kBundleName));
			CFPreferencesAppSynchronize(CFSTR(kBundleName));
			CFRelease(numPrefValue);
			
			if (g_intLogging == LOGGING_DEBUG)
			{
				fprintf(stderr, "MarkExported %s at %lu\n", lpszPrefName, timetValue);
			}
		}
	}
}

OSStatus FTP_Upload(const char * lpszFTPHost, const char * lpszFileRemote, const char * lpszFTPUser, const char * lpszFTPPassword, const char * lpszPathLocal)
{
	AMLOGDEBUG("Begin FTP_Upload for %s to %s\n", lpszPathLocal, lpszFileRemote);
	
	OSStatus err = EXIT_SUCCESS;
	char * filename = tmpnam(NULL);
	char * flags = "";
	char user[MAX_PATH] = {0};
	char password[MAX_PATH] = {0};

	EscapeForExpect(user, sizeof(user), lpszFTPUser);
	EscapeForExpect(password, sizeof(password), lpszFTPPassword);
	
	if (g_intLogging == LOGGING_DEBUG)
	{
		fprintf( stderr, "FTP script is %s\n", filename );	

		flags = "-v";
	}
	
	FILE* f = fopen(filename, "w");
	if (f)
	{		
		fprintf(f, "#!/usr/bin/expect\n");
		fprintf(f, "spawn ftp %s %s\n", flags, lpszFTPHost);
		fprintf(f, "expect \"220\"\n");
		fprintf(f, "send \"%s\\r\"\n", user);
		fprintf(f, "expect \"Password:\"\n");
		fprintf(f, "send \"%s\\r\"\n", password);
		fprintf(f, "expect \"ftp>\"\n");
		fprintf(f, "send \"put \\\"%s\\\" \\\"%s\\\"\\r\"\n", lpszPathLocal, lpszFileRemote);
		fprintf(f, "expect \"ftp>\"\n");
		fprintf(f, "send \"quit\\r\"\n");
		fprintf(f, "interact\n");

		fclose(f);
	}
	
	CFStringRef strCommand = CFStringCreateWithFormat(NULL, NULL, CFSTR("/usr/bin/expect -f %s"), filename);
	if (strCommand)
	{		
		char * cmd = CFStringToCString(strCommand, kCFStringEncodingMacRoman);
		if (cmd)
		{			
			int status = system(cmd);	
			if (status != 0)
			{
				fprintf(stderr, "Return value = %d\n", status);
				err = EXIT_FAILURE;
			}
			else
			{
				DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_FTP);
			}
			
			DisposePtr(cmd);
		}
		
		CFRelease(strCommand);
	}
	
	MyDeleteFile(filename);
	
	AMLOGDEBUG("End FTP_Upload\n");
	
	return err;
}

OSStatus SFTP_Upload(const char * lpszFTPHost, const char * lpszFileRemote, const char * lpszFTPUser, const char * lpszFTPPassword, const char * lpszPathLocal)
{
	AMLOGDEBUG("Begin SFTP_Upload for %s to %s\n", lpszPathLocal, lpszFileRemote);
	
	OSStatus err = EXIT_SUCCESS;
	char * filename = tmpnam(NULL);
	char * flags = "";
	char password[MAX_PATH] = {0};
	
	EscapeForExpect(password, sizeof(password), lpszFTPPassword);

	if (g_intLogging == LOGGING_DEBUG)
	{
		fprintf( stderr, "SFTP script is %s\n", filename );	
		
		flags = "-v";
	}
	
	FILE* f = fopen(filename, "w");
	if (f)
	{		
		fprintf(f, "#!/usr/bin/expect\n");
		fprintf(f, "spawn sftp %s -o \"StrictHostKeyChecking no\" %s@%s\n", flags, lpszFTPUser, lpszFTPHost);

		if (strlen(lpszFTPPassword))
		{
			fprintf(f, "expect \"password:\"\n");
			fprintf(f, "send \"%s\\r\"\n", password);
		}
		
		fprintf(f, "expect \"sftp>\"\n");
		fprintf(f, "send \"put \\\"%s\\\" \\\"%s\\\"\\r\"\n", lpszPathLocal, lpszFileRemote);
		fprintf(f, "expect \"sftp>\"\n");
		fprintf(f, "send \"quit\\r\"\n");
		fprintf(f, "interact\n");
		
		fclose(f);
	}

	CFStringRef strCommand = CFStringCreateWithFormat(NULL, NULL, CFSTR("/usr/bin/expect -f %s"), filename);
	if (strCommand)
	{		
		char * cmd = CFStringToCString(strCommand, kCFStringEncodingMacRoman);
		if (cmd)
		{			
			int status = system(cmd);	
			if (status != 0)
			{
				fprintf(stderr, "Return value = %d\n", status);
				err = EXIT_FAILURE;
			}
			else
			{
				DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_SFTP);
			}
			
			DisposePtr(cmd);
		}
			
		CFRelease(strCommand);
	}
	
	MyDeleteFile(filename);
	
	AMLOGDEBUG("End SFTP_Upload\n");
	
	return err;
}

OSStatus taskFunc(void *parameter)
{
	Boolean processingEvents = true;
	MPEventFlags theFlags;
    OSStatus err = noErr;
	int intNext = 0;

	if ( g_intLogging == LOGGING_DEBUG )
	{
		fprintf( stderr, "Begin taskFunc\n" );	
	}

	while ( processingEvents )
	{
		if ( g_intLogging == LOGGING_DEBUG )
		{
			fprintf( stderr, "Do MPWaitForEvent\n" );	
		}

		err = MPWaitForEvent( g_hSignalPlayEvent, &theFlags, kDurationForever );
		if ( err != noErr )
		{
			fprintf( stderr, "MPWaitForEvent failed with %lu!\n", err );
		}
	
		if ( g_intLogging == LOGGING_DEBUG )
		{
			fprintf( stderr, "MPWaitForEvent fired\n" );	
		}

		if ( ( theFlags & kNewSong ) != 0 )
		{
			if ( g_intLogging == LOGGING_DEBUG )
			{
				fprintf( stderr, "kNewSong\n" );	
			}
			
			DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_PLAY);

			intNext = g_intNext;
			
			//
			// ArtworkID
			//

			char szArtworkID[256] = {0};
			GetArtworkID(g_paPlaylist[intNext].track.artist, g_paPlaylist[intNext].track.album, &szArtworkID[0] );
			g_paPlaylist[intNext].strArtworkId = CFStringCreateWithCString( kCFAllocatorDefault, &szArtworkID[0], 0 );

			//
			// Amazon
			//

			if ( g_paPlaylist[intNext].fPlay && g_intAmazonLookup > 0 )
			{
				if ( g_paPlaylist[ intNext ].track.isCompilationTrack == TRUE )
				{
					DoAmazonLookup(g_paPlaylist[intNext].track.grouping, NULL, g_paPlaylist[intNext].track.album, &g_paPlaylist[intNext].strDetailPageURL, &g_paPlaylist[intNext].strSmallImageUrl, &g_paPlaylist[intNext].strMediumImageUrl, &g_paPlaylist[intNext].strLargeImageUrl);
				}
				else
				{
					DoAmazonLookup(g_paPlaylist[intNext].track.grouping, g_paPlaylist[intNext].track.artist, g_paPlaylist[intNext].track.album, &g_paPlaylist[intNext].strDetailPageURL, &g_paPlaylist[intNext].strSmallImageUrl, &g_paPlaylist[intNext].strMediumImageUrl, &g_paPlaylist[intNext].strLargeImageUrl );					
				}
			}

			//
			// Apple
			//

			if (g_paPlaylist[intNext].fPlay && g_intAppleLookup > 0)
			{
				DoAppleLookup(g_paPlaylist[intNext].track.artist, g_paPlaylist[intNext].track.album, g_paPlaylist[intNext].track.name, &g_paPlaylist[intNext].strAppleURL);			
			}

			//
			// Write out XML file
			//

			WriteXML( true, intNext );

			//
			// Export artwork
			//
			
			if ( g_paPlaylist[intNext].fPlay && ( g_intArtworkUpload > 0 || g_intArtworkExport > 0 ) && !IsExported(szArtworkID, g_timetExportedMarker) )
			{
				LoadedScriptInfoPtr scriptInfo = NULL;				
				err = LoadCompiledScript( CFSTR( kBundleName ), CFSTR("NowPlaying"), &scriptInfo );
				if ( err == noErr )
				{			
					CFStringRef strArtworkPathLocal;
					AEDescList theResult;
					
					err = script_exportArtwork( scriptInfo, &theResult );
					if (err != noErr)
					{
						fprintf(stderr, "Error %ld in script_exportArtwork\n", err);
					}

					// Free the scripts
					UnloadCompiledScript( scriptInfo );
					scriptInfo = NULL;
					
					OSStatus err1 = AEDescToCFString( &theResult, &strArtworkPathLocal );
					if ( err1 == 0 )
					{
						if ( CFStringGetLength( strArtworkPathLocal ) > 0 )
						{
							char szArtworkPathLocal[128] = {0};
							CFStringGetCString( strArtworkPathLocal, &szArtworkPathLocal[0], sizeof( szArtworkPathLocal ), 0 );
							
							if ( g_intLogging == LOGGING_DEBUG )
							{
								fprintf( stderr, "ExportedArtwork = %s\n", szArtworkPathLocal );	
							}
							
							AEDisposeDesc( &theResult );

							if ( err == noErr )
							{
								if ( g_intArtworkUpload > 0 )
								{
									char szArtworkFileRemote[256] = {0};
									strcpy( szArtworkFileRemote, g_szFTPPath );
									
									char* lpszEnd =  strrchr( szArtworkFileRemote, '/' );
									if ( lpszEnd )
									{
										lpszEnd++;
										*lpszEnd = '\0';
									}
									
									char *lpszFileNameOnly = strrchr( szArtworkPathLocal, '/' );
									
									if ( lpszFileNameOnly )
									{
										lpszFileNameOnly++;
										strcat( szArtworkFileRemote, lpszFileNameOnly );
									}
									
									if ( g_intUploadProtocol == 2 )
									{
										if ( FTP_Upload( g_szFTPHost, szArtworkFileRemote, g_szFTPUser, g_szFTPPassword, szArtworkPathLocal ) == EXIT_SUCCESS)
										{
											MarkExported(szArtworkID);
										}
										else
										{
											if ( g_intLogging == LOGGING_DEBUG )
											{
												fprintf( stderr, "FTP_Upload failed for artwork\n" );
											}
										}
									}
									else if ( g_intUploadProtocol == 3 )
									{
										if ( SFTP_Upload( g_szFTPHost, szArtworkFileRemote, g_szFTPUser, g_szFTPPassword, szArtworkPathLocal ) == EXIT_SUCCESS)
										{
											MarkExported(szArtworkID);
										}
										else
										{
											if ( g_intLogging == LOGGING_DEBUG )
											{
												fprintf( stderr, "SFTP_Upload failed for artwork\n" );
											}
										}
									}									
								}
								
								//
								// Delete the artwork when done uploading if we don't want it local
								//
								
								if ( g_intArtworkExport == 0 )
								{
									MyDeleteFile( szArtworkPathLocal );
								}
							}
						}
						
						if ( strArtworkPathLocal )
						{
							CFRelease( strArtworkPathLocal );
						}
					}
					else
					{
						fprintf( stderr, "Can't convert description to string %ld\n", err1 );
					}						
				}
				else
				{
					fprintf( stderr, "Unable to load compiled script!\n" );
				}
			}
			
			//
			// Upload the XML file
			//

			if ( g_intUploadProtocol == 2 )
			{
				FTP_Upload( g_szFTPHost, g_szFTPPath, g_szFTPUser, g_szFTPPassword, g_szOutputFile );
			}
			else if ( g_intUploadProtocol == 3 )
			{				
				SFTP_Upload( g_szFTPHost, g_szFTPPath, g_szFTPUser, g_szFTPPassword, g_szOutputFile );
			}

			//
			// Do ping
			//

			DoPing( true, intNext );
			
			//
			// Do Twitter
			//
			
			if ( g_intTwitterEnabled && strlen( g_szTwitterAuthKey ) > 0 && strlen( g_szTwitterAuthSecret ) > 0 )
			{				
				time_t now = time(NULL);
				time_t then =  g_timetTwitterLatest + g_intTwitterRateLimitMinutes * 60;
				
				if (now > then)
				{
					DoTwitter( intNext );
				}
				else
				{					
					if (g_intLogging == LOGGING_DEBUG)
					{
						fprintf(stderr, "Rate limited at %d minutes; %lu seconds until resume; Skipping Twitter\n", g_intTwitterRateLimitMinutes, then - now);
					}					
				}
			}
			
			//
			// Do Facebook
			//

			if (g_intFacebookEnabled && strlen( g_szFacebookSessionKey ) > 0)
			{
				time_t now = time(NULL);
				time_t then =  g_timetFacebookLatest + g_intFacebookRateLimitMinutes * 60;
				
				if (now > then)
				{
					DoFacebookStream(intNext);
				}
				else
				{					
					fprintf(stderr, "Rate limited at %d minutes; %lu seconds until resume; Skipping Facebook\n", g_intFacebookRateLimitMinutes, then - now);					
				}
			}			
		}

		//
		// If iTunes is closing
		//
		
		if ( ( theFlags & kSelfDestruct ) != 0 )
		{
			processingEvents = false;
			
			if ( g_intLogging == LOGGING_DEBUG )
			{
				fprintf( stderr, "taskFunc stop processing events\n" );	
			}
		}
	}

	//
	// If we ever played at least one song, then tell everyone we stopped.
	//

	if ( g_intOnce > 0 &  g_intPublishStop > 0 )
	{
		//
		// Write out XML file
		//

		WriteXML( false, 0 );

		//
		// Upload the XML file
		//

		if ( g_intUploadProtocol == 2 )
		{
			FTP_Upload( g_szFTPHost, g_szFTPPath, g_szFTPUser, g_szFTPPassword, g_szOutputFile );
		}
		else if ( g_intUploadProtocol == 3 )
		{
			if ( g_intLogging == LOGGING_DEBUG )
			{
				fprintf( stderr, "Begin SFTP_Upload\n" );
			}
			
			SFTP_Upload( g_szFTPHost, g_szFTPPath, g_szFTPUser, g_szFTPPassword, g_szOutputFile );
			
			if ( g_intLogging == LOGGING_DEBUG )
			{
				fprintf( stderr, "End SFTP_Upload\n" );
			}
		}		

		//
		// Do ping
		//
		
		DoPing( false, intNext );
	}

	//
	// Signal main thread to proceed with cleanup
	//

	MPSetEvent( g_hSignalCleanupEvent, kCompleted );
	
	if ( g_intLogging == LOGGING_DEBUG )
	{
		fprintf( stderr, "End taskFunc\n" );	
	}

	return noErr;
}

void CacheMySettings(bool fFirst)
{
	char szTemp[MAX_PATH] = {0};
	
	if ( fFirst )
	{
		LoadSettingNumber( MY_REGISTRY_VALUE_PLAYLIST_LENGTH, &g_intPlaylistLength, DEFAULT_PLAYLIST_LENGTH );
	}

	LoadSettingString( MY_REGISTRY_VALUE_GUID, g_szGuid, sizeof(g_szGuid), "" );
	LoadSettingString( MY_REGISTRY_VALUE_EMAIL, g_szEmail, sizeof(g_szEmail), "" );
	LoadSettingString( MY_REGISTRY_VALUE_LICENSE_KEY, g_szLicenseKey, sizeof(g_szLicenseKey), "" );
	LoadSettingString( MY_REGISTRY_VALUE_FILE, g_szOutputFile, sizeof(g_szOutputFile), "/tmp/now_playing.xml" );
	LoadSettingString( MY_REGISTRY_VALUE_HOST, g_szFTPHost, sizeof(g_szFTPHost), "" );
	LoadSettingString( MY_REGISTRY_VALUE_USER, g_szFTPUser, sizeof(g_szFTPUser), "" );
	LoadSettingString( MY_REGISTRY_VALUE_PASSWORD, szTemp, sizeof(szTemp), "" );
	LoadSettingString( MY_REGISTRY_VALUE_PATH, g_szFTPPath, sizeof(g_szFTPPath), "/tmp/now_playing.xml" );
	LoadSettingNumber( MY_REGISTRY_VALUE_PROTOCOL, &g_intUploadProtocol, 1 );
	LoadSettingNumber( MY_REGISTRY_VALUE_XML_ENCODING, &g_intXmlEncoding, 1 );
	LoadSettingNumber( MY_REGISTRY_VALUE_XML_CDATA, &g_intUseXmlCData, 1 );
	LoadSettingNumber( MY_REGISTRY_VALUE_ARTWORK_UPLOAD, &g_intArtworkUpload, DEFAULT_ARTWORK_UPLOAD );
	LoadSettingNumber( MY_REGISTRY_VALUE_ARTWORK_WIDTH, &g_intArtworkWidth, DEFAULT_ARTWORK_WIDTH );
	LoadSettingNumber( MY_REGISTRY_VALUE_PUBLISH_STOP, &g_intPublishStop, DEFAULT_PUBLISH_STOP );
	LoadSettingNumber( MY_REGISTRY_VALUE_ARTWORK_EXPORT, &g_intArtworkExport, DEFAULT_ARTWORK_EXPORT );
	LoadSettingNumber( MY_REGISTRY_VALUE_SKIPSHORT, &g_intSkipShort, DEFAULT_SKIPSHORT_SECONDS );
	LoadSettingString( MY_REGISTRY_VALUE_SKIPKINDS, g_szSkipKinds, sizeof(g_szSkipKinds), "" );
	LoadSettingString( MY_REGISTRY_VALUE_TRACKBACK_URL, g_szTrackBackUrl, sizeof(g_szTrackBackUrl), "" );
	LoadSettingString( MY_REGISTRY_VALUE_PING_EXTRA_INFO, g_szPingExtraInfo, sizeof(g_szPingExtraInfo), "" );
	LoadSettingNumber( MY_REGISTRY_VALUE_AMAZON_ENABLED, &g_intAmazonLookup, DEFAULT_AMAZON_ENABLED );
	LoadSettingNumber( MY_REGISTRY_VALUE_AMAZON_USE_ASIN, &g_intAmazonUseASIN, DEFAULT_AMAZON_USE_ASIN );
	LoadSettingNumber( MY_REGISTRY_VALUE_AMAZON_LOCALE, &g_intAmazonLocale, AMAZON_LOCALE_US );
	LoadSettingString( MY_REGISTRY_VALUE_AMAZON_ASSOCIATE, g_szAmazonAssociate, sizeof(g_szAmazonAssociate), MY_AMAZON_ASSOCIATE_ID );
	LoadSettingNumber( MY_REGISTRY_VALUE_APPLE_ENABLED, &g_intAppleLookup, DEFAULT_APPLE_ENABLED );
	LoadSettingString( MY_REGISTRY_VALUE_APPLE_ASSOCIATE, g_szAppleAssociate, sizeof(g_szAppleAssociate), DEFAULT_APPLE_ASSOCIATE );
	LoadSettingNumberLong( MY_REGISTRY_VALUE_UPDATE_TIMESTAMP, &g_timeLastUpdateCheck, 0 );
	LoadSettingNumberLong( MY_REGISTRY_VALUE_EXPORT_TIMESTAMP, &g_timetExportedMarker, 0 );
	LoadSettingNumber( MY_REGISTRY_VALUE_LOGGING, &g_intLogging, DEFAULT_LOG_LEVEL );
	LoadSettingNumber( MY_REGISTRY_VALUE_FACEBOOK_ENABLED, &g_intFacebookEnabled, 0 );
	LoadSettingString( MY_REGISTRY_VALUE_FACEBOOK_MESSAGE, g_szFacebookMessage, sizeof(g_szFacebookMessage), "" );
	LoadSettingString( MY_REGISTRY_VALUE_FACEBOOK_ATTACHMENT_DESCRIPTION, g_szFacebookAttachmentDescription, sizeof(g_szFacebookAttachmentDescription), "" );
	LoadSettingString( MY_REGISTRY_VALUE_FACEBOOK_SESSION_KEY, g_szFacebookSessionKey, sizeof(g_szFacebookSessionKey), "" );
	LoadSettingNumber( MY_REGISTRY_VALUE_FACEBOOK_RATE_LIMIT_MINUTES, &g_intFacebookRateLimitMinutes, FACEBOOK_RATE_LIMIT_MINUTES_DEFAULT );
	LoadSettingNumberLong( MY_REGISTRY_VALUE_FACEBOOK_LATEST, &g_timetFacebookLatest, 0 );
	LoadSettingNumber( MY_REGISTRY_VALUE_TWITTER_ENABLED, &g_intTwitterEnabled, 0 );
	LoadSettingString( MY_REGISTRY_VALUE_TWITTER_MESSAGE, g_szTwitterMessage, sizeof(g_szTwitterMessage), "" );
	LoadSettingString( MY_REGISTRY_VALUE_TWITTER_KEY, g_szTwitterAuthKey, sizeof(g_szTwitterAuthKey), "" );
	LoadSettingString( MY_REGISTRY_VALUE_TWITTER_SECRET, g_szTwitterAuthSecret, sizeof(g_szTwitterAuthSecret), "" );
	LoadSettingNumber( MY_REGISTRY_VALUE_TWITTER_RATE_LIMIT_MINUTES, &g_intTwitterRateLimitMinutes, TWITTER_RATE_LIMIT_MINUTES_DEFAULT );
	LoadSettingNumberLong( MY_REGISTRY_VALUE_TWITTER_LATEST, &g_timetTwitterLatest, 0 );	
		
	if (strchr(g_szAmazonAssociate, '@') != NULL)
	{
		strcpy(g_szAmazonAssociate, MY_AMAZON_ASSOCIATE_ID);
	}
	
	if ( szTemp[0] != '\0' )
	{
		DecryptString( szTemp, g_szFTPPassword, MAX_PATH );
	}

	g_fLicensed = IsLicensed( MY_SECRET_KEY, g_szEmail, MY_REGISTRY_KEY, g_szLicenseKey );
	
	if (strlen(g_szGuid) == 0)
	{
		CFUUIDRef uuid = CFUUIDCreate(kCFAllocatorDefault);
		if (uuid)
		{
			CFStringRef strUuid = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@"), uuid);
			if (strUuid)
			{
				CFStringGetCString(strUuid, g_szGuid, sizeof(g_szGuid), kCFStringEncodingUTF8);
								
				CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_GUID), strUuid, CFSTR(kBundleName));
				CFPreferencesAppSynchronize(CFSTR(kBundleName));

				CFRelease(strUuid);
			}
			
			CFRelease(uuid);
		}
	}
}

void Cleanup()
{
	MPEventFlags theFlags;
    OSStatus err = noErr;

	if ( g_intLogging == LOGGING_DEBUG )
	{
		fprintf( stderr, "Begin Cleanup\n" );	
	}

	//
	// Tell worker to stop and exit
	//

	MPSetEvent( g_hSignalPlayEvent, kSelfDestruct );

	//
	// Wait for worker to signal that it has completed; Give it a reasonable amount of time to complete
	//

	err = MPWaitForEvent( g_hSignalCleanupEvent, &theFlags, kDurationMillisecond * 1000 * 30 );
	if (err == noErr && ( theFlags & kCompleted ) != 0 )
	{
		if ( g_intLogging == LOGGING_DEBUG )
		{
			fprintf( stderr, "Task Cleanup Completed\n" );
		}

		if ( g_paPlaylist )
		{
			free( g_paPlaylist );
			g_paPlaylist = NULL;
		}
	}
	else
	{
		if ( g_intLogging == LOGGING_DEBUG )
		{
			fprintf( stderr, "Task Cleanup NOT Completed %lu\n", err );
		}
	}

	//
	// Cleanup events
	//

	MPDeleteEvent( g_hSignalPlayEvent );
	MPDeleteEvent( g_hSignalCleanupEvent );

	if ( g_intLogging == LOGGING_DEBUG )
	{
		fprintf( stderr, "End Cleanup\n" );	
	}	
}

bool IsFirstTime()
{
	int intValue = 0;

	LoadSettingNumber( MY_REGISTRY_VALUE_CONFIGURED, &intValue, 0 );
	
	if ( intValue == 0 )
	{
		return true;
	}
	else
	{
		return false;
	}
}

pascal OSStatus settingsControlHandler(EventHandlerCallRef inRef, EventRef inEvent, void* userData)
{
    WindowRef wr = NULL;
    ControlID controlID;
    ControlRef control = NULL;

	//
    // Get control hit by event
	//
	
    GetEventParameter( inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof( ControlRef ), NULL, &control );
    wr = GetControlOwner( control );
    GetControlID( control, &controlID );

	if ( controlID.signature == 'twit' && controlID.id == 1 )
	{
		SetKeyboardFocus(wr, GrabCRef(wr, 'twit', 2), kControlFocusNextPart);

		DoTwitterAuthorize();
	}
	else if ( controlID.signature == 'twit' && controlID.id == 7 )
	{
		CFStringRef strEmpty = CFSTR("");
		CFStringRef strPin;
		Size actualSize = 0;

		GetControlData(GrabCRef( wr, 'twit', 2 ), kControlEditTextPart, kControlEditTextCFStringTag, sizeof( strPin ), &strPin, &actualSize );

		DoTwitterVerify(strPin);
		
		SetControlData(GrabCRef(wr, 'twit', 2), kControlEditTextPart, kControlEditTextCFStringTag, sizeof(strEmpty), &strEmpty);
		
		DoTwitterUiState(wr);
	}
	else if ( controlID.signature == 'twit' && controlID.id == 6 )
	{
		CFStringRef strValue = CFSTR(TWITTER_MESSAGE_DEFAULT);
		SetControlData(GrabCRef(wr, 'twit', 0), kControlEditTextPart, kControlEditTextCFStringTag, sizeof(strValue), &strValue);
	}
	else if ( controlID.signature == 'twit' && controlID.id == 8 )
	{
		DoTwitterReset();
		
		DoTwitterUiState(wr);
	}
	else if ( controlID.signature == 'face' && controlID.id == 1 )
	{
		DoFacebookAdd();
	}
	else if ( controlID.signature == 'face' && controlID.id == 3 )
	{
		DoFacebookAuthorize();
		
		DoFacebookUiState(wr);
	}
	else if ( controlID.signature == 'face' && controlID.id == 6 )
	{
		CFStringRef strValue = CFSTR(FACEBOOK_MESSAGE_DEFAULT);
		SetControlData(GrabCRef(wr, 'face', 0), kControlEditTextPart, kControlEditTextCFStringTag, sizeof(strValue), &strValue);
	}
	else if ( controlID.signature == 'face' && controlID.id == 32 )
	{
		DoFacebookReset();
		
		DoFacebookUiState(wr);
	}
	else if ( controlID.signature == 'face' && controlID.id == 40 )
	{
		// If they changed status <-> news, update UI to active description field

		DoFacebookUiState(wr);
	}
	else if ( controlID.signature == 'uplo' && controlID.id == 7 )
	{
		//
		// Set marker to now
		//

		g_timetExportedMarker = time(NULL);
		CFNumberRef numPrefValue = CFNumberCreate(NULL, kCFNumberLongType, &g_timetExportedMarker);
		if (numPrefValue != NULL)
		{
			CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_EXPORT_TIMESTAMP), numPrefValue, CFSTR(kBundleName));
			CFPreferencesAppSynchronize(CFSTR(kBundleName));
			CFRelease(numPrefValue);
		}
	}
	else if ( controlID.signature == 'main' && controlID.id == 1 )
	{
		//
		// Get the values
		//

		CFStringRef strEmail;
		CFStringRef strLicenseKey;
		Size actualSize = 0;
			
		GetControlData( GrabCRef( wr, 'lice', 1 ), kControlEditTextPart, kControlEditTextCFStringTag, sizeof( strEmail ), &strEmail, &actualSize );
		GetControlData( GrabCRef( wr, 'lice', 2 ), kControlEditTextPart, kControlEditTextCFStringTag, sizeof( strLicenseKey ), &strLicenseKey, &actualSize );
			
		//
		// Validate: If validate fails, send true.
		//
					
		if ( CFStringGetLength( strEmail ) > 0 && CFStringGetLength( strLicenseKey ) > 0 )
		{
			char szEmail[128];
			char szLicenseKey[128];
			CFStringGetCString( strEmail, &szEmail[0], 128, 0 );
			CFStringGetCString( strLicenseKey, &szLicenseKey[0], 128, 0 );
			if ( !IsLicensed( MY_SECRET_KEY, szEmail, MY_REGISTRY_KEY, szLicenseKey ) )
			{
				DialogRef alertDialog;
				CreateStandardAlert( kAlertStopAlert, CFSTR("Invalid license key. Try again."), NULL, NULL, &alertDialog );
				RunStandardAlert( alertDialog, NULL, NULL );
			
				return noErr;
			}
		}
		
		SaveMyPreferences( wr );

		HideWindow( wr );
	}
	else if ( controlID.signature == 'main' && controlID.id == 2 )
	{
		HideWindow( wr );
    }

    return noErr;
}

void DoPropertySheet()
{
	static EventTypeSpec controlEvent = { kEventClassControl, kEventControlHit };
	static WindowRef settingsDialog = NULL;
	
	if ( settingsDialog == NULL )
	{
		CFBundleRef theBundle = CFBundleGetBundleWithIdentifier( CFSTR( kBundleName ) );
		if ( theBundle != NULL )
		{
			IBNibRef nibRef = NULL;
			if ( CreateNibReferenceWithCFBundle( theBundle, CFSTR("SettingsDialog"), &nibRef ) == noErr )
			{                                 
				CreateWindowFromNib( nibRef, CFSTR("PluginSettings"), &settingsDialog );
				DisposeNibReference( nibRef );
				InstallWindowEventHandler( settingsDialog, NewEventHandlerUPP( settingsControlHandler ), 1, &controlEvent, 0, NULL );
					
				// Events for the Tab control that I will assign a handler for
				EventTypeSpec tabControlEvents[] = { { kEventClassControl, kEventControlHit }, { kEventClassCommand, kEventCommandProcess } };
					
				// Installing the handler, and passing in my windowref as the refCon for later use
				InstallControlEventHandler( GrabCRef( settingsDialog, kTabMasterSig, kTabMasterID ), PrefsTabEventHandlerProc, GetEventTypeCount( tabControlEvents ), tabControlEvents, settingsDialog, NULL );
				
				// Set the initial tab state
				SetInitialTabState( settingsDialog );

				// Look for the bundle’s version number and update display with it
				CFTypeRef strVersionBundle = CFBundleGetValueForInfoDictionaryKey( theBundle, CFSTR("CFBundleShortVersionString") );
				if ( strVersionBundle != NULL )
				{
					CFRetain( strVersionBundle );
					
					if ( CFGetTypeID( strVersionBundle ) == CFStringGetTypeID() )
					{
						CFStringRef strVersionLabel = CFStringCreateWithFormat( NULL, NULL, CFSTR("Version: %@"), strVersionBundle );
						if ( strVersionLabel != NULL )
						{
							SetControlData( GrabCRef( settingsDialog, 'abou', 1 ), 0, kControlStaticTextCFStringTag, sizeof( strVersionLabel ), &strVersionLabel );

							CFRelease( strVersionLabel );
						}
					}

					CFRelease( strVersionBundle );
				}
			}
		}
	}
			
	// Show the dialog now that it should be created
	if ( settingsDialog != NULL )
	{
		// Load up the values in the fields
		LoadMyPreferences(settingsDialog);
		
		DoTwitterUiState(settingsDialog);
		DoFacebookUiState(settingsDialog);

		// Make visible
		ShowWindow(settingsDialog);
	}
}

void CacheVersion()
{
	CFBundleRef theBundle = CFBundleGetBundleWithIdentifier( CFSTR( kBundleName ) );
	if ( theBundle )
	{
		CFStringRef strVersionMy = CFBundleGetValueForInfoDictionaryKey( theBundle, CFSTR("CFBundleShortVersionString") );
		if ( strVersionMy )
		{
			CFRetain( strVersionMy );
			
			CFStringGetCString( strVersionMy, &g_szVersion[0], sizeof(g_szVersion), kCFStringEncodingUTF8 );
			
			CFRelease( strVersionMy );
		}
	}
}

void DoInit()
{
	int i = 0;

	CacheMySettings( true );
	CacheVersion();
	
	g_paPlaylist = (PlaylistItem*) calloc( g_intPlaylistLength, sizeof( PlaylistItem ) );
	
	for ( i = 0; i < g_intPlaylistLength; i++ )
	{
		memset( &g_paPlaylist[ i ].track, 0, sizeof( ITTrackInfo ) );

		g_paPlaylist[ i ].fSet = false;
		g_paPlaylist[ i ].fPlay = false;
	
		g_paPlaylist[ i ].strDetailPageURL = NULL;
		g_paPlaylist[ i ].strSmallImageUrl = NULL;
		g_paPlaylist[ i ].strMediumImageUrl = NULL;
		g_paPlaylist[ i ].strLargeImageUrl = NULL;
		g_paPlaylist[ i ].strAppleURL = NULL;
	}

	MPCreateEvent( &g_hSignalPlayEvent );
	MPCreateEvent( &g_hSignalCleanupEvent );

	MPTaskID taskID;
	MPCreateTask( taskFunc, NULL, 2 * 1024 * 1024, 0, NULL, NULL, 0, &taskID );
	
	if ( IsFirstTime() )
	{
		DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_INSTALL);
		
		DialogRef alertDialog;
		CreateStandardAlert( kAlertNoteAlert, CFSTR("It appears that this is the first time you have used the Now Playing plugin. The configuration window will appear now so you can set your preferences."), NULL, NULL, &alertDialog );
		RunStandardAlert( alertDialog, NULL, NULL );
		
		DoPropertySheet();
	}
	else
	{
		DoSoftwareUpdateCheck();
	}
	
	DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_LAUNCH);
}

static OSStatus VisualPluginHandler(OSType message,VisualPluginMessageInfo *messageInfo,void *refCon)
{
	OSStatus status;
	VisualPluginData * visualPluginData;

	visualPluginData = (VisualPluginData*) refCon;
	
	status = noErr;

	if ( g_intLogging == LOGGING_DEBUG )
	{
		fprintf( stderr, "Begin VisualPluginHandler(%lu)\n", message );	
	}

	switch ( message )
	{
		/*
			Sent when the visual plugin is registered.  The plugin should do minimal
			memory allocations here.  The resource fork of the plugin is still available.
		*/		
		case kVisualPluginInitMessage:
		{
			visualPluginData = (VisualPluginData*) NewPtrClear(sizeof(VisualPluginData));
			if (visualPluginData == nil)
			{
				status = memFullErr;
				break;
			}

			visualPluginData->appCookie	= messageInfo->u.initMessage.appCookie;
			visualPluginData->appProc	= messageInfo->u.initMessage.appProc;

			/* Remember the file spec of our plugin file. We need this so we can open our resource fork during */
			/* the configuration message */
			
			status = PlayerGetPluginFileSpec(visualPluginData->appCookie, visualPluginData->appProc, &visualPluginData->pluginFileSpec);

			messageInfo->u.initMessage.refCon = (void*) visualPluginData;

			break;
		}
			
		/*
			Sent when the visual plugin is unloaded
		*/		
		case kVisualPluginCleanupMessage:
			if (visualPluginData != nil)
				DisposePtr((Ptr)visualPluginData);
			break;
			
		/*
			Sent when the visual plugin is enabled.  iTunes currently enables all
			loaded visual plugins.  The plugin should not do anything here.
		*/
		case kVisualPluginEnableMessage:
		case kVisualPluginDisableMessage:
			break;

		/*
			Sent if the plugin requests idle messages.  Do this by setting the kVisualWantsIdleMessages
			option in the RegisterVisualMessage.options field.
		*/
		case kVisualPluginIdleMessage:
			break;
					
		/*
			Sent if the plugin requests the ability for the user to configure it.  Do this by setting
			the kVisualWantsConfigure option in the RegisterVisualMessage.options field.
		*/
		case kVisualPluginConfigureMessage:
		{
			DoPropertySheet();

			break;
		}

		/*
			Sent when iTunes is going to show the visual plugin in a port.  At
			this point,the plugin should allocate any large buffers it needs.
		*/
		case kVisualPluginShowWindowMessage:
			break;
			
		/*
			Sent when iTunes is no longer displayed.
		*/
		case kVisualPluginHideWindowMessage:
		{
			MemClear( &visualPluginData->trackInfo, sizeof( visualPluginData->trackInfo ) );
			MemClear( &visualPluginData->streamInfo, sizeof( visualPluginData->streamInfo ) );
			break;
		}
		
		/*
			Sent when iTunes needs to change the port or rectangle of the currently
			displayed visual.
		*/
		case kVisualPluginSetWindowMessage:
		{
			visualPluginData->destOptions = messageInfo->u.setWindowMessage.options;
			break;
		}
		
		/*
			Sent for the visual plugin to render a frame.
		*/
		case kVisualPluginRenderMessage:
			break;

		/*
			Sent in response to an update event.  The visual plugin should update
			into its remembered port.  This will only be sent if the plugin has been
			previously given a ShowWindow message.
		*/	
		case kVisualPluginUpdateMessage:
			break;
		
		/*
			Sent when the player starts.
		*/
		case kVisualPluginPlayMessage:
		{			
			if (messageInfo->u.playMessage.trackInfo != nil)
			{
				visualPluginData->trackInfo = *messageInfo->u.playMessage.trackInfoUnicode;
			}
			else
			{
				MemClear(&visualPluginData->trackInfo,sizeof(visualPluginData->trackInfo));
			}
			
			if (messageInfo->u.playMessage.streamInfo != nil)
			{
				visualPluginData->streamInfo = *messageInfo->u.playMessage.streamInfoUnicode;
			}
			else
			{
				MemClear(&visualPluginData->streamInfo,sizeof(visualPluginData->streamInfo));
			}
			
			visualPluginData->playing = true;
		
			DoUpdate( &visualPluginData->trackInfo, true );
		
			break;
		}

		/*
			Sent when the player changes the current track information.  This
			is used when the information about a track changes,or when the CD
			moves onto the next track.  The visual plugin should update any displayed
			information about the currently playing song.
		*/
		case kVisualPluginChangeTrackMessage:
		{
			if (messageInfo->u.changeTrackMessage.trackInfo != nil)
			{
				visualPluginData->trackInfo = *messageInfo->u.changeTrackMessage.trackInfoUnicode;
			}
			else
			{
				MemClear(&visualPluginData->trackInfo,sizeof(visualPluginData->trackInfo));
			}
			
			if (messageInfo->u.changeTrackMessage.streamInfo != nil)
			{
				visualPluginData->streamInfo = *messageInfo->u.changeTrackMessage.streamInfoUnicode;
			}
			else
			{
				MemClear(&visualPluginData->streamInfo,sizeof(visualPluginData->streamInfo));
			}
			
			DoUpdate( &visualPluginData->trackInfo, true );

			break;
		}

		/*
			Sent when the player stops.
		*/
		case kVisualPluginStopMessage:
		{
			visualPluginData->playing = false;
						
			break;
		}
		
		/*
			Sent when the player changes position.
		*/
		case kVisualPluginSetPositionMessage:
			break;

		/*
			Sent when the player pauses.  iTunes does not currently use pause or unpause.
			A pause in iTunes is handled by stopping and remembering the position.
		*/
		case kVisualPluginPauseMessage:
			visualPluginData->playing = false;
			break;
			
		/*
			Sent when the player unpauses.  iTunes does not currently use pause or unpause.
			A pause in iTunes is handled by stopping and remembering the position.
		*/
		case kVisualPluginUnpauseMessage:
			visualPluginData->playing = true;
			break;
		
		/*
			Sent to the plugin in response to a MacOS event.  The plugin should return noErr
			for any event it handles completely,or an error (unimpErr) if iTunes should handle it.
		*/
		case kVisualPluginEventMessage:
			break;

		default:
			status = unimpErr;
			break;
	}
	
	if ( g_intLogging == LOGGING_DEBUG )
	{
		fprintf( stderr, "End VisualPluginHandler\n" );
	}

	return status;	
}

static OSStatus RegisterVisualPlugin(PluginMessageInfo *messageInfo)
{
	OSStatus			status;
	PlayerMessageInfo	playerMessageInfo;

	MemClear( &playerMessageInfo.u.registerVisualPluginMessage, sizeof(playerMessageInfo.u.registerVisualPluginMessage ) );	

	CFStringGetPascalString( CFSTR( kTVisualPluginName ), (Ptr) &playerMessageInfo.u.registerVisualPluginMessage.name[0], 64, 0 );

	SetNumVersion( &playerMessageInfo.u.registerVisualPluginMessage.pluginVersion, kTVisualPluginMajorVersion, kTVisualPluginMinorVersion, kTVisualPluginReleaseStage, kTVisualPluginNonFinalRelease );

	//playerMessageInfo.u.registerVisualPluginMessage.options					= kVisualWantsIdleMessages | kVisualWantsConfigure;
	playerMessageInfo.u.registerVisualPluginMessage.options					= kVisualWantsConfigure;
	playerMessageInfo.u.registerVisualPluginMessage.handler					= (VisualPluginProcPtr) VisualPluginHandler;
	playerMessageInfo.u.registerVisualPluginMessage.registerRefCon			= 0;
	playerMessageInfo.u.registerVisualPluginMessage.creator					= kTVisualPluginCreator;
	
	playerMessageInfo.u.registerVisualPluginMessage.timeBetweenDataInMS		= 0xFFFFFFFF; // 16 milliseconds = 1 Tick, 0xFFFFFFFF = Often as possible.
	playerMessageInfo.u.registerVisualPluginMessage.numWaveformChannels		= 2;
	playerMessageInfo.u.registerVisualPluginMessage.numSpectrumChannels		= 2;
	
	playerMessageInfo.u.registerVisualPluginMessage.minWidth				= 64;
	playerMessageInfo.u.registerVisualPluginMessage.minHeight				= 64;
	playerMessageInfo.u.registerVisualPluginMessage.maxWidth				= 32767;
	playerMessageInfo.u.registerVisualPluginMessage.maxHeight				= 32767;
	playerMessageInfo.u.registerVisualPluginMessage.minFullScreenBitDepth	= 0;
	playerMessageInfo.u.registerVisualPluginMessage.maxFullScreenBitDepth	= 0;
	playerMessageInfo.u.registerVisualPluginMessage.windowAlignmentInBytes	= 0;
	
	status = PlayerRegisterVisualPlugin(messageInfo->u.initMessage.appCookie,messageInfo->u.initMessage.appProc,&playerMessageInfo);
		
	return status;
}

extern OSStatus iTunesPluginMainMachO(OSType message, PluginMessageInfo* messageInfo, void* refCon)
{
	OSStatus status = noErr;
	
	(void) refCon;
	
	switch ( message )
	{
		case kPluginInitMessage:
			status = RegisterVisualPlugin( messageInfo );
			DoInit();
			break;
			
		case kPluginCleanupMessage:
			Cleanup();
			status = noErr;
			break;
			
		default:
			status = unimpErr;
			break;
	}
	
	return status;
}
