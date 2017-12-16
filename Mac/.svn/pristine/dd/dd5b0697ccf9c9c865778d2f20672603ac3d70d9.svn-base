#include "NowPlaying.h"
#include "Defines.h"
#include "License.h"
#include "Encrypt.h"
#include "ScriptSupport.h"
#include "Global.h"
#include "b64.h"
#include "sha2.h"
#include "hmac_sha256.h"
#include "xmalloc.h"
#include "oauth.h"
#include <stdio.h>
#include <CommonCrypto/CommonDigest.h>
#import "Logger.h"

#define MY_ENCODING_DEFAULT kCFStringEncodingUTF8
#define URL_DOWNLOAD_UPDATE "http://brandon.fuller.name/downloads/nowplaying/NowPlaying.dmg"
#define UPDATE_URL "http://brandon.fuller.name/downloads/nowplaying/feed-update-mac.xml"
#define AMAZON_LOCALE_US 5
#define DEFAULT_LOG_LEVEL LOGGING_ERROR

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
    char szTimestamp[256]; 
}
PlaylistItem;

bool g_fLicensed = false;
int g_intOnce = 0;
PlaylistItem* g_paPlaylist;
MPEventID g_hSignalPlayEvent;
MPEventID g_hSignalCleanupEvent;
time_t g_timetChanged = 0;
time_t g_timeLastUpdateCheck = 0;

ConfigurationData configurationData;

void DoUpdate(ITTrackInfo* x, bool fPlay);
void PrintTrackInfoNumberToFile(FILE * f, const char * lpszNodeName, int intNodeValue);
void PrintTrackInfoStringToFile(FILE* f, const char* lpszNodeName, ITUniStr255 NodeValue, bool fUseCData);
void PrintTrackInfoCFStringToFile(FILE* f, const char* lpszNodeName, CFStringRef strValue, bool fUseCData);
bool LoadMyPreferenceString(WindowRef wr, CFStringRef strPrefName, OSType theSig, SInt32 theNum, CFStringRef strPrefValueDefault, bool fEncrypted);
ControlRef GrabCRef(WindowRef theWindow, OSType theSig, SInt32 theNum);
void LoadSettingString(const char * lpszPrefName, char * lpszValue, size_t cbValue, const char * lpszDefault);
void DoGoogleAnalytics(const char * lpszAction);

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
	if ( configurationData.g_intXmlEncoding == 1 )
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

void EscapeAmpersands(CFMutableStringRef string)
{
	CFRange range;
	range.location = 0;
	range.length = CFStringGetLength(string);
	
	CFStringFindAndReplace(string, CFSTR("&"), CFSTR("&amp;"), range, 0);
}

void OutputCFStringToStdErr(const char * lpszPrefix, CFStringRef cfString)
{
    MyNSLog2(lpszPrefix, cfString);
}

void LOGDEBUG(const char * lpszPrefix, CFStringRef cfString)
{
	if (configurationData.g_intLogging == LOGGING_DEBUG)
	{
		OutputCFStringToStdErr(lpszPrefix, cfString);
	}
}

void AMLOGDEBUG(const char * format, ...)
{
	if (configurationData.g_intLogging == LOGGING_ERROR || configurationData.g_intLogging == LOGGING_INFO || configurationData.g_intLogging == LOGGING_DEBUG)
	{
		va_list args;
		va_start(args, format);
        MyNSLogv(format, args);
		va_end(args);
	}
}

void AMLOGINFO(const char * format, ...)
{
	if (configurationData.g_intLogging == LOGGING_ERROR || configurationData.g_intLogging == LOGGING_INFO)
	{
		va_list args;
		va_start(args, format);
        MyNSLogv(format, args);
		va_end(args);
	}
}

void AMLOGERROR(const char * format, ...)
{
	if (configurationData.g_intLogging == LOGGING_ERROR)
	{
		va_list args;
		va_start(args, format);
        MyNSLogv(format, args);
		va_end(args);
	}
}

void MessageBox(NSAlertStyle alertType, CFStringRef message)
{
	NSAlert *alert = [NSAlert alertWithMessageText:(NSString *) message
                                         defaultButton:@"OK"
                                       alternateButton:nil
                                           otherButton:nil
                             informativeTextWithFormat:@""];
	[alert setAlertStyle:alertType];
	[alert runModal];
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
		char * lpszBuffer = (char *) malloc( size );
		if (lpszBuffer)
		{
			if (CFStringGetCString(strValue, lpszBuffer, size, kCFStringEncodingUTF8))
			{
				char * lpszBufferEncoded = (char *) malloc(size * 3);
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
					
					if (configurationData.g_intLogging == LOGGING_DEBUG)					
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

CFStringRef GetJsonTagContents(CFStringRef strData, CFStringRef strTagName)
{
	CFStringRef strResult = NULL;
	CFStringRef strTagBegin = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("\"%@\":\""), strTagName);
	CFStringRef strTagEnd = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("\""), strTagName);

	CFRange rangeBegin = CFStringFind(strData, strTagBegin, 0);
	if (rangeBegin.location != kCFNotFound)
	{
		CFRange rangeBeginEndFind;
        
		rangeBeginEndFind.location = rangeBegin.location + rangeBegin.length;
		rangeBeginEndFind.length = CFStringGetLength( strData ) - rangeBeginEndFind.location;
		
		CFRange rangeEnd;
            
		if (CFStringFindWithOptions( strData, strTagEnd, rangeBeginEndFind, 0, &rangeEnd))
		{
			if (rangeEnd.location != kCFNotFound)
			{
				CFRange rangeSub;
                    
				rangeSub.location = rangeBeginEndFind.location;
				rangeSub.length = rangeEnd.location - rangeSub.location;
                    
				strResult = CFStringCreateWithSubstring( kCFAllocatorDefault, strData, rangeSub );
					
				if (configurationData.g_intLogging == LOGGING_DEBUG)
				{
					OutputCFStringToStdErr("Got JSON Tag Contents = ", strResult);
				}
			}
        }
	}
	else
	{
		LOGDEBUG("GetJsonTagContents failed to find tag = ", strTagName);
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

CFStringRef GetUrlContents(CFStringRef strUrl, CFStringRef strMethod, CFStringRef strPostData)
{
	CFStringRef value = NULL; 
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	
	NSURL* url = [NSURL URLWithString:(NSString*) strUrl];
	NSMutableURLRequest* urlRequest = [[NSMutableURLRequest alloc] initWithURL:url];
	
	[urlRequest setHTTPMethod:(NSString*) strMethod];
    [urlRequest setHTTPShouldHandleCookies:NO];

	if (strPostData != NULL)
	{
		[urlRequest setHTTPBody:[(NSString *) strPostData dataUsingEncoding:NSUTF8StringEncoding]];
	}

	NSURLResponse* response;
	NSError* err;
	NSData* responseData = [NSURLConnection sendSynchronousRequest:urlRequest returningResponse:&response error:&err];
	
	if (responseData != NULL)
	{
		value = CFStringCreateFromExternalRepresentation(kCFAllocatorDefault, (CFDataRef) responseData, kCFStringEncodingUTF8);

		LOGDEBUG("GetUrlContents: ", value);	
	}
	
	[pool release];

	return value;
}

void DoSoftwareUpdateCheck()
{
	time_t timeNow = 0;
	CFStringRef strData;

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
					CFStringRef strVersionMy = (CFStringRef) CFBundleGetValueForInfoDictionaryKey( theBundle, CFSTR("CFBundleShortVersionString") );
					if ( strVersionMy )
					{
						CFRetain( strVersionMy );

						if ( CFStringCompare( strVersionRss, strVersionMy, 0 ) > 0 )
						{
							NSAlert *alert = [[NSAlert alloc] init];
							[alert addButtonWithTitle:@"Yes"];
							[alert addButtonWithTitle:@"No"];
							[alert setMessageText:@"A software update is available for the Now Playing plugin!\n\nWould you like to download it now?"];
							[alert setInformativeText:@""];
							[alert setAlertStyle:NSInformationalAlertStyle];

							if ([alert runModal] == NSAlertFirstButtonReturn)
							{
								LaunchURL( URL_DOWNLOAD_UPDATE );
							}
							
							[alert release];
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
	AMLOGDEBUG("Begin DoAppleLookup\n");
	
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

	CFStringRef strUrl = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("http://%s%s%@+%@+%@&at=%s"), APPLE_LOOKUP_HOSTNAME, APPLE_LOOKUP_PATH, strTitle, strAlbum, strArtist, strlen( configurationData.g_szAppleAssociate ) ? configurationData.g_szAppleAssociate : DEFAULT_APPLE_ASSOCIATE);
	if (strUrl)
	{
		LOGDEBUG("Apple Lookup URL: ", strUrl);
		
		CFStringRef strData = GetUrlContents(strUrl, CFSTR("GET"), NULL);
		if (strData != NULL)
		{
            CFStringRef strTrackViewUrl = GetRandomHTMLChunk(strData, CFSTR(MARKER_APPLE_SONG_START), CFSTR(MARKER_APPLE_SONG_STOP));
            if (strTrackViewUrl != NULL)
            {
                CFMutableStringRef strTrackViewUrl1 = CFStringCreateMutableCopy( kCFAllocatorDefault, 0, strTrackViewUrl );

                EscapeAmpersands(strTrackViewUrl1);

                *strAppleURL = strTrackViewUrl1;
                
                LOGDEBUG("Apple URL: ", *strAppleURL);
			
                DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_APPLE);
                
                CFRelease(strTrackViewUrl);
            }

			CFRelease(strData);
		}
		
		CFRelease(strUrl);
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
	
	AMLOGDEBUG("End DoAppleLookup\n");	
}

void DoAmazonLookup(ITUniStr255 grouping, ITUniStr255 artist, ITUniStr255 album, CFStringRef* strDetailPageURL, CFStringRef* strSmallImageUrl, CFStringRef* strMediumImageUrl, CFStringRef* strLargeImageUrl)
{
	CFMutableStringRef strData;
	char szValue[MAX_PATH] = {0};
	
	AMLOGDEBUG("Begin DoAmazonLookup\n");

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

	if (configurationData. g_intAmazonLocale == 1 )
	{
		strDomain = CFSTR("ca");
	}
	else if ( configurationData.g_intAmazonLocale == 2 )
	{
		strDomain = CFSTR("de");
	}
	else if ( configurationData.g_intAmazonLocale == 3 )
	{
		strDomain = CFSTR("fr");
	}
	else if ( configurationData.g_intAmazonLocale == 4 )
	{
		strDomain = CFSTR("co.jp");
	}
	else if ( configurationData.g_intAmazonLocale == 6 )
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
		
		if (configurationData.g_intAmazonUseASIN > 0 && CFStringGetLength(strGrouping) == 10)
		{
			strUrl = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("http://webservices.amazon.%@/onca/xml?AWSAccessKeyId=%s&AssociateTag=%s&ItemId=%@&Operation=ItemLookup&ResponseGroup=Medium&Service=AWSECommerceService&Timestamp=%s"), strDomain, MY_AMAZON_ACCESS_KEY_ID, strlen(configurationData.g_szAmazonAssociate) > 0 ? configurationData.g_szAmazonAssociate : MY_AMAZON_ASSOCIATE_ID, strGrouping, szTimestamp);
		}
		else
		{
			strUrl = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("http://webservices.amazon.%@/onca/xml?AWSAccessKeyId=%s&Artist=%@&AssociateTag=%s&Operation=ItemSearch&ResponseGroup=Medium&SearchIndex=Music&Service=AWSECommerceService&Timestamp=%s&Title=%@"), strDomain, MY_AMAZON_ACCESS_KEY_ID, strArtist, strlen(configurationData.g_szAmazonAssociate) > 0 ? configurationData.g_szAmazonAssociate : MY_AMAZON_ASSOCIATE_ID, szTimestamp, strAlbum);
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
									AMLOGERROR("Stream read error, domain = %ld, error = %d\n", error.domain, (int) error.error);
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
							
							CFHTTPMessageRef myResponse = (CFHTTPMessageRef) CFReadStreamCopyProperty( myReadStream, kCFStreamPropertyHTTPResponseHeader );
							if ( myResponse )
							{
								//CFStringRef myStatusLine = CFHTTPMessageCopyResponseStatusLine( myResponse );
								UInt32 myErrCode = CFHTTPMessageGetResponseStatusCode( myResponse );

								AMLOGDEBUG("HTTP Status Code: %u\n", (unsigned int) myErrCode);
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
	
	AMLOGDEBUG("End DoAmazonLookup\n");
}

void DoTrackInfo(FILE* f, int i, int intOrder)
{	
	if ( configurationData.g_intPlaylistLength > 1 )
	{
		fprintf( f, "\t<song order=\"%d\" timestamp=\"%s\">\n", intOrder, g_paPlaylist[i].szTimestamp );
	}
	else
	{
		fprintf( f, "\t<song timestamp=\"%s\">\n", g_paPlaylist[i].szTimestamp );
	}

    PrintTrackInfoStringToFile( f, "title", g_paPlaylist[i].track.name, configurationData.g_intUseXmlCData > 0 ? true : false );
	PrintTrackInfoStringToFile( f, "artist", g_paPlaylist[i].track.artist, configurationData.g_intUseXmlCData > 0 ? true : false );
	PrintTrackInfoStringToFile( f, "album", g_paPlaylist[i].track.album, configurationData.g_intUseXmlCData > 0 ? true : false );
	PrintTrackInfoStringToFile( f, "genre", g_paPlaylist[i].track.genre, configurationData.g_intUseXmlCData > 0 ? true : false );
	PrintTrackInfoStringToFile( f, "kind", g_paPlaylist[i].track.kind, false );
	PrintTrackInfoNumberToFile( f, "track", g_paPlaylist[i].track.trackNumber );
	PrintTrackInfoNumberToFile( f, "numTracks", g_paPlaylist[i].track.numTracks );
	PrintTrackInfoNumberToFile( f, "year", g_paPlaylist[i].track.year );
	PrintTrackInfoStringToFile( f, "comments", g_paPlaylist[i].track.comments, configurationData.g_intUseXmlCData > 0 ? true : false  );
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
	PrintTrackInfoStringToFile( f, "composer", g_paPlaylist[i].track.composer, configurationData.g_intUseXmlCData > 0 ? true : false  );
	PrintTrackInfoStringToFile( f, "grouping", g_paPlaylist[i].track.grouping, configurationData.g_intUseXmlCData > 0 ? true : false  );
	PrintTrackInfoStringToFile( f, "file", g_paPlaylist[i].track.fileName, configurationData.g_intUseXmlCData > 0 ? true : false  );
	PrintTrackInfoCFStringToFile( f, "artworkID", g_paPlaylist[i].strArtworkId, false );

	fprintf( f, "\t</song>\n" );
}

void WriteXML(bool fPlay, int intNext)
{
	FILE* f = NULL;
	int i = 0;
	int intOrder = 1;

	AMLOGDEBUG("Begin WriteXML\n");

	f = fopen( configurationData.g_szOutputFile, "w" );
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

			if ( configurationData.g_intPlaylistLength > 1 )
			{
				for ( i = intNext; i >= 0; i-- )
				{
					if ( g_paPlaylist && g_paPlaylist[ i ].fSet )
					{
						DoTrackInfo( f, i, intOrder );
						intOrder++;
					}
				}
				
				for ( i = configurationData.g_intPlaylistLength - 1; i > intNext; i-- )
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
	
	AMLOGDEBUG("End WriteXML\n");
}

void MyDeleteFile(const char * lpszFile)
{
	FSRef fileRef;

	if (FSPathMakeRef((const unsigned char *) lpszFile, &fileRef, false ) == noErr)
	{
		if (FSDeleteObject(&fileRef ) == noErr)
		{
			AMLOGDEBUG("Deleted file = %s\n", lpszFile);
		}
	}
}

OSStatus script_exportArtwork(LoadedScriptInfoPtr scriptInfo, AEDesc* theResult)
{
	OSStatus err = noErr;
    AEDescList theParameters;
	AEDesc theMessage;
	AEDesc theDir;
	
	AMLOGDEBUG("Begin exportArtwork (%d)\n", configurationData.g_intArtworkWidth);

	err = LongToAEDesc(configurationData.g_intArtworkWidth, &theMessage);
	if (err == noErr)
	{
		char szPath[MAX_PATH] = {0};

		strcpy(szPath, configurationData.g_szOutputFile);
		MyPathRemoveFileSpec(szPath);
		
		CFStringRef cfOutputDir = CFStringCreateWithCString(kCFAllocatorDefault, szPath, 0);
		if (cfOutputDir)
		{
			err = CFStringToAEDesc(cfOutputDir, &theDir);
			if (err == noErr)
			{
				err = AEBuildDesc(&theParameters, NULL, "[@,@]", &theMessage, &theDir);
				if (noErr == err)
				{
					AEDesc errorMessages;

					err = CallScriptSubroutine(scriptInfo, "export_artwork", &theParameters, theResult, &errorMessages);
					if (err != noErr)
					{
						AMLOGERROR("CallScriptSubroutine failed with %d\n", err);
						
						CFStringRef strError;
						AEDescToCFString(&errorMessages, &strError);
						LOGDEBUG("Error: ", strError);
						CFRelease(strError);
					}					

					AEDisposeDesc(&theParameters);
				}
				else
				{
					AMLOGERROR("AEBuildDesc failed with %d\n", err);				
				}
				
				AEDisposeDesc(&theDir);
			}
			else
			{
				AMLOGERROR("CFStringToAEDesc failed with %d\n", err);				
			}
			
			CFRelease(cfOutputDir);
		}

		AEDisposeDesc(&theMessage);
	}
	else
	{
		AMLOGDEBUG("LongToAEDesc failed with %d\n", err);
	}

	AMLOGDEBUG("End exportArtwork\n");

	return err;
}

void PrintTrackInfoStringToFile(FILE* f, const char* lpszNodeName, ITUniStr255 NodeValue, bool fUseCData)
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
					AMLOGDEBUG("<%s><![CDATA[%s]]></%s>\n", lpszNodeName, &szBuffer[0], lpszNodeName);
				}
				else
				{
					fprintf( f, "\t\t<%s>%s</%s>\n", lpszNodeName, &szBuffer[0], lpszNodeName );
					AMLOGDEBUG("<%s>%s</%s>\n", lpszNodeName, &szBuffer[0], lpszNodeName);
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

void PrintTrackInfoCFStringToFile(FILE* f, const char* lpszNodeName, CFStringRef strValue, bool fUseCData)
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

void DoPing(bool fPlay, int intNext)
{
	AMLOGDEBUG("Begin DoPing\n");

	if ( strlen( &configurationData.g_szTrackBackUrl[0] ) > 0 )
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
							   
				if ( configurationData.g_intAmazonLookup )
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

				if ( configurationData.g_intAppleLookup )
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

				if ( configurationData.g_intAmazonLookup )
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

			CFStringRef strUrl = CFStringCreateWithCString( kCFAllocatorDefault, &configurationData.g_szTrackBackUrl[0], 0 );
			if ( strUrl )
			{
				CFURLRef urlRef = CFURLCreateWithString( kCFAllocatorDefault, strUrl, NULL );
				if ( urlRef )
				{
					CFIndex lenData = CFStringGetLength( strData );					
					UInt8* buffer = (UInt8*) CFAllocatorAllocate( kCFAllocatorDefault, fPlay ? lenData : 1, 0 );
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

								if ( strlen( &configurationData.g_szPingExtraInfo[0] ) > 0 )
								{
									CFStringRef strPingExtraInfo = CFStringCreateWithCString( kCFAllocatorDefault, &configurationData.g_szPingExtraInfo[0], 0 );
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
												AMLOGERROR("Stream read error, domain = %ld, error = %d\n", error.domain, (int) error.error);
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
									
										CFHTTPMessageRef myResponse = (CFHTTPMessageRef) CFReadStreamCopyProperty( myReadStream, kCFStreamPropertyHTTPResponseHeader );
										if ( myResponse )
										{
											//CFStringRef myStatusLine = CFHTTPMessageCopyResponseStatusLine( myResponse );
											UInt32 myErrCode = CFHTTPMessageGetResponseStatusCode( myResponse );
											
											AMLOGDEBUG("HTTP Status Code: %u\n", (unsigned int) myErrCode);
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

	AMLOGDEBUG("End DoPing\n");
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
	strcpy(configurationData.g_szTwitterAuthKey, "");
	strcpy(configurationData.g_szTwitterAuthSecret, "");
	strcpy(configurationData.g_szTwitterUsername, "");
	configurationData.g_timetTwitterLatest = 0;
	
	CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_TWITTER_KEY), CFSTR(""), CFSTR(kBundleName));											
	CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_TWITTER_SECRET), CFSTR(""), CFSTR(kBundleName));
	CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_TWITTER_SCREENNAME), CFSTR(""), CFSTR(kBundleName));
	
	CFNumberRef numPrefValue = CFNumberCreate(NULL, kCFNumberLongType, &configurationData.g_timetTwitterLatest);
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
						strcpy(&configurationData.g_szTwitterAuthKey[0], t_key);
						strcpy(&configurationData.g_szTwitterAuthSecret[0], t_secret);

						MessageBox(NSInformationalAlertStyle, CFSTR(TWITTER_AUTHORIZE_MESSAGE));
					
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
						MessageBox(NSCriticalAlertStyle, CFSTR(TWITTER_MESSAGE_AUTHORIZATION_FAILED));
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
				char * req_url = oauth_sign_url2(lpszUrl, &postarg, OA_HMAC, NULL, TWITTER_CONSUMER_KEY, TWITTER_SECRET_KEY, configurationData.g_szTwitterAuthKey, configurationData.g_szTwitterAuthSecret);
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
											
											strcpy(configurationData.g_szTwitterAuthKey, t_key);
										}
										
										CFStringRef strSecret = CFStringCreateWithCString(kCFAllocatorDefault, t_secret, kCFStringEncodingUTF8);
										if (strSecret)
										{										
											CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_TWITTER_SECRET), strSecret, CFSTR(kBundleName));
											CFRelease(strSecret);
											
											strcpy(configurationData.g_szTwitterAuthSecret, t_secret);
										}

										char * lpszScreenName = NULL;
										if (GetTokenFromReply(lpszResponse, TWITTER_TOKEN_SCREENNAME, &lpszScreenName))
										{
											CFStringRef strScreenName = CFStringCreateWithCString(kCFAllocatorDefault, lpszScreenName, kCFStringEncodingUTF8);
											if (strScreenName)
											{
												CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_TWITTER_SCREENNAME), strScreenName, CFSTR(kBundleName));											
												CFRelease(strScreenName);
												
												strcpy(configurationData.g_szTwitterUsername, lpszScreenName);
											}
											
											free(lpszScreenName);
										}

										CFPreferencesAppSynchronize(CFSTR(kBundleName));
										
										MessageBox(NSInformationalAlertStyle, CFSTR(TWITTER_MESSAGE_SUCCESS));

										free(t_key);
										free(t_secret);
									}
									else
									{
										MessageBox(NSCriticalAlertStyle, CFSTR(TWITTER_MESSAGE_ACCESS_FAILED));
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
	configurationData.g_timetTwitterLatest = time(NULL);
	
	// Save it in case we exit the media player
	CFNumberRef numPrefValue = CFNumberCreate(NULL, kCFNumberLongType, &configurationData.g_timetTwitterLatest);
	if (numPrefValue != NULL)
	{
		CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_TWITTER_LATEST), numPrefValue, CFSTR(kBundleName));
		CFPreferencesAppSynchronize(CFSTR(kBundleName));
		CFRelease(numPrefValue);
	}
}

void DoTwitter(int intNext)
{
	AMLOGDEBUG("Begin DoTwitter\n");

	CFMutableStringRef strMessage = CFStringCreateMutable(kCFAllocatorDefault, 0);
	if (strMessage)
	{
		CFStringAppendCString(strMessage, strlen(configurationData.g_szTwitterMessage) ? configurationData.g_szTwitterMessage : TWITTER_MESSAGE_DEFAULT, kCFStringEncodingUTF8);
		
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
				char * req_url = oauth_sign_url2(lpszUrl, &postarg, OA_HMAC, NULL, TWITTER_CONSUMER_KEY, TWITTER_SECRET_KEY, configurationData.g_szTwitterAuthKey, configurationData.g_szTwitterAuthSecret);
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
								CFStringRef strTweetId = GetJsonTagContents(strResponse, CFSTR("id_str"));
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
	
	AMLOGDEBUG("End DoTwitter\n");
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
			CFStringAppendCString(strPostData, configurationData.g_szFacebookSessionKey, kCFStringEncodingUTF8);
			
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
				CFStringAppendCString(strPostData, configurationData.g_szFacebookSessionKey, kCFStringEncodingUTF8);
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
						
						CFStringGetCString(strName, configurationData.g_szFacebookUsername, sizeof(configurationData.g_szFacebookUsername), kCFStringEncodingUTF8);
						
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
	strcpy(configurationData.g_szFacebookSessionKey, "");
	strcpy(configurationData.g_szFacebookUsername, "");
	configurationData.g_timetFacebookLatest = 0;
	
	CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_FACEBOOK_SESSION_KEY), CFSTR(""), CFSTR(kBundleName));											
	CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_FACEBOOK_UID), CFSTR(""), CFSTR(kBundleName));
	CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_FACEBOOK_SCREENNAME), CFSTR(""), CFSTR(kBundleName));
	
	CFNumberRef numPrefValue = CFNumberCreate(NULL, kCFNumberLongType, &configurationData.g_timetFacebookLatest);
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
	configurationData.g_timetFacebookLatest = time(NULL);
	
	// Save it in case we exit the media player
	CFNumberRef numPrefValue = CFNumberCreate(NULL, kCFNumberLongType, &configurationData.g_timetFacebookLatest);
	if (numPrefValue != NULL)
	{
		CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_FACEBOOK_LATEST), numPrefValue, CFSTR(kBundleName));
		CFPreferencesAppSynchronize(CFSTR(kBundleName));
		CFRelease(numPrefValue);
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
	AMLOGDEBUG("Begin DoFacebookStream\n");
	
	if (configurationData.g_intAmazonLookup == 0 || (g_paPlaylist[intNext].strDetailPageURL && CFStringGetLength(g_paPlaylist[intNext].strDetailPageURL) > 0 && g_paPlaylist[intNext].strSmallImageUrl && CFStringGetLength(g_paPlaylist[intNext].strSmallImageUrl) > 0))
	{
		CFStringRef strCallId = CFStringCreateWithFormat(NULL, NULL, CFSTR("%lu"), time(NULL));
		if (strCallId)
		{
			CFMutableStringRef strPostData = CFStringCreateMutable(kCFAllocatorDefault, 0);
			if (strPostData)
			{
                //
                // ACCESS TOKEN
                //
                
                CFStringAppend(strPostData, CFSTR("&access_token="));
                CFStringAppendCString(strPostData, configurationData.g_szFacebookSessionKey, kCFStringEncodingUTF8);

                //
                // MESSAGE - The actual text of the post
                //

                {
                    CFStringAppend(strPostData, CFSTR("&"));
                    CFStringAppend(strPostData, CFSTR(FACEBOOK_PARAMETER_MESSAGE));
                    CFStringAppend(strPostData, CFSTR("="));
                    
                    CFMutableStringRef strMessage = CFStringCreateMutable(kCFAllocatorDefault, 0);
                    if (strMessage)
                    {
                        CFStringAppendCString(strMessage, strlen(configurationData.g_szFacebookMessage) ? configurationData.g_szFacebookMessage : FACEBOOK_MESSAGE_DEFAULT, kCFStringEncodingUTF8);
                    
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
                                    
                                    CFStringAppendCString(strPostData, lpszMessage1, kCFStringEncodingUTF8);
                                    
                                    free(lpszMessage1);
                                }
                            }
                            
                            free(lpszMessage);
                        }
                        
                        CFRelease(strMessage);
                    }
                }

                //
                // PICTURE - Thumbnail shown from Amazon
                //

                if (g_paPlaylist[intNext].strSmallImageUrl && CFStringGetLength(g_paPlaylist[intNext].strSmallImageUrl) > 0)
                {
                    CFStringAppend(strPostData, CFSTR("&"));
                    CFStringAppend(strPostData, CFSTR(FACEBOOK_PARAMETER_PICTURE));
                    CFStringAppend(strPostData, CFSTR("="));
                    
                    CFStringRef strUrlDecoded = UrlDecode(g_paPlaylist[intNext].strSmallImageUrl);
                    if (strUrlDecoded)
                    {
                        CFStringAppend(strPostData, strUrlDecoded);
                        CFRelease(strUrlDecoded);
                    }
                }

                //
                // NAME - Track name
                //

                if (g_paPlaylist[intNext].track.name)
                {
                    CFStringAppend(strPostData, CFSTR("&"));
                    CFStringAppend(strPostData, CFSTR(FACEBOOK_PARAMETER_NAME));
                    CFStringAppend(strPostData, CFSTR("="));
                    CFStringRef strValue = ITUniStr255ToCFString(g_paPlaylist[intNext].track.name);
                    if (strValue)
                    {
                        CFStringAppend(strPostData, strValue);
                        CFRelease(strValue);
                    }
                }

                //
                // LINK - Album URL from iTunes (Amazon link doesn't show you additional info)
                //

                if (false && g_paPlaylist[intNext].strAppleURL && CFStringGetLength(g_paPlaylist[intNext].strAppleURL) > 0)
                {
                    CFStringAppend(strPostData, CFSTR("&"));
                    CFStringAppend(strPostData, CFSTR(FACEBOOK_PARAMETER_LINK));
                    CFStringAppend(strPostData, CFSTR("="));
                    
                    char szAppleURL[1024] = {0};
                    char szAppleURLEncoded[1024] = {0};
                    
                    if (CFStringGetCString( g_paPlaylist[intNext].strAppleURL, &szAppleURL[0], sizeof( szAppleURL ), 0))
                    {
                        UrlEncode(&szAppleURLEncoded[0], sizeof( szAppleURLEncoded ), &szAppleURL[0]);
                        CFStringAppendCString(strPostData, szAppleURLEncoded, kCFStringEncodingUTF8);
                    }
                }

                //
                // CAPTION - Artist shown under track name
                //

                if (g_paPlaylist[intNext].track.artist)
                {
                    CFStringAppend(strPostData, CFSTR("&"));
                    CFStringAppend(strPostData, CFSTR(FACEBOOK_PARAMETER_CAPTION));
                    CFStringAppend(strPostData, CFSTR("="));
                    CFStringRef strValue = ITUniStr255ToCFString(g_paPlaylist[intNext].track.artist);
                    if (strValue)
                    {
                        CFStringAppend(strPostData, strValue);
                        CFRelease(strValue);
                    }
                }
            
                //
                // PROPERTIES
                //

                if (false)
                {
                    CFMutableStringRef strProperties = CFStringCreateMutable(kCFAllocatorDefault, 0);
                    if (strProperties)
                    {
                        bool fHasProp = false;
                        
                        CFStringAppend(strProperties, CFSTR("["));
                        
                        CFStringRef strAlbum = ITUniStr255ToCFString(g_paPlaylist[intNext].track.album);
                        if (strAlbum)
                        {
                            if (CFStringGetLength(strAlbum) > 0)
                            {
                                if (fHasProp) CFStringAppend(strProperties, CFSTR(","));
                                CFStringAppend(strProperties, CFSTR("\"Album: "));
                                CFStringAppend(strProperties, strAlbum);
                                CFStringAppend(strProperties, CFSTR("\""));
                            
                                fHasProp = true;
                            }
                            
                            CFRelease(strAlbum);
                        }
                        
                        if (g_paPlaylist[intNext].track.year > 0)
                        {
                            CFStringRef strYear = CFStringCreateWithFormat(kCFAllocatorDefault, 0, CFSTR("%d"), g_paPlaylist[intNext].track.year);
                            if (strYear)
                            {
                                if (fHasProp) CFStringAppend(strProperties, CFSTR(","));
                                CFStringAppend(strProperties, CFSTR( "\"Year: "));
                                CFStringAppend(strProperties, strYear);
                                CFStringAppend(strProperties, CFSTR("\""));
                                
                                fHasProp = true;
                                
                                CFRelease(strYear);
                            }
                        }
                        
                        CFStringAppend(strProperties, CFSTR("]"));
                        
                        CFStringAppend(strPostData, CFSTR("&"));
                        CFStringAppend(strPostData, CFSTR("properties"));
                        CFStringAppend(strPostData, CFSTR("="));
                        CFStringAppend(strPostData, strProperties);
                    }
                }
                
                //
                // ACTIONS - Additional sub-links
                //

                if (g_paPlaylist[intNext].track.artist)
                {
                    CFStringRef strValue = ITUniStr255ToCFString(g_paPlaylist[intNext].track.artist);
                    if (strValue)
                    {
                        CFMutableStringRef strActionLinks = CFStringCreateMutable(kCFAllocatorDefault, 0);
                        if (strActionLinks)
                        {
                            CFStringAppendFormat(strActionLinks, 0, CFSTR("[{\"name\":\"Artist\",\"link\":\"http://en.wikipedia.org/w/index.php?search=%@\"}]"), strValue);

                            char * lpszValue = CFStringToCString(strActionLinks, kCFStringEncodingUTF8);
                            if (lpszValue)
                            {
                                char szActionLinks1[5 * 1024] = {0};
                                UrlEncodeAndConcat(szActionLinks1, sizeof(szActionLinks1), lpszValue);
                                free(lpszValue);
                                
                                CFStringAppend(strPostData, CFSTR("&"));
                                CFStringAppend(strPostData, CFSTR("actions"));
                                CFStringAppend(strPostData, CFSTR("="));
                                CFStringAppendCString(strPostData, szActionLinks1, kCFStringEncodingUTF8);
                            }

                            CFRelease(strActionLinks);
                        }
                        
                        CFRelease(strValue);
                    }
                }
                
                LOGDEBUG("POST: ", strPostData);
									 
				//
				// Do the Ping
				//
                
                char szUid[MAX_PATH] = {0};
                
                LoadSettingString(MY_REGISTRY_VALUE_FACEBOOK_UID, szUid, sizeof(szUid), "");
				 
				CFStringRef strUrl = CFStringCreateWithFormat(kCFAllocatorDefault, 0, CFSTR("%s://%s/%s/%s"), FACEBOOK_PROTOCOL, FACEBOOK_HOSTNAME_GRAPH, szUid, FACEBOOK_PATH_FEED);
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
			AMLOGINFO("Skipping publish because Amazon lookup enabled but song not found\n");
		}
	}
		 
	AMLOGDEBUG("End DoFacebookStream\n");
}
																			 
void DoFacebookAuthorize()
{
	bool fResult = false;
	
	if (strlen(configurationData.g_szFacebookAuthToken) > 0)
	{
		CFStringRef strUrl = CFStringCreateWithFormat(kCFAllocatorDefault, 0, CFSTR("http://%s%s%s"), FACEBOOK_AUTHORIZE_HOSTNAME, FACEBOOK_AUTHORIZE_PATH, configurationData.g_szFacebookAuthToken);
		if (strUrl)
		{
			CFStringRef strData = GetUrlContents(strUrl, CFSTR(HTTP_METHOD_GET), NULL);
			if (strData)
			{
				LOGDEBUG("Facebook Authorize Result: ", strData);

				CFStringRef strSessionKey = GetXMLTagContents(strData, CFSTR(FACEBOOK_RESPONSE_SESSION_KEY));
				if (strSessionKey)
				{
					if (CFStringGetCString(strSessionKey, configurationData.g_szFacebookSessionKey, sizeof(configurationData.g_szFacebookSessionKey), MY_ENCODING_DEFAULT))
					{
						CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_FACEBOOK_SESSION_KEY), strSessionKey, CFSTR(kBundleName));
						CFPreferencesAppSynchronize(CFSTR(kBundleName));
							
						DoFacebookUserId();

						DoFacebookInfo();

						MessageBox(NSInformationalAlertStyle, CFSTR(FACEBOOK_MESSAGE_VERIFY_SUCCESS));

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
			MessageBox(NSCriticalAlertStyle, CFSTR("Facebook authorization was not successful."));
		}
	}
	else
	{
		MessageBox(NSCriticalAlertStyle, CFSTR("Step 1 must be completed first."));
	}
}

void DoFacebookAdd()
{
	MessageBox(NSInformationalAlertStyle, CFSTR(FACEBOOK_MESSAGE_ADD));
	
	CFUUIDRef uuid = CFUUIDCreate(kCFAllocatorDefault);
	if (uuid)
	{
		CFStringRef strUrl = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%s%@&version=%s"), FACEBOOK_URL_ADD, uuid, configurationData.g_szVersion);
		if (strUrl)
		{
			char szUrl[MAX_PATH] = {0};
			if (CFStringGetCString(strUrl, szUrl, sizeof(szUrl), kCFStringEncodingUTF8))
			{
				LaunchURL(&szUrl[0]);
				
				CFStringRef strUuid = CFUUIDCreateString(kCFAllocatorDefault, uuid);
				if (strUuid)
				{
					CFStringGetCString(strUuid, configurationData.g_szFacebookAuthToken, sizeof(configurationData.g_szFacebookAuthToken), kCFStringEncodingUTF8);
					
					CFRelease(strUuid);
				}
			}

			CFRelease(strUrl);
		}
		
		CFRelease(uuid);
	}
}

void DoGoogleAnalytics(const char * lpszAction)
{
	char szVisitor[MAX_PATH] = {0};
	
 	sprintf(szVisitor, "%s%s", configurationData.g_szGuid, GOOGLE_ANALYTICS_UTMAC);
	
	unsigned char szDigest[16];

	CC_MD5_CTX c;
	CC_MD5_Init(&c);
	CC_MD5_Update(&c, (unsigned char *) szVisitor, strlen(szVisitor));
	CC_MD5_Final(szDigest, &c);
	
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
												  configurationData.g_szVersion);
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
						unsigned char szDigest[16];
						
						CC_MD5_CTX c;						
						CC_MD5_Init(&c);
						CC_MD5_Update(&c, (unsigned char *) lpszBuf, strlen(lpszBuf));
						CC_MD5_Final(szDigest, &c);
						
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

	AMLOGDEBUG("Begin DoUpdate\n");

	if ( !g_fLicensed && fPlay )
	{
		if ( g_intOnce > TRIAL_LIMIT )
		{		
			AMLOGDEBUG("Exiting DoUpdate -- Over Trial Limit\n");

			return;
		}
		else if ( g_intOnce == TRIAL_LIMIT )
		{
			MessageBox(NSCriticalAlertStyle, CFSTR("TRIAL VERSION\n\nPurchase the licensed version today! This trial version will only work for the five songs you play each session."));
		}
	}

	//
	// Skip any short songs if configured
	//

	if ( configurationData.g_intSkipShort > 0 && pTrack->totalTimeInMS > 0 && (unsigned int) configurationData.g_intSkipShort * 1000 > pTrack->totalTimeInMS )
	{
		AMLOGDEBUG("Duration %u ms; skipping", (unsigned int) pTrack->totalTimeInMS);

		return;
	}
	
	//
	// Skip any songs that match kinds
	//

	if (strlen(configurationData.g_szSkipKinds) > 0)
	{
		bool skip = false;
		
		CFStringRef strKind = ITUniStr255ToCFString(pTrack->kind);
		if (strKind)
		{
			char * lpszKind = CFStringToCString(strKind, 0);
			if (lpszKind)
			{			
				char * lpszTokens = strdup(configurationData.g_szSkipKinds);
				if (lpszTokens)
				{
					char * lpszToken = strtok(lpszTokens, ",");
					while (lpszToken != NULL && !skip)
					{					
						if (strcmp(lpszToken, lpszKind) == 0)
						{
							AMLOGDEBUG("Kind matched \"%s\"; skipping\n", lpszKind);

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
	
	if ( configurationData.g_intPlaylistLength > 1 )
	{
		//
		// If the delay has not passed, then update the previous entry again
		//
		
		if ( time( NULL ) >= g_timetChanged + configurationData.g_intPlaylistBufferDelay )
		{
			configurationData.g_intNext++;
			
			if ( configurationData.g_intNext == configurationData.g_intPlaylistLength )
			{
				configurationData.g_intNext = 0;
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
	
	memcpy( &g_paPlaylist[ configurationData.g_intNext ].track, pTrack, sizeof( ITTrackInfo ) );

	g_paPlaylist[ configurationData.g_intNext ].fSet = true;
	g_paPlaylist[ configurationData.g_intNext ].fPlay = true;
	
	if ( g_paPlaylist[ configurationData.g_intNext ].strDetailPageURL )
	{
		CFRelease( g_paPlaylist[ configurationData.g_intNext ].strDetailPageURL );
		g_paPlaylist[ configurationData.g_intNext ].strDetailPageURL = NULL;
	}

	if ( g_paPlaylist[ configurationData.g_intNext ].strSmallImageUrl )
	{
		CFRelease( g_paPlaylist[ configurationData.g_intNext ].strSmallImageUrl );
		g_paPlaylist[ configurationData.g_intNext ].strSmallImageUrl = NULL;
	}

	if ( g_paPlaylist[ configurationData.g_intNext ].strMediumImageUrl )
	{
		CFRelease( g_paPlaylist[ configurationData.g_intNext ].strMediumImageUrl );
		g_paPlaylist[ configurationData.g_intNext ].strMediumImageUrl = NULL;
	}

	if ( g_paPlaylist[ configurationData.g_intNext ].strLargeImageUrl )
	{
		CFRelease( g_paPlaylist[ configurationData.g_intNext ].strLargeImageUrl );
		g_paPlaylist[ configurationData.g_intNext ].strLargeImageUrl = NULL;
	}

	if ( g_paPlaylist[ configurationData.g_intNext ].strAppleURL )
	{
		CFRelease( g_paPlaylist[ configurationData.g_intNext ].strAppleURL );
		g_paPlaylist[ configurationData.g_intNext ].strAppleURL = NULL;
	}

	if ( g_paPlaylist[ configurationData.g_intNext ].strArtworkId )
	{
		CFRelease( g_paPlaylist[ configurationData.g_intNext ].strArtworkId );
		g_paPlaylist[ configurationData.g_intNext ].strArtworkId = NULL;
	}

	//
	// Alert worker thread that it has stuff to process
	//

	err = MPSetEvent( g_hSignalPlayEvent, kNewSong );
	if ( err != noErr )
	{
		AMLOGERROR("MPSetEvent failed with %d!\n", (int) err);
	}

	//
	// Count for the trial
	//			

	if ( fPlay )
	{
		g_intOnce++;
	}

	AMLOGDEBUG("End DoUpdate\n");
}

static void MemClear(LogicalAddress dest,SInt32 length)
{
	register unsigned char * ptr;

	ptr = (unsigned char *) dest;
	
	while (length-- > 0)
		*ptr++ = 0;
}

void SaveMyStringPreference(CFStringRef strPrefName, NSString* string, bool fEncrypted)
{
	if (fEncrypted && [string length] > 0)
	{
		char szTemp[MAX_PATH] = {0};
		char szValue[MAX_PATH] = {0};

		CFStringGetCString( (CFStringRef) string, &szValue[0], sizeof(szValue), MY_ENCODING_DEFAULT);
	
		EncryptString(szValue, szTemp, sizeof(szTemp));
		
		CFStringRef	strPrefValue = CFStringCreateWithCString(NULL, szTemp, MY_ENCODING_DEFAULT);
		
		CFPreferencesSetAppValue(strPrefName, strPrefValue, CFSTR(kBundleName));
		
		CFRelease(strPrefValue);
	}
	else
	{
		CFPreferencesSetAppValue(strPrefName, (CFStringRef) string, CFSTR(kBundleName));		
	}
}

void SaveMyNumberPreference(CFStringRef strPrefName, int number)
{
	CFNumberRef numPrefValue = CFNumberCreate(NULL, kCFNumberSInt16Type, &number);
	if (numPrefValue != NULL)
	{
		CFPreferencesSetAppValue(strPrefName, numPrefValue, CFSTR(kBundleName));
	
		CFRelease(numPrefValue);
	}
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

	AMLOGINFO("Reading Preference: Key = %s, Value = %d\n", lpszPrefName, nValue);
		
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

	AMLOGINFO("Reading Preference: Key = %s, Value = %lu\n", lpszPrefName, nValue);
		
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
	StringCbCopy(lpszValue, cbValue, lpszDefault);
	
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
				AMLOGDEBUG("IsExported %s at %lu vs. %lu -- true\n", lpszPrefName, timetValue, timetMarker);

				return true;
			}
		}
	}

	AMLOGDEBUG("IsExported %s -- false\n", lpszPrefName);
	
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
			
			AMLOGDEBUG("MarkExported %s at %lu\n", lpszPrefName, timetValue);
		}
	}
}

OSStatus FTP_Upload(const char * lpszFTPHost, const char * lpszFileRemote, const char * lpszFTPUser, const char * lpszFTPPassword, const char * lpszPathLocal)
{
	AMLOGDEBUG("Begin FTP_Upload for %s to %s\n", lpszPathLocal, lpszFileRemote);
	
	OSStatus err = EXIT_SUCCESS;
	char * filename = tmpnam(NULL);
	const char * flags = "";
	char user[MAX_PATH] = {0};
	char password[MAX_PATH] = {0};

	EscapeForExpect(user, sizeof(user), lpszFTPUser);
	EscapeForExpect(password, sizeof(password), lpszFTPPassword);

	AMLOGDEBUG("FTP script is %s\n", filename);
	
	if (configurationData.g_intLogging == LOGGING_DEBUG)
	{
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

	CFStringRef strCommand = CFStringCreateWithFormat(NULL, NULL, CFSTR("/usr/bin/expect -f %s%s"), filename, (configurationData.g_intLogging == LOGGING_DEBUG || configurationData.g_intLogging == LOGGING_INFO) ? "" : " > /dev/null");
	if (strCommand)
	{		
		char * cmd = CFStringToCString(strCommand, kCFStringEncodingMacRoman);
		if (cmd)
		{			
			int status = system(cmd);	
			if (status != 0)
			{
				AMLOGERROR("Return value = %d\n", status);
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
	const char * flags = "";
	char password[MAX_PATH] = {0};
	
	EscapeForExpect(password, sizeof(password), lpszFTPPassword);

	AMLOGDEBUG("SFTP script is %s\n", filename);
	
	if (configurationData.g_intLogging == LOGGING_DEBUG)
	{	
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

	CFStringRef strCommand = CFStringCreateWithFormat(NULL, NULL, CFSTR("/usr/bin/expect -f %s%s"), filename, (configurationData.g_intLogging == LOGGING_DEBUG || configurationData.g_intLogging == LOGGING_INFO) ? "" : " > /dev/null");
	if (strCommand)
	{		
		char * cmd = CFStringToCString(strCommand, kCFStringEncodingMacRoman);
		if (cmd)
		{			
			int status = system(cmd);	
			if (status != 0)
			{
				AMLOGERROR("Return value = %d\n", status);
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

	AMLOGDEBUG("Begin taskFunc\n");

	while (processingEvents)
	{
		AMLOGDEBUG("Do MPWaitForEvent\n");

		err = MPWaitForEvent( g_hSignalPlayEvent, &theFlags, kDurationForever );
		if ( err != noErr )
		{
			AMLOGERROR("MPWaitForEvent failed with %d!\n", (int) err);
		}
	
		if ( ( theFlags & kNewSong ) != 0 )
		{		
			DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_PLAY);

			intNext = configurationData.g_intNext;
			
			//
			// ArtworkID
			//

			char szArtworkID[256] = {0};
			GetArtworkID(g_paPlaylist[intNext].track.artist, g_paPlaylist[intNext].track.album, &szArtworkID[0] );
			g_paPlaylist[intNext].strArtworkId = CFStringCreateWithCString( kCFAllocatorDefault, &szArtworkID[0], 0 );
            
            //
            // Timestamp
            //
            
            time_t now_t = time( NULL );
            struct tm now;
            now = *gmtime( &now_t );
            sprintf( &g_paPlaylist[intNext].szTimestamp[0], TIMEZONE_FORMAT_GMT, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec );           

			//
			// Amazon
			//

			if ( g_paPlaylist[intNext].fPlay && configurationData.g_intAmazonLookup > 0 )
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

			if (g_paPlaylist[intNext].fPlay && configurationData.g_intAppleLookup > 0)
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
			
			if ( g_paPlaylist[intNext].fPlay && ( configurationData.g_intArtworkUpload > 0 || configurationData.g_intArtworkExport > 0 ) && !IsExported(szArtworkID, configurationData.g_timetExportedMarker) )
			{
				LoadedScriptInfoPtr scriptInfo = NULL;				
				err = LoadCompiledScript( CFSTR( kBundleName ), CFSTR("NowPlaying"), &scriptInfo );
				if ( err == noErr )
				{			
					CFStringRef strArtworkPathLocal;
					AEDescList theResult;
					
					err = script_exportArtwork( scriptInfo, &theResult );
					if (err == noErr)
					{					
						OSStatus err1 = AEDescToCFString( &theResult, &strArtworkPathLocal );
						if ( err1 == 0 )
						{
							if ( CFStringGetLength( strArtworkPathLocal ) > 0 )
							{
								char szArtworkPathLocal[128] = {0};
								CFStringGetCString( strArtworkPathLocal, &szArtworkPathLocal[0], sizeof( szArtworkPathLocal ), 0 );
								
								AMLOGDEBUG("ExportedArtwork = %s\n", szArtworkPathLocal);
								
								AEDisposeDesc( &theResult );

								if ( err == noErr )
								{
									if ( configurationData.g_intArtworkUpload > 0 )
									{
										char szArtworkFileRemote[256] = {0};
										strcpy( szArtworkFileRemote, configurationData.g_szFTPPath );
										
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
										
										if ( configurationData.g_intUploadProtocol == 2 )
										{
											if ( FTP_Upload( configurationData.g_szFTPHost, szArtworkFileRemote, configurationData.g_szFTPUser, configurationData.g_szFTPPassword, szArtworkPathLocal ) == EXIT_SUCCESS)
											{
												MarkExported(szArtworkID);
											}
											else
											{
												AMLOGERROR("FTP_Upload failed for artwork\n");
											}
										}
										else if ( configurationData.g_intUploadProtocol == 3 )
										{
											if ( SFTP_Upload( configurationData.g_szFTPHost, szArtworkFileRemote, configurationData.g_szFTPUser, configurationData.g_szFTPPassword, szArtworkPathLocal ) == EXIT_SUCCESS)
											{
												MarkExported(szArtworkID);
											}
											else
											{
												AMLOGERROR("SFTP_Upload failed for artwork\n");
											}
										}									
									}
									
									//
									// Delete the artwork when done uploading if we don't want it local
									//
									
									if ( configurationData.g_intArtworkExport == 0 )
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
							AMLOGERROR("Can't convert description to string %d\n", (int) err1);
						}
					}
					
					// Free the scripts
					UnloadCompiledScript( scriptInfo );
					scriptInfo = NULL;										
				}
				else
				{
					AMLOGERROR("Unable to load compiled script!\n");
				}
			}
			
			//
			// Upload the XML file
			//

			if ( configurationData.g_intUploadProtocol == 2 )
			{
				FTP_Upload( configurationData.g_szFTPHost, configurationData.g_szFTPPath, configurationData.g_szFTPUser, configurationData.g_szFTPPassword, configurationData.g_szOutputFile );
			}
			else if ( configurationData.g_intUploadProtocol == 3 )
			{				
				SFTP_Upload( configurationData.g_szFTPHost, configurationData.g_szFTPPath, configurationData.g_szFTPUser, configurationData.g_szFTPPassword, configurationData.g_szOutputFile );
			}

			//
			// Do ping
			//

			DoPing( true, intNext );
			
			//
			// Do Twitter
			//
			
			if ( configurationData.g_intTwitterEnabled && strlen( configurationData.g_szTwitterAuthKey ) > 0 && strlen( configurationData.g_szTwitterAuthSecret ) > 0 )
			{				
				time_t now = time(NULL);
				time_t then =  configurationData.g_timetTwitterLatest + configurationData.g_intTwitterRateLimitMinutes * 60;
				
				if (now > then)
				{
					DoTwitter( intNext );
				}
				else
				{					
					AMLOGINFO("Rate limited at %d minutes; %lu seconds until resume; Skipping Twitter\n", configurationData.g_intTwitterRateLimitMinutes, then - now);
				}
			}
			
			//
			// Do Facebook
			//

			if (configurationData.g_intFacebookEnabled && strlen( configurationData.g_szFacebookSessionKey ) > 0)
			{
				time_t now = time(NULL);
				time_t then =  configurationData.g_timetFacebookLatest + configurationData.g_intFacebookRateLimitMinutes * 60;
				
				if (now > then)
				{
					DoFacebookStream(intNext);
				}
				else
				{
					AMLOGINFO("Rate limited at %d minutes; %lu seconds until resume; Skipping Facebook\n", configurationData.g_intFacebookRateLimitMinutes, then - now);					
				}
			}			
		}

		//
		// If iTunes is closing
		//
		
		if ( ( theFlags & kSelfDestruct ) != 0 )
		{
			processingEvents = false;
			
			AMLOGDEBUG("taskFunc stop processing events\n");
		}
	}

	//
	// If we ever played at least one song, then tell everyone we stopped.
	//

	if ( g_intOnce > 0 &  configurationData.g_intPublishStop > 0 )
	{
		//
		// Write out XML file
		//

		WriteXML( false, 0 );

		//
		// Upload the XML file
		//

		if (configurationData. g_intUploadProtocol == 2 )
		{
			FTP_Upload( configurationData.g_szFTPHost, configurationData.g_szFTPPath, configurationData.g_szFTPUser, configurationData.g_szFTPPassword, configurationData.g_szOutputFile );
		}
		else if ( configurationData.g_intUploadProtocol == 3 )
		{
			AMLOGDEBUG("Begin SFTP_Upload\n");
			
			SFTP_Upload( configurationData.g_szFTPHost, configurationData.g_szFTPPath, configurationData.g_szFTPUser, configurationData.g_szFTPPassword, configurationData.g_szOutputFile );
			
			AMLOGDEBUG("End SFTP_Upload\n");
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
	
	AMLOGDEBUG("End taskFunc\n");

	return noErr;
}

void CacheMySettings(bool fFirst)
{
	char szTemp[MAX_PATH] = {0};
	
	if ( fFirst )
	{
		LoadSettingNumber( MY_REGISTRY_VALUE_PLAYLIST_LENGTH, &configurationData.g_intPlaylistLength, DEFAULT_PLAYLIST_LENGTH );
	}

	LoadSettingString( MY_REGISTRY_VALUE_GUID, configurationData.g_szGuid, sizeof(configurationData.g_szGuid), "" );
	LoadSettingString( MY_REGISTRY_VALUE_EMAIL, configurationData.g_szEmail, sizeof(configurationData.g_szEmail), "" );
	LoadSettingString( MY_REGISTRY_VALUE_LICENSE_KEY, configurationData.g_szLicenseKey, sizeof(configurationData.g_szLicenseKey), "" );
	LoadSettingString( MY_REGISTRY_VALUE_FILE, configurationData.g_szOutputFile, sizeof(configurationData.g_szOutputFile), "/tmp/now_playing.xml" );
	LoadSettingString( MY_REGISTRY_VALUE_HOST, configurationData.g_szFTPHost, sizeof(configurationData.g_szFTPHost), "" );
	LoadSettingString( MY_REGISTRY_VALUE_USER, configurationData.g_szFTPUser, sizeof(configurationData.g_szFTPUser), "" );
	LoadSettingString( MY_REGISTRY_VALUE_PASSWORD, szTemp, sizeof(szTemp), "" );
	LoadSettingString( MY_REGISTRY_VALUE_PATH, configurationData.g_szFTPPath, sizeof(configurationData.g_szFTPPath), "/tmp/now_playing.xml" );
	LoadSettingNumber( MY_REGISTRY_VALUE_PROTOCOL, &configurationData.g_intUploadProtocol, 1 );
	LoadSettingNumber( MY_REGISTRY_VALUE_XML_ENCODING, &configurationData.g_intXmlEncoding, 1 );
	LoadSettingNumber( MY_REGISTRY_VALUE_XML_CDATA, &configurationData.g_intUseXmlCData, 1 );
	LoadSettingNumber( MY_REGISTRY_VALUE_ARTWORK_UPLOAD, &configurationData.g_intArtworkUpload, DEFAULT_ARTWORK_UPLOAD );
	LoadSettingNumber( MY_REGISTRY_VALUE_ARTWORK_WIDTH, &configurationData.g_intArtworkWidth, DEFAULT_ARTWORK_WIDTH );
	LoadSettingNumber( MY_REGISTRY_VALUE_PUBLISH_STOP, &configurationData.g_intPublishStop, DEFAULT_PUBLISH_STOP );
	LoadSettingNumber( MY_REGISTRY_VALUE_ARTWORK_EXPORT, &configurationData.g_intArtworkExport, DEFAULT_ARTWORK_EXPORT );
	LoadSettingNumber( MY_REGISTRY_VALUE_SKIPSHORT, &configurationData.g_intSkipShort, DEFAULT_SKIPSHORT_SECONDS );
	LoadSettingString( MY_REGISTRY_VALUE_SKIPKINDS, configurationData.g_szSkipKinds, sizeof(configurationData.g_szSkipKinds), "" );
	LoadSettingString( MY_REGISTRY_VALUE_TRACKBACK_URL, configurationData.g_szTrackBackUrl, sizeof(configurationData.g_szTrackBackUrl), "" );
	LoadSettingString( MY_REGISTRY_VALUE_PING_EXTRA_INFO, configurationData.g_szPingExtraInfo, sizeof(configurationData.g_szPingExtraInfo), "" );
	LoadSettingNumber( MY_REGISTRY_VALUE_AMAZON_ENABLED, &configurationData.g_intAmazonLookup, DEFAULT_AMAZON_ENABLED );
	LoadSettingNumber( MY_REGISTRY_VALUE_AMAZON_USE_ASIN, &configurationData.g_intAmazonUseASIN, DEFAULT_AMAZON_USE_ASIN );
	LoadSettingNumber( MY_REGISTRY_VALUE_AMAZON_LOCALE, &configurationData.g_intAmazonLocale, AMAZON_LOCALE_US );
	LoadSettingString( MY_REGISTRY_VALUE_AMAZON_ASSOCIATE, configurationData.g_szAmazonAssociate, sizeof(configurationData.g_szAmazonAssociate), "" );
	LoadSettingNumber( MY_REGISTRY_VALUE_APPLE_ENABLED, &configurationData.g_intAppleLookup, DEFAULT_APPLE_ENABLED );
	LoadSettingString( MY_REGISTRY_VALUE_APPLE_ASSOCIATE, configurationData.g_szAppleAssociate, sizeof(configurationData.g_szAppleAssociate), "" );
	LoadSettingNumberLong( MY_REGISTRY_VALUE_UPDATE_TIMESTAMP, &configurationData.g_timeLastUpdateCheck, 0 );
	LoadSettingNumberLong( MY_REGISTRY_VALUE_EXPORT_TIMESTAMP, &configurationData.g_timetExportedMarker, 0 );
	LoadSettingNumber( MY_REGISTRY_VALUE_LOGGING, &configurationData.g_intLogging, DEFAULT_LOG_LEVEL );
	LoadSettingNumber( MY_REGISTRY_VALUE_FACEBOOK_ENABLED, &configurationData.g_intFacebookEnabled, 0 );
	LoadSettingString( MY_REGISTRY_VALUE_FACEBOOK_MESSAGE, configurationData.g_szFacebookMessage, sizeof(configurationData.g_szFacebookMessage), "" );
	LoadSettingString( MY_REGISTRY_VALUE_FACEBOOK_ATTACHMENT_DESCRIPTION, configurationData.g_szFacebookAttachmentDescription, sizeof(configurationData.g_szFacebookAttachmentDescription), "" );
	LoadSettingString( MY_REGISTRY_VALUE_FACEBOOK_SESSION_KEY, configurationData.g_szFacebookSessionKey, sizeof(configurationData.g_szFacebookSessionKey), "" );
	LoadSettingNumber( MY_REGISTRY_VALUE_FACEBOOK_RATE_LIMIT_MINUTES, &configurationData.g_intFacebookRateLimitMinutes, FACEBOOK_RATE_LIMIT_MINUTES_DEFAULT );
	LoadSettingString( MY_REGISTRY_VALUE_FACEBOOK_SCREENNAME, configurationData.g_szFacebookUsername, sizeof(configurationData.g_szFacebookUsername), "" );
	LoadSettingNumberLong( MY_REGISTRY_VALUE_FACEBOOK_LATEST, &configurationData.g_timetFacebookLatest, 0 );
	LoadSettingNumber( MY_REGISTRY_VALUE_TWITTER_ENABLED, &configurationData.g_intTwitterEnabled, 0 );
	LoadSettingString( MY_REGISTRY_VALUE_TWITTER_MESSAGE, configurationData.g_szTwitterMessage, sizeof(configurationData.g_szTwitterMessage), "" );
	LoadSettingString( MY_REGISTRY_VALUE_TWITTER_KEY, configurationData.g_szTwitterAuthKey, sizeof(configurationData.g_szTwitterAuthKey), "" );
	LoadSettingString( MY_REGISTRY_VALUE_TWITTER_SECRET, configurationData.g_szTwitterAuthSecret, sizeof(configurationData.g_szTwitterAuthSecret), "" );
	LoadSettingNumber( MY_REGISTRY_VALUE_TWITTER_RATE_LIMIT_MINUTES, &configurationData.g_intTwitterRateLimitMinutes, TWITTER_RATE_LIMIT_MINUTES_DEFAULT );
	LoadSettingNumberLong( MY_REGISTRY_VALUE_TWITTER_LATEST, &configurationData.g_timetTwitterLatest, 0 );	
	LoadSettingString( MY_REGISTRY_VALUE_TWITTER_SCREENNAME, configurationData.g_szTwitterUsername, sizeof(configurationData.g_szTwitterUsername), "" );
		
	if (strchr(configurationData.g_szAmazonAssociate, '@') != NULL)
	{
		strcpy(configurationData.g_szAmazonAssociate, "");
	}
	
	if ( szTemp[0] != '\0' )
	{
		DecryptString( szTemp, configurationData.g_szFTPPassword, MAX_PATH );
	}

	g_fLicensed = IsLicensed( MY_SECRET_KEY, configurationData.g_szEmail, MY_REGISTRY_KEY, configurationData.g_szLicenseKey ) == 1 ? true : false;
	
	if (strlen(configurationData.g_szGuid) == 0)
	{
		CFUUIDRef uuid = CFUUIDCreate(kCFAllocatorDefault);
		if (uuid)
		{
			CFStringRef strUuid = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@"), uuid);
			if (strUuid)
			{
				CFStringGetCString(strUuid, configurationData.g_szGuid, sizeof(configurationData.g_szGuid), kCFStringEncodingUTF8);
								
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

	AMLOGDEBUG("Begin Cleanup\n");

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
		AMLOGDEBUG("Task Cleanup Completed\n");

		if ( g_paPlaylist )
		{
			free( g_paPlaylist );
			g_paPlaylist = NULL;
		}
	}
	else
	{
		AMLOGERROR("Task Cleanup NOT Completed %d\n", (int) err);
	}

	//
	// Cleanup events
	//

	MPDeleteEvent( g_hSignalPlayEvent );
	MPDeleteEvent( g_hSignalCleanupEvent );

	AMLOGDEBUG("End Cleanup\n");
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

void ResetExportCache()
{
	configurationData.g_timetExportedMarker = time(NULL);
	CFNumberRef numPrefValue = CFNumberCreate(NULL, kCFNumberLongType, &configurationData.g_timetExportedMarker);
	if (numPrefValue != NULL)
	{
		CFPreferencesSetAppValue(CFSTR(MY_REGISTRY_VALUE_EXPORT_TIMESTAMP), numPrefValue, CFSTR(kBundleName));
		CFPreferencesAppSynchronize(CFSTR(kBundleName));
		CFRelease(numPrefValue);
	}
}

void CacheVersion()
{
	CFBundleRef theBundle = CFBundleGetBundleWithIdentifier( CFSTR( kBundleName ) );
	if ( theBundle )
	{
		CFStringRef strVersionMy = (CFStringRef) CFBundleGetValueForInfoDictionaryKey( theBundle, CFSTR("CFBundleShortVersionString") );
		if ( strVersionMy )
		{
			CFRetain( strVersionMy );
			
			CFStringGetCString( strVersionMy, &configurationData.g_szVersion[0], sizeof(configurationData.g_szVersion), kCFStringEncodingUTF8 );
			
			CFRelease( strVersionMy );
		}
	}
}

void DoInit()
{
	int i = 0;

	memset(&configurationData, 0, sizeof(configurationData));

	configurationData.g_intLogging = DEFAULT_LOG_LEVEL;
	configurationData.g_intXmlEncoding = 1;
	configurationData.g_intUploadProtocol = 0;
	configurationData.g_intPlaylistLength = DEFAULT_PLAYLIST_LENGTH;
	configurationData.g_intArtworkExport = DEFAULT_ARTWORK_EXPORT;
	configurationData.g_intArtworkUpload = DEFAULT_ARTWORK_UPLOAD;
	configurationData.g_intArtworkWidth = DEFAULT_ARTWORK_WIDTH;
	configurationData.g_intPublishStop = DEFAULT_PUBLISH_STOP;
	configurationData.g_intSkipShort = DEFAULT_SKIPSHORT_SECONDS;
	configurationData.g_intAmazonLookup = DEFAULT_AMAZON_ENABLED;
	configurationData.g_intAmazonLocale = AMAZON_LOCALE_US;
	configurationData.g_intAmazonUseASIN = DEFAULT_AMAZON_USE_ASIN;
	configurationData.g_intAppleLookup = DEFAULT_APPLE_ENABLED;
	configurationData.g_intNext = 0;
	configurationData.g_intPlaylistBufferDelay = 0;
	configurationData.g_intFacebookEnabled = 0;
	configurationData.g_intFacebookRateLimitMinutes = FACEBOOK_RATE_LIMIT_MINUTES_DEFAULT;
	configurationData.g_timetFacebookLatest = 0;
	configurationData.g_intTwitterEnabled = 0;
	configurationData.g_timetExportedMarker = 0;
	configurationData.g_intTwitterRateLimitMinutes = TWITTER_RATE_LIMIT_MINUTES_DEFAULT;
	configurationData.g_timetTwitterLatest = 0;	

	CacheMySettings( true );
	CacheVersion();
	
	g_paPlaylist = (PlaylistItem*) calloc( configurationData.g_intPlaylistLength, sizeof( PlaylistItem ) );
	
	for ( i = 0; i < configurationData.g_intPlaylistLength; i++ )
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
		
		MessageBox(NSInformationalAlertStyle, CFSTR("It appears that this is the first time you have used the Now Playing plugin. The configuration window will appear now so you can set your preferences."));
		
		ConfigureVisual(nil, &configurationData);
	}
	else
	{
		DoSoftwareUpdateCheck();
	}
	
	DoGoogleAnalytics(GOOGLE_ANALYTICS_EVENT_LAUNCH);
}

static OSStatus VisualPluginHandler(OSType message, VisualPluginMessageInfo *messageInfo, void *refCon)
{
	OSStatus status = noErr;
	VisualPluginData * visualPluginData = (VisualPluginData*) refCon;

	//AMLOGDEBUG("Begin VisualPluginHandler(%lu)\n", message);

	switch ( message )
	{
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

			messageInfo->u.initMessage.refCon = (void*) visualPluginData;

			break;
		}
			
		case kVisualPluginCleanupMessage:
		{
			if (visualPluginData != nil)
			{
				DisposePtr((Ptr)visualPluginData);
			}
			
			break;
		}

		case kVisualPluginConfigureMessage:
		{
			status = ConfigureVisual(visualPluginData, &configurationData);

			break;
		}

		case kVisualPluginPlayMessage:
		{			
			if (messageInfo->u.playMessage.trackInfo != nil)
			{
#ifdef __LP64__		
				visualPluginData->trackInfo = *messageInfo->u.playMessage.trackInfo;
#else
				visualPluginData->trackInfo = *messageInfo->u.playMessage.trackInfoUnicode;
#endif				
			}
			else
			{
				MemClear(&visualPluginData->trackInfo,sizeof(visualPluginData->trackInfo));
			}
			
			if (messageInfo->u.playMessage.streamInfo != nil)
			{
#ifdef __LP64__
				visualPluginData->streamInfo = *messageInfo->u.playMessage.streamInfo;
#else
				visualPluginData->streamInfo = *messageInfo->u.playMessage.streamInfoUnicode;
#endif
			}
			else
			{
				MemClear(&visualPluginData->streamInfo,sizeof(visualPluginData->streamInfo));
			}
			
			visualPluginData->playing = true;
		
			DoUpdate( &visualPluginData->trackInfo, true );
		
			break;
		}

		case kVisualPluginChangeTrackMessage:
		{
			if (messageInfo->u.changeTrackMessage.trackInfo != nil)
			{
#ifdef __LP64__
				visualPluginData->trackInfo = *messageInfo->u.changeTrackMessage.trackInfo;
#else
				visualPluginData->trackInfo = *messageInfo->u.changeTrackMessage.trackInfoUnicode;			
#endif				
			}
			else
			{
				MemClear(&visualPluginData->trackInfo,sizeof(visualPluginData->trackInfo));
			}
			
			if (messageInfo->u.changeTrackMessage.streamInfo != nil)
			{
#ifdef __LP64__
				visualPluginData->streamInfo = *messageInfo->u.changeTrackMessage.streamInfo;
#else
				visualPluginData->streamInfo = *messageInfo->u.changeTrackMessage.streamInfoUnicode;
#endif
			}
			else
			{
				MemClear(&visualPluginData->streamInfo,sizeof(visualPluginData->streamInfo));
			}
			
			DoUpdate( &visualPluginData->trackInfo, true );

			break;
		}

		case kVisualPluginStopMessage:
		{
			visualPluginData->playing = false;
						
			break;
		}

		case kVisualPluginEnableMessage:
		case kVisualPluginDisableMessage:
		{
			break;
		}

		default:
		{
			//AMLOGDEBUG("Message = %d\n", message);
			status = unimpErr;
			break;
		}
	}
	
	//AMLOGDEBUG("End VisualPluginHandler (%d)\n", status);

	return status;	
}

OSStatus RegisterVisualPlugin(PluginMessageInfo* messageInfo)
{
	PlayerMessageInfo playerMessageInfo;

	MemClear(&playerMessageInfo.u.registerVisualPluginMessage, sizeof(playerMessageInfo.u.registerVisualPluginMessage));	

#ifdef __LP64__
	
	GetVisualName(playerMessageInfo.u.registerVisualPluginMessage.name);

	SetNumVersion(&playerMessageInfo.u.registerVisualPluginMessage.pluginVersion, kTVisualPluginMajorVersion, kTVisualPluginMinorVersion, kTVisualPluginReleaseStage, kTVisualPluginNonFinalRelease);
	
	playerMessageInfo.u.registerVisualPluginMessage.options					= GetVisualOptions();
	playerMessageInfo.u.registerVisualPluginMessage.handler					= (VisualPluginProcPtr) VisualPluginHandler;
	playerMessageInfo.u.registerVisualPluginMessage.registerRefCon			= 0;
	playerMessageInfo.u.registerVisualPluginMessage.creator					= kTVisualPluginCreator;
	
	playerMessageInfo.u.registerVisualPluginMessage.pulseRateInHz			= kStoppedPulseRateInHz;	// update my state N times a second
	playerMessageInfo.u.registerVisualPluginMessage.numWaveformChannels		= 2;
	playerMessageInfo.u.registerVisualPluginMessage.numSpectrumChannels		= 2;
	
	playerMessageInfo.u.registerVisualPluginMessage.minWidth				= 64;
	playerMessageInfo.u.registerVisualPluginMessage.minHeight				= 64;
	playerMessageInfo.u.registerVisualPluginMessage.maxWidth				= 0;	// no max width limit
	playerMessageInfo.u.registerVisualPluginMessage.maxHeight				= 0;	// no max height limit
	
#else
	
	CFStringGetPascalString(kTVisualPluginName, (unsigned char *) &playerMessageInfo.u.registerVisualPluginMessage.name[0], 64, 0);
	
	SetNumVersion( &playerMessageInfo.u.registerVisualPluginMessage.pluginVersion, kTVisualPluginMajorVersion, kTVisualPluginMinorVersion, kTVisualPluginReleaseStage, kTVisualPluginNonFinalRelease );
	
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
										
#endif
	
	return PlayerRegisterVisualPlugin(messageInfo->u.initMessage.appCookie, messageInfo->u.initMessage.appProc, &playerMessageInfo);
}

void UpdateInfoTimeOut( VisualPluginData * visualPluginData )
{
	// reset the timeout value we will use to show the info/artwork if we have it during DrawVisual()
	visualPluginData->drawInfoTimeOut = time( NULL ) + kInfoTimeOutInSeconds;
}