
#define PRODUCT_NAME "Now Playing"

#define UPDATE_HOSTNAME "brandon.fuller.name"
#define UPDATE_VERSION_TAG_BEGIN "<su:version>"
#define UPDATE_VERSION_TAG_END "</su:version>"

#define MY_REGISTRY_VALUE_CONFIGURED "Configured"
#define MY_REGISTRY_VALUE_FILE "File"
#define MY_REGISTRY_VALUE_LICENSE_KEY "License Key"
#define MY_REGISTRY_VALUE_EMAIL "E-mail"
#define MY_REGISTRY_VALUE_STYLESHEET "Style Sheet"
#define MY_REGISTRY_VALUE_PROTOCOL "Upload Protocol"
#define MY_REGISTRY_VALUE_FTP_PASSIVE "FTP Passive"
#define MY_REGISTRY_VALUE_HOST "Host"
#define MY_REGISTRY_VALUE_USER "User"
#define MY_REGISTRY_VALUE_PASSWORD "Password"
#define MY_REGISTRY_VALUE_PATH "Path"
#define MY_REGISTRY_VALUE_TRACKBACK_URL "TrackBack URL"
#define MY_REGISTRY_VALUE_TRACKBACK_PASSPHRASE "TrackBack Passphrase"
#define MY_REGISTRY_VALUE_PING_EXTRA_INFO "Ping Extra Info"
#define MY_REGISTRY_VALUE_PLAYLIST_LENGTH "Playlist Length"
#define MY_REGISTRY_VALUE_PUBLISH_STOP "Publish Stop"
#define MY_REGISTRY_VALUE_ARTWORK_EXPORT "Artwork Export"
#define MY_REGISTRY_VALUE_CLEAR_PLAYLIST "Clear Playlist"
#define MY_REGISTRY_VALUE_DELAY "Playlist Buffer Delay"
#define MY_REGISTRY_VALUE_UPDATE_TIMESTAMP "Last Software Update Check"
#define MY_REGISTRY_VALUE_AMAZON_LOCALE "Amazon Locale"
#define MY_REGISTRY_VALUE_AMAZON_ASSOCIATE "Amazon Associate ID"
#define MY_REGISTRY_VALUE_APPLE_ENABLED "Apple Enabled"
#define MY_REGISTRY_VALUE_APPLE_ASSOCIATE "Apple Associate ID"
#define MY_REGISTRY_VALUE_PLAYLIST_CACHE "Playlist Cache"
#define MY_REGISTRY_VALUE_PLAYLIST_CACHE_INDEX "Playlist Cache Index"
#define MY_REGISTRY_VALUE_XML_CDATA "Use CDATA in XML"
#define MY_REGISTRY_VALUE_XML_ENCODING "XML Encoding"
#define MY_REGISTRY_VALUE_AMAZON_ENABLED "Amazon Enabled"
#define MY_REGISTRY_VALUE_AMAZON_USE_ASIN "Amazon Use ASIN"
#define MY_REGISTRY_VALUE_LOGGING "Logging"
#define MY_REGISTRY_VALUE_ARTWORK_UPLOAD "Artwork Upload"
#define MY_REGISTRY_VALUE_ARTWORK_WIDTH "Artwork Width"
#define MY_REGISTRY_VALUE_TWITTER_ENABLED "Twitter Enabled"
#define MY_REGISTRY_VALUE_TWITTER_USERNAME "Twitter Username"
#define MY_REGISTRY_VALUE_TWITTER_PASSWORD "Twitter Password"
#define MY_REGISTRY_VALUE_TWITTER_MESSAGE "Twitter Message"
#define MY_REGISTRY_VALUE_FACEBOOK_ENABLED "Facebook Enabled"
#define MY_REGISTRY_VALUE_FACEBOOK_MESSAGE "Facebook Message"
#define MY_REGISTRY_VALUE_FACEBOOK_SESSION_KEY "Facebook Session Key"
#define MY_REGISTRY_VALUE_FACEBOOK_SECRET "Facebook Secret"
#define MY_REGISTRY_VALUE_EXPORT_TIMESTAMP "Export Timestamp"

#define MY_SECRET_KEY "K2jv98f"

#define SOFTWARE_UPDATE_CHECK_INTERVAL_DAYS 7

#define AMAZON_LOCALE_US "us"
#define AMAZON_LOCALE_UK "uk"
#define AMAZON_LOCALE_JP "jp"
#define AMAZON_LOCALE_DE "de"
#define AMAZON_LOCALE_CA "ca"
#define AMAZON_LOCALE_FR "fr"

#define UPLOAD_PROTOCOL_NONE "(Off)"
#define UPLOAD_PROTOCOL_FTP "FTP"
#define UPLOAD_PROTOCOL_SFTP "SFTP"

#define MY_AMAZON_DEV_TOKEN "D19SL3MIFFPEJU"
#define MY_AMAZON_ACCESS_KEY_ID "03AKJ1J6S0FY8K0WRER2"
#define MY_AMAZON_ASSOCIATE_ID "brandonfuller-20"

#define DEFAULT_APPLE_ASSOCIATE "lHZs7dJ62kA"
#define URL_APPLE_STORE "http://click.linksynergy.com/fs-bin/stat?id=%s&amp;offerid=78941&amp;type=3&amp;subid=0&amp;tmpid=1826&amp;RD_PARM1=http%%253A%%252F%%252Fphobos.apple.com%%252FWebObjects%%252FMZStore.woa%%252Fwa%%252FviewAlbum%%253Fi%%253D%s%%2526id%%253D%s%%2526s%%253D143441%%2526partnerId%%253D30"
#define APPLE_LOOKUP_HOSTNAME "ax.phobos.apple.com.edgesuite.net"

#define XML_ENCODING_UTF_8 "UTF-8"
#define XML_ENCODING_ISO_8859_1 "ISO-8859-1"

#define LOGGING_NONE "(None)"
#define LOGGING_ERROR "Error"
#define LOGGING_INFO "Info"
#define LOGGING_DEBUG "Debug"

#define DEFAULT_PUBLISH_STOP 0
#define DEFAULT_CLEAR_PLAYLIST 0
#define DEFAULT_PLAYLIST_LENGTH 1
#define DEFAULT_SECONDS_DELAY 15
#define DEFAULT_AMAZON_ENABLED 1
#define DEFAULT_AMAZON_USE_ASIN 0
#define DEFAULT_ARTWORK_EXPORT 0
#define DEFAULT_ARTWORK_UPLOAD 0
#define DEFAULT_ARTWORK_WIDTH 160
#define DEFAULT_APPLE_ENABLED 0

#define TRIAL_LIMIT 5

#define TIMEZONE_FORMAT_GMT "%4d-%02d-%02dT%02d:%02d:%02dZ"
#define TIMEZONE_FORMAT_ALL "%4d-%02d-%02dT%02d:%02d:%02d%s%02d:%02d"

#define PROP_UNSUPPORTED -5

#define MEDIA_PLAYER_UNKNOWN 0
#define MEDIA_PLAYER_ITUNES 1
#define MEDIA_PLAYER_WMP 2
#define MEDIA_PLAYER_YAHOO 3
#define MEDIA_PLAYER_WINAMP 4

#define BUF_LEN_BIG 1024

#define TAG_TITLE "title"
#define TAG_ARTIST "artist"
#define TAG_ALBUM "album"
#define TAG_GENRE "genre"
#define TAG_KIND "kind"
#define TAG_TRACK "track"
#define TAG_NUMTRACKS "numTracks"
#define TAG_YEAR "year"
#define TAG_COMMENTS "comments"
#define TAG_TIME "time"
#define TAG_BITRATE "bitrate"
#define TAG_RATING "rating"
#define TAG_DISC "disc"
#define TAG_NUMDISCS "numDiscs"
#define TAG_PLAYCOUNT "playCount"
#define TAG_COMPILATION "compilation"
#define TAG_URLAMAZON "urlAmazon"
#define TAG_URLAPPLE "urlApple"
#define TAG_IMAGE "image"
#define TAG_IMAGESMALL "imageSmall"
#define TAG_IMAGELARGE "imageLarge"
#define TAG_COMPOSER "composer"
#define TAG_GROUPING "grouping"
#define TAG_URLSOURCE "urlSource"
#define TAG_FILE "file"
#define TAG_ARTWORKID "artworkID"

#define TWITTER_HOSTNAME "twitter.com"
#define TWITTER_PATH "/statuses/update.xml"
#define TWITTER_POST_VALUE "status"
#define TWITTER_MESSAGE_DEFAULT "Listening to - <artist> ~~ <title> [<year>, <genre>]. Posted by Brandon Fuller's Now Playing!"

#define TWITTER_TAG_TITLE "<title>"
#define TWITTER_TAG_ARTIST "<artist>"
#define TWITTER_TAG_ALBUM "<album>"
#define TWITTER_TAG_GENRE "<genre>"
#define TWITTER_TAG_KIND "<kind>"
#define TWITTER_TAG_TRACK "<track>"
#define TWITTER_TAG_NUMTRACKS "<numTracks>"
#define TWITTER_TAG_YEAR "<year>"
#define TWITTER_TAG_COMMENTS "<comments>"
#define TWITTER_TAG_TIME "<time>"
#define TWITTER_TAG_BITRATE "<bitrate>"
#define TWITTER_TAG_RATING "<rating>"
#define TWITTER_TAG_DISC "<disc>"
#define TWITTER_TAG_NUMDISCS "<numDiscs>"
#define TWITTER_TAG_PLAYCOUNT "<playCount>"
#define TWITTER_TAG_COMPOSER "<composer>"
#define TWITTER_TAG_GROUPING "<grouping>"
#define TWITTER_TAG_FILE "<file>"
#define TWITTER_TAG_IMAGE "<image>"
#define TWITTER_TAG_IMAGESMALL "<imageSmall>"
#define TWITTER_TAG_IMAGELARGE "<imageLarge>"
#define TWITTER_TAG_URLAMAZON "<urlAmazon>"
#define TWITTER_TAG_TIMESTAMP "<timestamp>"
#define TWITTER_TAG_AMAZONMATCH_START "<hasAmazon>"
#define TWITTER_TAG_AMAZONMATCH_STOP "</hasAmazon>"

#define FACEBOOK_HOSTNAME "api.facebook.com"
#define FACEBOOK_PATH "/restserver.php"
#define FACEBOOK_API_KEY "67deda865cb0fb74f838533ea31327d6"
#define FACEBOOK_API_SECRET "ba11bf717dd76a30d27aeb948a6a18f6"
#define FACEBOOK_MESSAGE_DEFAULT "<center><hasAmazon><a href=\"<urlAmazon>\"><img alt=\"<artist>\" src=\"<image>\"/></a><br/></hasAmazon><b><artist></b><br/><i><title></i><br/><hasAmazon><a href=\"<urlAmazon>\"></hasAmazon><album><hasAmazon></a></hasAmazon></center><br/><table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\"><tr><td><span style=\"color:#999999;font-size:7pt;\">Last updated<br/><fb:time t=\"<timestamp>\"/></span></td><td style=\"text-align: right\"><a href=\"http://www.facebook.com/apps/application.php?id=2392944680\"><span style=\"font-size:7pt\">Get this app!</span></a></td></tr></table>"
#define FACEBOOK_LOGIN_MESSAGE "A web browser will now be opened so you can login to Facebook. After login, return to this screen and press the Authorize button."