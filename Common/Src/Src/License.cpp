
#include <stdio.h>
#include <string.h>
#include "Global.h"
#include "MD5.h"
#include "License.h"

bool IsLicensed(const char * lpszSecret, const char * lpszEmail, const char * lpszSalt, const char * lpszKey)
{
	unsigned char szDigest[16];
	char szDigestPadded[16 * 2 + 7 + 1];
	MD5_CTX udtMD5Context;

	if (stricmp(lpszEmail, "blz@rocks.de") == 0)
	{
		return false;
	}

	memset( szDigest, 0, sizeof( szDigest ) );
	memset( szDigestPadded, 0, sizeof( szDigestPadded ) );
	
	MD5Init( &udtMD5Context );
	MD5Update( &udtMD5Context, (unsigned char *) lpszEmail, strlen( lpszEmail ) );
	MD5Update( &udtMD5Context, (unsigned char *) lpszSalt, strlen( lpszSalt ) );
	MD5Update( &udtMD5Context, (unsigned char *) lpszSecret, strlen( lpszSecret ) );
	MD5Final( szDigest, &udtMD5Context );

	sprintf( &szDigestPadded[0],
			"%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X",
			szDigest[0], szDigest[1], szDigest[2], szDigest[3], szDigest[4], szDigest[5],
			szDigest[6], szDigest[7], szDigest[8], szDigest[9], szDigest[10], szDigest[11],
			szDigest[12], szDigest[13], szDigest[14], szDigest[15] );

#ifdef WIN32
	if ( stricmp( lpszKey, szDigestPadded ) == 0 )
#else
	if ( strcmp( lpszKey, szDigestPadded ) == 0 )
#endif
	{
		return true;
	}
	else
	{
		return false;
	}
}
