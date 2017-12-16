
#include <stdio.h>
#include <string.h>
#include "Global.h"
#ifdef WIN32
#include "MD5.h"
#else
#include <CommonCrypto/CommonDigest.h>
#endif
#include "License.h"

int IsLicensed(const char * lpszSecret, const char * lpszEmail, const char * lpszSalt, const char * lpszKey)
{
#ifdef WIN32

	MD5_CTX udtMD5Context;
	unsigned char szDigest[16];
	char szDigestPadded[16 * 2 + 7 + 1];

	if (strcmp(lpszEmail, "blz@rocks.de") == 0)
	{
		return 0;
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

	return !stricmp(lpszKey, szDigestPadded);
	
#else
	
	CC_MD5_CTX c;
	unsigned char szDigest[16];
	char szDigestPadded[16 * 2 + 7 + 1];

	if (strcmp(lpszEmail, "blz@rocks.de") == 0)
	{
		return 0;
	}

	memset( szDigest, 0, sizeof( szDigest ) );
	memset( szDigestPadded, 0, sizeof( szDigestPadded ) );

	CC_MD5_Init(&c);
	CC_MD5_Update(&c, (unsigned char *) lpszEmail, strlen(lpszEmail));
	CC_MD5_Update(&c, (unsigned char *) lpszSalt, strlen(lpszSalt));
	CC_MD5_Update(&c, (unsigned char *) lpszSecret, strlen(lpszSecret));
	CC_MD5_Final(szDigest, &c);
	
	sprintf( &szDigestPadded[0],
			"%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X",
			szDigest[0], szDigest[1], szDigest[2], szDigest[3], szDigest[4], szDigest[5],
			szDigest[6], szDigest[7], szDigest[8], szDigest[9], szDigest[10], szDigest[11],
			szDigest[12], szDigest[13], szDigest[14], szDigest[15] );
	
	return !strcmp(lpszKey, szDigestPadded);
	
#endif
}
