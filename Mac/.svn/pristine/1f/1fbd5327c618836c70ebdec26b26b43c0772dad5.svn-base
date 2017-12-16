#import <Cocoa/Cocoa.h>

@interface MyWindowController : NSWindowController
	<NSTextFieldDelegate,NSWindowDelegate>
{
	IBOutlet NSWindow* window;
	IBOutlet NSTabView* tabViewControl;
	IBOutlet NSTextField* versionLabel;
	IBOutlet NSButton* optionsPublishStopField;
	IBOutlet NSButton* optionsExportArtworkField;
	IBOutlet NSTextField* optionsArtworkWidthField;
	IBOutlet NSTextField* optionSkipShorterThanField;
	IBOutlet NSTextField* optionsSkipKindsField;
	IBOutlet NSComboBox* optionsLoggingField;
	IBOutlet NSTextField* xmlOutputFileField;
	IBOutlet NSTextField* xmlPlaylistLengthField;
	IBOutlet NSComboBox* xmlEncodingField;
	IBOutlet NSButton* xmlCDataField;
	IBOutlet NSComboBox* uploadProtocolField;
	IBOutlet NSTextField* uploadHostnameField;
	IBOutlet NSTextField* uploadUsernameField;
	IBOutlet NSTextField* uploadPasswordField;
	IBOutlet NSTextField* uploadFilenameField;
	IBOutlet NSButton* uploadArtworkField;
	IBOutlet NSTextField* pingUrlField;	
	IBOutlet NSTextField* pingExtraInfoField;
	IBOutlet NSButton* twitterEnabledField;
	IBOutlet NSTextField* twitterRateLimitField;
	IBOutlet NSTextField* twitterMessageField;
	IBOutlet NSTextField* twitterPinField;
	IBOutlet NSTextField* twitterUsernameField;
	IBOutlet NSTextField* twitterPinLabelField;
	IBOutlet NSButton* twitterResetButton;
	IBOutlet NSButton* twitterAuthorizeButton;
	IBOutlet NSButton* twitterVerifyButton;
	IBOutlet NSTextField* twitterTagsLabel;
	IBOutlet NSButton* facebookEnabledField;
	IBOutlet NSTextField* facebookRateLimitField;
	IBOutlet NSTextField* facebookCaptionField;
	IBOutlet NSTextField* facebookDescriptionField;
	IBOutlet NSButton* facebookDefaultButton;
	IBOutlet NSButton* facebookAuthorizeButton;
	IBOutlet NSButton* facebookValidateButton;
	IBOutlet NSButton* facebookResetButton;
	IBOutlet NSTextField* facebookUsernameField;
	IBOutlet NSTextField* facebookTagsLabel;
	IBOutlet NSButton* amazonEnabledField;
	IBOutlet NSTextField* amazonAssociateIdField;
	IBOutlet NSComboBox* amazonLocaleField;
	IBOutlet NSButton* amazonAsinHintField;
	IBOutlet NSButton* appleEnabledField;
	IBOutlet NSTextField* appleAffiliateIdField;
	IBOutlet NSTextField* licenseEmailField;
	IBOutlet NSTextField* licenseKeyField;
}

- (void)populateFields:(ConfigurationData *) configurationData;
- (IBAction)doResetExportCacheButton:(id)sender;
- (IBAction)doTwitterAuthorizeButton:(id)sender;
- (IBAction)doTwitterVerifyButton:(id)sender;
- (IBAction)doTwitterDefaultButton:(id)sender;
- (IBAction)doTwitterResetButton:(id)sender;
- (IBAction)doTwitterUiState:(id)sender;
- (IBAction)doFacebookResetButton:(id)sender;
- (IBAction)doFacebookAuthorizeButton:(id)sender;
- (IBAction)doFacebookValidateButton:(id)sender;
- (IBAction)doFacebookUiState:(id)sender;
- (IBAction)doFacebookDefaultButton:(id)sender;
- (IBAction)doOkButton:(id)sender;
- (IBAction)doCancelButton:(id)sender;
- (void)awakeFromNib;
- (BOOL)control:(NSControl *)control isValidObject:(id)obj;

@end