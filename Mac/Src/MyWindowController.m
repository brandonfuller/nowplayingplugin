#import "Defines.h"
#import "License.h"
#import "NowPlaying.h"
#import "MyWindowController.h"

@implementation MyWindowController

- (id)init
{
	self = [super initWithWindowNibName:@"SettingsDialog" owner:self];
	return self;
}

- (void)populateFields:(ConfigurationData *)configurationData
{
	//
	// Select first tab
	//

	[tabViewControl selectFirstTabViewItem:self];
	
	//
	// Set static text
	//
	
	[versionLabel setStringValue:[[NSString alloc] initWithFormat:@"Version: %@",[[NSString alloc] initWithCString:configurationData->g_szVersion]]];
	[facebookTagsLabel setStringValue:(NSString*) CFSTR(FACEBOOK_TAGS_SUPPORTED)];
	[twitterTagsLabel setStringValue:(NSString*) CFSTR(TWITTER_TAGS_SUPPORTED)];	

	//
	// Textfields: Numbers
	//

	[optionsArtworkWidthField setIntegerValue:configurationData->g_intArtworkWidth];
	[optionSkipShorterThanField setIntegerValue:configurationData->g_intSkipShort];
	[xmlPlaylistLengthField setIntegerValue:configurationData->g_intPlaylistLength];
	[twitterRateLimitField setIntegerValue:configurationData->g_intTwitterRateLimitMinutes];
	[facebookRateLimitField setIntegerValue:configurationData->g_intFacebookRateLimitMinutes];
	
	//
	// Textfields: Strings
	//

	[optionsSkipKindsField setStringValue:[[NSString alloc] initWithCString:configurationData->g_szSkipKinds]];
	[xmlOutputFileField setStringValue:[[NSString alloc] initWithCString:configurationData->g_szOutputFile]];
	[uploadHostnameField setStringValue:[[NSString alloc] initWithCString:configurationData->g_szFTPHost]];
	[uploadUsernameField setStringValue:[[NSString alloc] initWithCString:configurationData->g_szFTPUser]];
	[uploadPasswordField setStringValue:[[NSString alloc] initWithCString:configurationData->g_szFTPPassword]];
	[uploadFilenameField setStringValue:[[NSString alloc] initWithCString:configurationData->g_szFTPPath]];
	[pingUrlField setStringValue:[[NSString alloc] initWithCString:configurationData->g_szTrackBackUrl]];
	[pingExtraInfoField setStringValue:[[NSString alloc] initWithCString:configurationData->g_szPingExtraInfo]];
	[twitterMessageField setStringValue:[[NSString alloc] initWithCString:configurationData->g_szTwitterMessage]];
	[facebookCaptionField setStringValue:[[NSString alloc] initWithCString:configurationData->g_szFacebookMessage]];
	[facebookDescriptionField setStringValue:[[NSString alloc] initWithCString:configurationData->g_szFacebookAttachmentDescription]];
	[amazonAssociateIdField setStringValue:[[NSString alloc] initWithCString:configurationData->g_szAmazonAssociate]];
	[appleAffiliateIdField setStringValue:[[NSString alloc] initWithCString:configurationData->g_szAppleAssociate]];
	[licenseEmailField setStringValue:[[NSString alloc] initWithCString:configurationData->g_szEmail]];
	[licenseKeyField setStringValue:[[NSString alloc] initWithUTF8String:configurationData->g_szLicenseKey]];

	//
	// Checkboxes
	//

	[optionsPublishStopField setState: configurationData->g_intPublishStop != 0 ? NSOnState : NSOffState];
	[optionsExportArtworkField setState: configurationData->g_intArtworkExport != 0 ? NSOnState : NSOffState];	
	[xmlCDataField setState: configurationData->g_intUseXmlCData != 0 ? NSOnState : NSOffState];
	[uploadArtworkField setState: configurationData->g_intArtworkUpload != 0 ? NSOnState : NSOffState];
	[twitterEnabledField setState: configurationData->g_intTwitterEnabled != 0 ? NSOnState : NSOffState];
	[facebookEnabledField setState: configurationData->g_intFacebookEnabled != 0 ? NSOnState : NSOffState];
	[amazonEnabledField setState: configurationData->g_intAmazonLookup != 0 ? NSOnState : NSOffState];
	[amazonAsinHintField setState: configurationData->g_intAmazonUseASIN != 0 ? NSOnState : NSOffState];
	[appleEnabledField setState: configurationData->g_intAppleLookup != 0 ? NSOnState : NSOffState];

	//
	// Comboboxes
	//

	[xmlEncodingField addItemWithObjectValue:[[NSString alloc] initWithUTF8String:XML_ENCODING_LABEL_ISO_8859_1]];
	[xmlEncodingField addItemWithObjectValue:[[NSString alloc] initWithUTF8String:XML_ENCODING_LABEL_UTF_8]];
	[xmlEncodingField selectItemAtIndex:(configurationData->g_intXmlEncoding == 0 ? 1 : configurationData->g_intXmlEncoding - 1)];
	[xmlEncodingField setObjectValue:[xmlEncodingField objectValueOfSelectedItem]];
	
	[optionsLoggingField addItemWithObjectValue:[[NSString alloc] initWithUTF8String:LOGGING_LABEL_NONE]];
	[optionsLoggingField addItemWithObjectValue:[[NSString alloc] initWithUTF8String:LOGGING_LABEL_ERROR]];
	[optionsLoggingField addItemWithObjectValue:[[NSString alloc] initWithUTF8String:LOGGING_LABEL_INFO]];
	[optionsLoggingField addItemWithObjectValue:[[NSString alloc] initWithUTF8String:LOGGING_LABEL_DEBUG]];
	[optionsLoggingField selectItemAtIndex:(configurationData->g_intLogging == 0 ? 1 : configurationData->g_intLogging - 1)];
	[optionsLoggingField setObjectValue:[optionsLoggingField objectValueOfSelectedItem]];

	[amazonLocaleField addItemWithObjectValue:[[NSString alloc] initWithUTF8String:AMAZON_LOCALE_LABEL_CA]];
	[amazonLocaleField addItemWithObjectValue:[[NSString alloc] initWithUTF8String:AMAZON_LOCALE_LABEL_DE]];
	[amazonLocaleField addItemWithObjectValue:[[NSString alloc] initWithUTF8String:AMAZON_LOCALE_LABEL_FR]];
	[amazonLocaleField addItemWithObjectValue:[[NSString alloc] initWithUTF8String:AMAZON_LOCALE_LABEL_JP]];
	[amazonLocaleField addItemWithObjectValue:[[NSString alloc] initWithUTF8String:AMAZON_LOCALE_LABEL_US]];
	[amazonLocaleField addItemWithObjectValue:[[NSString alloc] initWithUTF8String:AMAZON_LOCALE_LABEL_UK]];	
	[amazonLocaleField selectItemAtIndex:(configurationData->g_intAmazonLocale == 0 ? 1 : configurationData->g_intAmazonLocale - 1)];
	[amazonLocaleField setObjectValue:[amazonLocaleField objectValueOfSelectedItem]];
	
	[uploadProtocolField addItemWithObjectValue:[[NSString alloc] initWithUTF8String:UPLOAD_PROTOCOL_LABEL_NONE]];
	[uploadProtocolField addItemWithObjectValue:[[NSString alloc] initWithUTF8String:UPLOAD_PROTOCOL_LABEL_FTP]];
	[uploadProtocolField addItemWithObjectValue:[[NSString alloc] initWithUTF8String:UPLOAD_PROTOCOL_LABEL_SFTP]];
	[uploadProtocolField selectItemAtIndex:(configurationData->g_intUploadProtocol == 0 ? 1: configurationData->g_intUploadProtocol - 1)];
	[uploadProtocolField setObjectValue:[uploadProtocolField objectValueOfSelectedItem]];
	
	//
	// Setup UI
	//

	[self doFacebookUiState:self];
	[self doTwitterUiState:self];	
}

- (IBAction) doOkButton:(id)sender 
{
	NSString* email = [licenseEmailField stringValue];
	NSString* key = [licenseKeyField stringValue];
	
	if ([email length] > 0 && [key length] > 0 )
	{
		char szEmail[128];
		char szLicenseKey[128];

		[email getCString:&szEmail[0] maxLength:sizeof(szEmail) encoding:NSUTF8StringEncoding];
		[key getCString:&szLicenseKey[0] maxLength:sizeof(szLicenseKey) encoding:NSUTF8StringEncoding];

		if (IsLicensed(MY_SECRET_KEY, szEmail, MY_REGISTRY_KEY, szLicenseKey) == 0)
		{
			MessageBox(NSCriticalAlertStyle, CFSTR("Invalid license key. Try again."));			
			return;
		}
	}
	
	// Set as configured
	SInt16 nValue = 1;
	CFNumberRef numPrefValue = CFNumberCreate( NULL, kCFNumberSInt16Type, &nValue );
	CFPreferencesSetAppValue( CFSTR( MY_REGISTRY_VALUE_CONFIGURED ), numPrefValue, CFSTR( kBundleName ) );
	CFRelease( numPrefValue );
	
	// Options
	SaveMyNumberPreference(CFSTR(MY_REGISTRY_VALUE_PUBLISH_STOP), [optionsPublishStopField intValue]);
	SaveMyNumberPreference(CFSTR(MY_REGISTRY_VALUE_ARTWORK_EXPORT), [optionsExportArtworkField intValue]);
	SaveMyNumberPreference(CFSTR(MY_REGISTRY_VALUE_ARTWORK_WIDTH), [optionsArtworkWidthField intValue]);
	SaveMyNumberPreference(CFSTR(MY_REGISTRY_VALUE_LOGGING), [optionsLoggingField indexOfSelectedItem] + 1);
	SaveMyNumberPreference(CFSTR(MY_REGISTRY_VALUE_SKIPSHORT), [optionSkipShorterThanField intValue]);
	SaveMyStringPreference(CFSTR(MY_REGISTRY_VALUE_SKIPKINDS), [optionsSkipKindsField stringValue], false);

	// XML
	SaveMyStringPreference(CFSTR(MY_REGISTRY_VALUE_FILE), [xmlOutputFileField stringValue],false);
	SaveMyNumberPreference(CFSTR(MY_REGISTRY_VALUE_XML_CDATA), [xmlCDataField intValue]);
	SaveMyNumberPreference(CFSTR(MY_REGISTRY_VALUE_PLAYLIST_LENGTH), [xmlPlaylistLengthField intValue]);
	SaveMyNumberPreference(CFSTR(MY_REGISTRY_VALUE_XML_ENCODING), [xmlEncodingField indexOfSelectedItem] + 1);
	
	// Upload
	SaveMyNumberPreference(CFSTR(MY_REGISTRY_VALUE_PROTOCOL), [uploadProtocolField indexOfSelectedItem] + 1);
	SaveMyStringPreference(CFSTR(MY_REGISTRY_VALUE_HOST), [uploadHostnameField stringValue], false);
	SaveMyStringPreference(CFSTR(MY_REGISTRY_VALUE_USER), [uploadUsernameField stringValue], false);
	SaveMyStringPreference(CFSTR(MY_REGISTRY_VALUE_PASSWORD), [uploadPasswordField stringValue], true);
	SaveMyStringPreference(CFSTR(MY_REGISTRY_VALUE_PATH), [uploadFilenameField stringValue], false);
	SaveMyNumberPreference(CFSTR(MY_REGISTRY_VALUE_ARTWORK_UPLOAD), [uploadArtworkField intValue]);
	
	// Ping
	SaveMyStringPreference(CFSTR(MY_REGISTRY_VALUE_TRACKBACK_URL), [pingUrlField stringValue], false);
	SaveMyStringPreference(CFSTR(MY_REGISTRY_VALUE_PING_EXTRA_INFO), [pingExtraInfoField stringValue], false);
	
	// Twitter
	SaveMyNumberPreference(CFSTR(MY_REGISTRY_VALUE_TWITTER_ENABLED), [twitterEnabledField intValue]);
	SaveMyStringPreference(CFSTR(MY_REGISTRY_VALUE_TWITTER_MESSAGE), [twitterMessageField stringValue], false);
	SaveMyNumberPreference(CFSTR(MY_REGISTRY_VALUE_TWITTER_RATE_LIMIT_MINUTES), [twitterRateLimitField intValue]);
	
	// Facebook
	SaveMyNumberPreference(CFSTR(MY_REGISTRY_VALUE_FACEBOOK_ENABLED), [facebookEnabledField intValue]);
	SaveMyStringPreference(CFSTR(MY_REGISTRY_VALUE_FACEBOOK_MESSAGE), [facebookCaptionField stringValue], false);
	SaveMyStringPreference(CFSTR(MY_REGISTRY_VALUE_FACEBOOK_ATTACHMENT_DESCRIPTION), [facebookDescriptionField stringValue], false);
	SaveMyNumberPreference(CFSTR(MY_REGISTRY_VALUE_FACEBOOK_RATE_LIMIT_MINUTES), [facebookRateLimitField intValue]);
	
	// Amazon
	SaveMyNumberPreference(CFSTR(MY_REGISTRY_VALUE_AMAZON_ENABLED), [amazonEnabledField intValue]);
	SaveMyNumberPreference(CFSTR(MY_REGISTRY_VALUE_AMAZON_LOCALE), [amazonLocaleField indexOfSelectedItem] + 1);
	SaveMyStringPreference(CFSTR(MY_REGISTRY_VALUE_AMAZON_ASSOCIATE), [amazonAssociateIdField stringValue], false);
	SaveMyNumberPreference(CFSTR(MY_REGISTRY_VALUE_AMAZON_USE_ASIN), [amazonAsinHintField intValue]);
	
	// Apple
	SaveMyNumberPreference(CFSTR(MY_REGISTRY_VALUE_APPLE_ENABLED), [appleEnabledField intValue]);
	SaveMyStringPreference(CFSTR(MY_REGISTRY_VALUE_APPLE_ASSOCIATE), [appleAffiliateIdField stringValue], false);

	// License
	SaveMyStringPreference(CFSTR(MY_REGISTRY_VALUE_EMAIL), [licenseEmailField stringValue], false);
	SaveMyStringPreference(CFSTR(MY_REGISTRY_VALUE_LICENSE_KEY), [licenseKeyField stringValue], false);
	
	// Flush
	CFPreferencesAppSynchronize(CFSTR(kBundleName));
	
	// Reload
	CacheMySettings(false);

	//[[self window] close];
	
	int result = 12345;
	[NSApp stopModalWithCode:result];
	[NSApp endSheet:[self window] returnCode:result];
	
	[[self window] orderOut:sender];
	[[self window] close];	
}

- (IBAction)doResetExportCacheButton:(id)sender
{
	ResetExportCache();
}

- (IBAction)doTwitterAuthorizeButton:(id)sender
{
	DoTwitterAuthorize();
}

- (IBAction)doTwitterVerifyButton:(id)sender
{
	DoTwitterVerify((CFStringRef) [twitterPinField stringValue]);
	
	[self doTwitterUiState:self];
}

- (IBAction)doTwitterDefaultButton:(id)sender
{
	[twitterMessageField setStringValue:(NSString*) CFSTR(TWITTER_MESSAGE_DEFAULT)];
}

- (IBAction)doTwitterResetButton:(id)sender
{
	DoTwitterReset();

	[self doTwitterUiState:self];
}

- (IBAction)doFacebookUiState:(id)sender
{
	if (strlen(configurationData.g_szFacebookUsername) > 0)
	{
		[facebookAuthorizeButton setHidden:TRUE];
		[facebookValidateButton setHidden:TRUE];
		[facebookResetButton setHidden:FALSE];
		[facebookUsernameField setHidden:FALSE];
		
		[facebookUsernameField setStringValue:[[NSString alloc] initWithCString:configurationData.g_szFacebookUsername]];
	}
	else
	{
		[facebookAuthorizeButton setHidden:FALSE];
		[facebookValidateButton setHidden:FALSE];
		[facebookResetButton setHidden:TRUE];
		[facebookUsernameField setHidden:TRUE];
	}
}

- (IBAction)doTwitterUiState:(id)sender
{
	if (strlen(configurationData.g_szTwitterUsername) > 0)
	{
		[twitterUsernameField setHidden:FALSE];
		[twitterResetButton setHidden:FALSE];		
		[twitterPinLabelField setHidden:TRUE];
		[twitterAuthorizeButton setHidden:TRUE];
		[twitterVerifyButton setHidden:TRUE];
		[twitterPinField setHidden:TRUE];
		
		[twitterUsernameField setStringValue:[[NSString alloc] initWithCString:configurationData.g_szTwitterUsername]];
	}
	else
	{
		[twitterUsernameField setHidden:TRUE];
		[twitterResetButton setHidden:TRUE];		
		[twitterPinLabelField setHidden:FALSE];
		[twitterAuthorizeButton setHidden:FALSE];
		[twitterVerifyButton setHidden:FALSE];
		[twitterPinField setHidden:FALSE];
	}
}

- (IBAction)doFacebookAuthorizeButton:(id)sender
{
	DoFacebookAdd();
}

- (IBAction)doFacebookValidateButton:(id)sender
{
	DoFacebookAuthorize();
	
	[self doFacebookUiState:self];
}

- (IBAction)doFacebookResetButton:(id)sender
{
	DoFacebookReset();
	
	[self doFacebookUiState:self];
}

- (IBAction)doFacebookDefaultButton:(id)sender
{
	[facebookCaptionField setStringValue:(NSString*) CFSTR(FACEBOOK_MESSAGE_DEFAULT)];
}

- (IBAction)doCancelButton:(id)sender 
{
	int result = 12345;
	[NSApp stopModalWithCode:result];
	[NSApp endSheet:[self window] returnCode:result];
	
	[[self window] orderOut:sender];
	[[self window] close];
}

- (BOOL) windowShouldClose:(id)sender
{
	[self doCancelButton:sender];

	return YES;
}
 
- (void)awakeFromNib
{
	[optionsArtworkWidthField setDelegate:self];
	[optionSkipShorterThanField setDelegate:self];
	[xmlPlaylistLengthField setDelegate:self];
	[twitterRateLimitField setDelegate:self];
	[facebookRateLimitField setDelegate:self];
}

- (BOOL)control:(NSControl *)control isValidObject:(id)obj
{
    if (control == optionsArtworkWidthField)
	{
		if ([control intValue] <= 0)
		{
			NSRunAlertPanel(@"Now Playing", @"Options Artwork Width must be greater than 0.", nil, nil, nil);
			return NO;
		}		
    }
    else if (control == optionSkipShorterThanField)
	{
		if ([control intValue] < 0)
		{
			NSRunAlertPanel(@"Now Playing", @"Options Skip Shorter Than must be greater than or equal to 0.", nil, nil, nil);
			return NO;
		}		
	}
    else if (control == xmlPlaylistLengthField)
	{
		if ([control intValue] <= 0)
		{
			NSRunAlertPanel(@"Now Playing", @"XML Playlist Length must be greater than 0.", nil, nil, nil);
			return NO;
		}		
	}
    else if (control == twitterRateLimitField)
	{
		if ([control intValue] < 0)
		{
			NSRunAlertPanel(@"Now Playing", @"Twitter Rate Limit must be greater than 0 minutes.", nil, nil, nil);
			return NO;
		}		
	}
    else if (control == facebookRateLimitField)
	{
		if ([control intValue] < 30)
		{
			NSRunAlertPanel(@"Now Playing", @"Facebook Rate Limit must be 30 minutes greater. We are all trying to avoid being classified as spammy which will cause Facebook to shutdown this plugin once again.", nil, nil, nil);
			return NO;
		}
	}

    return YES;
}

@end