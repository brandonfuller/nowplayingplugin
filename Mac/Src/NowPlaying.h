#ifndef NOWPLAYING_H
#define NOWPLAYING_H

#ifdef __LP64__
#include "iTunesVisualAPI-20.h"
#else
#include "iTunesVisualAPI-12.h"
#endif

#include <time.h>

#if TARGET_OS_WIN32
#include <Gdiplus.h>
#endif // TARGET_OS_WIN32

#define MAX_PATH 256

typedef struct ConfigurationData
{
	char g_szEmail[MAX_PATH];
	char g_szLicenseKey[MAX_PATH];
	char g_szKey[MAX_PATH];
	int g_intLogging;
	char g_szVersion[MAX_PATH];
	char g_szGuid[MAX_PATH];
	char g_szOutputFile[MAX_PATH];
	char g_szSkipKinds[MAX_PATH];
	int g_intXmlEncoding;
	int g_intUploadProtocol;
	int g_intPlaylistLength;
	char g_szFTPHost[MAX_PATH];
	char g_szFTPUser[MAX_PATH];
	char g_szFTPPassword[MAX_PATH];
	char g_szFTPPath[MAX_PATH];
	int g_intArtworkExport;
	int g_intArtworkUpload;
	int g_intArtworkWidth;
	int g_intPublishStop;
	int g_intSkipShort;
	char g_szTrackBackUrl[MAX_PATH];
	char g_szPingExtraInfo[MAX_PATH];
	int g_intAmazonLookup;
	int g_intAmazonLocale;
	char g_szAmazonAssociate[MAX_PATH];
	int g_intAmazonUseASIN;
	int g_intAppleLookup;
	char g_szAppleAssociate[MAX_PATH];
	time_t g_timetChanged;
	time_t g_timeLastUpdateCheck;
	int g_intNext;
	int g_intPlaylistBufferDelay;
	int g_intFacebookEnabled;
	char g_szFacebookMessage[1024];
	char g_szFacebookAttachmentDescription[1024];
	char g_szFacebookAuthToken[MAX_PATH];
	char g_szFacebookSessionKey[MAX_PATH];
	char g_szFacebookUsername[MAX_PATH];
	int g_intFacebookRateLimitMinutes;
	time_t g_timetFacebookLatest;
	int g_intTwitterEnabled;
	char g_szTwitterMessage[1024];
	time_t g_timetExportedMarker;
	char g_szTwitterAuthKey[1024];
	char g_szTwitterAuthSecret[1024];
	char g_szTwitterUsername[MAX_PATH];
	int g_intTwitterRateLimitMinutes;
	time_t g_timetTwitterLatest;	
	int g_intUseXmlCData;
}
ConfigurationData;

extern ConfigurationData configurationData;

//-------------------------------------------------------------------------------------------------
//	build flags
//-------------------------------------------------------------------------------------------------
#define USE_SUBVIEW						(TARGET_OS_MAC && 1)		// use a custom NSView subview on the Mac

//-------------------------------------------------------------------------------------------------
//	typedefs, structs, enums, etc.
//-------------------------------------------------------------------------------------------------

#define	kTVisualPluginCreator			'hook'
#define kTVisualPluginName              CFSTR("Now Playing")
#define	kTVisualPluginMajorVersion		2
#define	kTVisualPluginMinorVersion		0
#define	kTVisualPluginReleaseStage		finalStage
#define	kTVisualPluginNonFinalRelease	0

#define kBundleName "name.brandon.fuller.nowplaying.mac.itunes"
#define MY_REGISTRY_KEY "SOFTWARE\\Brandon Fuller\\Now Playing\\iTunes\\Mac"

#define FACEBOOK_TAGS_SUPPORTED "<title> <artist> <album> <genre> <kind> <track> <numTracks> <year> <comments> <time> <bitrate> <rating> <disc> <numDiscs> <playCount> <composer> <grouping> <file> <image> <imageSmall> <imageLarge> <urlAmazon> <hasAmazon> </hasAmazon>"
#define TWITTER_TAGS_SUPPORTED "<title> <artist> <album> <genre> <kind> <track> <numTracks> <year> <comments> <time> <bitrate> <rating> <disc> <numDiscs> <playCount> <composer> <grouping> <file> <image> <imageSmall> <imageLarge> <urlAmazon> <hasAmazon> </hasAmazon>"

#if TARGET_OS_MAC
#import <Cocoa/Cocoa.h>

// "namespace" our ObjC classname to avoid load conflicts between multiple visualizer plugins
#define VisualView		ComAppleExample_VisualView
#define GLVisualView	ComAppleExample_GLVisualView

@class VisualView;
@class GLVisualView;

#endif

#define kInfoTimeOutInSeconds		10							// draw info/artwork for N seconds when it changes or playback starts
#define kPlayingPulseRateInHz		10							// when iTunes is playing, draw N times a second
#define kStoppedPulseRateInHz		5							// when iTunes is not playing, draw N times a second

struct VisualPluginData
{
	void *				appCookie;
	ITAppProcPtr		appProc;
	
#if TARGET_OS_MAC
	NSView*				destView;
	NSRect				destRect;
#if USE_SUBVIEW
	VisualView*			subview;								// custom subview
#endif
	NSImage *			currentArtwork;
#else
	HWND				destView;
	RECT				destRect;
	Gdiplus::Bitmap* 	currentArtwork;
	long int			lastDrawTime;
#endif
	OptionBits			destOptions;
	
	RenderVisualData	renderData;
	UInt32				renderTimeStampID;
	
	ITTrackInfo			trackInfo;
	ITStreamInfo		streamInfo;
	
	// Plugin-specific data
	
	Boolean				playing;								// is iTunes currently playing audio?
	Boolean				padding[3];
	
	time_t				drawInfoTimeOut;						// when should we stop showing info/artwork?
	
	UInt8				minLevel[kVisualMaxDataChannels];		// 0-128
	UInt8				maxLevel[kVisualMaxDataChannels];		// 0-128
};
typedef struct VisualPluginData VisualPluginData;

void		GetVisualName( ITUniStr255 name );
OptionBits	GetVisualOptions( void );
OSStatus	RegisterVisualPlugin( PluginMessageInfo * messageInfo );

#ifdef __LP64__
OSStatus	ActivateVisual( VisualPluginData * visualPluginData, VISUAL_PLATFORM_VIEW destView, OptionBits options );
OSStatus	MoveVisual( VisualPluginData * visualPluginData, OptionBits newOptions );
OSStatus	DeactivateVisual( VisualPluginData * visualPluginData );
OSStatus	ResizeVisual( VisualPluginData * visualPluginData );
void		ProcessRenderData( VisualPluginData * visualPluginData, UInt32 timeStampID, const RenderVisualData * renderData );
void		ResetRenderData( VisualPluginData * visualPluginData );
void		UpdateInfoTimeOut( VisualPluginData * visualPluginData );
void		UpdateTrackInfo( VisualPluginData * visualPluginData, ITTrackInfo * trackInfo, ITStreamInfo * streamInfo );
void		UpdateArtwork( VisualPluginData * visualPluginData, VISUAL_PLATFORM_DATA coverArt, UInt32 coverArtSize, UInt32 coverArtFormat );
void		UpdatePulseRate( VisualPluginData * visualPluginData, UInt32 * ioPulseRate );
void		DrawVisual( VisualPluginData * visualPluginData );
void		PulseVisual( VisualPluginData * visualPluginData, UInt32 timeStampID, const RenderVisualData * renderData, UInt32 * ioPulseRate );
void		InvalidateVisual( VisualPluginData * visualPluginData );
#endif

OSStatus	ConfigureVisual( VisualPluginData * visualPluginData, ConfigurationData* configurationData );


#ifdef __cplusplus
extern "C"
{
#endif
	
void DoInit();
void Cleanup();
void MessageBox(NSAlertStyle alertType, CFStringRef message);
void CacheMySettings(bool fFirst);
void SaveMyStringPreference(CFStringRef strPrefName, NSString* string, bool fEncrypted);
void SaveMyNumberPreference(CFStringRef strPrefName, int number);
void ResetExportCache();
void DoTwitterAuthorize();
void DoTwitterVerify(CFStringRef strPin);
void DoTwitterReset();
void DoFacebookReset();
void DoFacebookAdd();
void DoFacebookAuthorize();
	
#ifdef	__cplusplus
}
#endif

#endif