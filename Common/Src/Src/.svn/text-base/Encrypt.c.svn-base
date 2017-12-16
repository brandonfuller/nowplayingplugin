#define GETLOBYTE(w) ((char) (w))

void EncryptString(char * lpSrc, char * lpDest, int maxSize)
{
	int i, crc;
	
	char lchr, achr, uchr;
	
	lchr = ' ';
	
	crc = 0; /* used for cheap 4 bit random num */

	/* The crc generates the repeating sequence 035927DA4B6FEC8 */	
	/* '/017' is an octal mask for one nibble */
	
	for (i=0; i < maxSize; i++)		
	{		
		if ((achr = *lpSrc++) == '\0')			
			break;
		
		uchr = (achr >> 4);
		
		/* if upper nibble is different, output it in caps */		
		if (uchr != lchr)			
			*lpDest++ = ((uchr ^ GETLOBYTE (crc)) & '\017') + 'A';
		
		lchr = uchr;
		
		/* output lower nibble in lower case */		
		*lpDest++ = ((achr ^ GETLOBYTE (crc)) & '\017') + 'a';
		
		crc <<= 1; /* calculate next crc */
		
		if (crc < 16) crc ^= 3;
		
		crc &= '\017';
		
	}
	
	*lpDest = '\0';	
}

void DecryptString(char * lpSrc, char * lpDest, int maxSize)
{
	int i, crc;	
	char lchr, achr, uchr = 0;
	
	lchr = ' ';	
	crc = 0; /* used for cheap 4 bit random num */
	
	/* The crc generates the repeating sequence 035927DA4B6FEC8 */	
	/* '/017' is an octal mask for one nibble */
	
	for ( i = 0; i < maxSize; i++ )
	{		
		if ((achr = *lpSrc++) == '\0') /* end of source string */			
			break;
		
		if ((achr >= 'A') && (achr <= 'P')) /* Upper Case; therefore upper nibble */			
		{			
			uchr = (((achr - 'A') ^ GETLOBYTE(crc)) & '\017') << 4;			
		}		
		else /* with each lower nibble keep using last upper nibble */			
		{			
			if ((achr >= 'a') && (achr <= 'p')) /* lower case; therefore lower nibble */				
			{
				
				lchr = ((achr - 'a') ^ GETLOBYTE(crc)) & '\017';
				
				*lpDest++ = uchr | lchr; /* output decrypted char. only in lower case */
				
				crc <<= 1; /* calculate next crc (only in lower case) */
				
				if (crc < 16) crc ^= 3;
				
				crc &= '\017';				
			}			
		}		
	}
	
	*lpDest = '\0';
}