/**\
|**|	QDMP.c
\**/

/**\
|**|	includes
\**/
#include<Carbon/Carbon.h>

#include "QDMP.h"

#if 1
#	define GET_PIXROWBYTES(p) GetPixRowBytes(p)
#	define BLOCK_COPY(s,d,c) MPBlockCopy(s,d,c)
#	define GET_PIXBASEADDR(p) GetPixBaseAddr(p)
#else
#	define GET_PIXROWBYTES(p) ((*p)->rowBytes & 0x3FFF)
#	define BLOCK_COPY(s,d,c) MP_BlockCopy(s,d,c)
#	define GET_PIXBASEADDR(p) (((*p)->rowBytes & 0x8000) ? (*(UInt32*)(*p)->baseAddr) : (*p)->baseAddr)
#endif

/**\
|**|	local (static) globals
\**/

static Boolean gMP_Flag = false;
#if __VEC__
static Boolean gAltiVec_Flag = false;
#endif __VEC__

/**\
|**|	local (static) functions
\**/

static inline UInt8 RGBColorToColor8(const PixMapHandle pPixMapHdl,const RGBColor* pColor)
{
	UInt8 result = -1;

	if (pPixMapHdl && *pPixMapHdl && pColor)
	{
		CTabHandle tCTabHdl = (*pPixMapHdl)->pmTable;
		if (tCTabHdl && *tCTabHdl)
		{
			SInt16 index,count = (*tCTabHdl)->ctSize + 1;	// # of entries
			UInt32 minSquared = -1;
			ColorSpec* tColorSpecPtr = (*tCTabHdl)->ctTable;

			for (index = 0;index < count;index++)
			{
				RGBColor tRGBColor = *pColor;
				UInt32 squared;

				tRGBColor.red -= tColorSpecPtr[index].rgb.red;
				tRGBColor.green -= tColorSpecPtr[index].rgb.green;
				tRGBColor.blue -= tColorSpecPtr[index].rgb.blue;

				squared =	(tRGBColor.red * tRGBColor.red) +
							(tRGBColor.green * tRGBColor.green) +
							(tRGBColor.blue * tRGBColor.blue);

				if (squared < minSquared)
				{
					result = index; // tColorSpecPtr[index].value;
					minSquared = squared;
				}
			}
		}
	}
	return result;
}

static inline UInt16 RGBColorToColor16(const RGBColor* pColor)
{
	return 	((pColor->red >> 1) & 0x7C00) |
			((pColor->green >> 6) & 0x03E0) |
			((pColor->blue >> 11) & 0x001F);
}

static inline UInt32 RGBColorToColor32(const RGBColor* pColor)
{
	return 	((pColor->red << 8) & 0x00FF0000) |
			((pColor->green) & 0x0000FF00) |
			((pColor->blue >> 8) & 0x000000FF);
}

static inline RGBColor Color16ToRGBColor(const UInt32 pColor16)
{
	RGBColor result;

	result.red = (pColor16 >> 10) & 0x1F;
	result.red |= result.red << 11;

	result.green = (pColor16 >> 5) & 0x1F;
	result.green |= result.green << 11;

	result.blue = pColor16 & 0x1F;
	result.blue |= result.blue << 11;

	return result;
}

static inline RGBColor Color32ToRGBColor(const UInt32 pColor32)
{
	RGBColor result;

	result.red = (pColor32 >> 16) & 0xFF;
	result.red |= result.red << 8;

	result.green = (pColor32 >> 8) & 0xFF;
	result.green |= result.green << 8;

	result.blue = pColor32 & 0xFF;
	result.blue |= result.blue << 8;

	return result;
}

static inline RGBColor FindColorByIndex(const PixMapHandle pPixMapHdl,const SInt16 pIndex)
{
	RGBColor result = {0,0,0};	// black

	if (pPixMapHdl && *pPixMapHdl)
	{
		CTabHandle tCTabHdl = (*pPixMapHdl)->pmTable;
		if (tCTabHdl && *tCTabHdl)
		{
			SInt16 index,count = (*tCTabHdl)->ctSize + 1;	// # of entries
			ColorSpec* tColorSpecPtr = (*tCTabHdl)->ctTable;

#ifdef HellFrozenOver
			for (index = pIndex;(index >= 0) && (index < count);index++)
			{
				if (pIndex == tColorSpecPtr[index].value)
					return tColorSpecPtr[index].rgb;
			}

			for (index = pIndex;(index >= 0) && (index < count);index--)
			{
				if (pIndex == tColorSpecPtr[index].value)
					return tColorSpecPtr[index].rgb;
			}
#else
			index = pIndex % count;
			return tColorSpecPtr[index].rgb;
#endif HellFrozenOver
		}
	}
	return result;
}

#if __VEC__
static inline Boolean MP_AltiVecBlockCopy(LogicalAddress pSrc,LogicalAddress pDst,const ByteCount pSize)
{
	vector float vAlignedData;			// hold aligned versions of vectors
	UInt8*	srcPtr = pSrc;
	UInt8*	dstPtr = pDst;
	SInt32 count = pSize;
	UInt32 index = 0;

	if (!gAltiVec_Flag)
		return false;	// can't copy

	{
		UInt32 srcAlign16 = 0x0F & -(UInt32)srcPtr;
		UInt32 dstAlign16 = 0x0F & -(UInt32)dstPtr;

		if (srcAlign16 || dstAlign16)
			return false;	// (source or dest isn't 16 byte aligned)
	}

	vec_dstt((float*) srcPtr,0,0);
	vec_dststt((float*) dstPtr,0,0);

	while (count >= 16)
	{
		vAlignedData = vec_ld(index, (float *) srcPtr);	// loads vAlignedData from srcPtr
		vec_st(vAlignedData, index, (float *) dstPtr);	// stores vAlignedData to dstPtr
		index += 16;
		count -= 16;
	}

	srcPtr += index;
	dstPtr += index;

	if (count >= 8)
	{
		*(UInt64*)dstPtr++ = *(UInt64*)srcPtr++;
		count -= 8;
	}

	if (count >= 4)
	{
		*(UInt32*)dstPtr++ = *(UInt32*)srcPtr++;
		count -= 4;
	}

	if (count >= 2)
	{
		*(UInt16*)dstPtr++ = *(UInt16*)srcPtr++;
		count -= 2;
	}

	if (count >= 1)
	{
		*dstPtr++ = *srcPtr++;
		count--;
	}

	return true;
}
#endif __VEC__

static inline void MP_BlockCopy(LogicalAddress pSrc,LogicalAddress pDst,const ByteCount pSize)
{
	UInt8*	srcPtr = pSrc;
	UInt8*	dstPtr = pDst;
	SInt32	deltaX = pSize;

	if (!deltaX)
		return;	// nothing to copy

	if (deltaX >= 16)
	{
		UInt8	align16 = 0x0F & -(UInt32) srcPtr;

		if (dstPtr > srcPtr)	// align to hightest address
			align16 = 0x0F & -(UInt32) dstPtr;

		if (align16 & 0x01)	// byte align
		{
			*dstPtr++ = *srcPtr++;
			deltaX--;
		}

		if (align16 & 0x02)	// word align
		{
			*(UInt16*)dstPtr++ = *(UInt16*)srcPtr++;
			deltaX -= 2;
		}

		if (align16 & 0x04)	// long align
		{
			*(UInt32*)dstPtr++ = *(UInt32*)srcPtr++;
			deltaX -= 4;
		}

		if (align16 & 0x08)	// long long align
		{
			*(UInt64*)dstPtr++ = *(UInt64*)srcPtr++;
			deltaX -= 8;
		}
	}

#if __VEC__
	if (!gAltiVec_Flag || !MP_AltiVecBlockCopy(srcPtr,dstPtr,deltaX))
#endif __VEC__
	{
		while (deltaX >= 64)
		{
			((UInt64*)dstPtr)[0] = ((UInt64*)srcPtr)[0];
			((UInt64*)dstPtr)[1] = ((UInt64*)srcPtr)[1];
			((UInt64*)dstPtr)[2] = ((UInt64*)srcPtr)[2];
			((UInt64*)dstPtr)[3] = ((UInt64*)srcPtr)[3];
			((UInt64*)dstPtr)[4] = ((UInt64*)srcPtr)[4];
			((UInt64*)dstPtr)[5] = ((UInt64*)srcPtr)[5];
			((UInt64*)dstPtr)[6] = ((UInt64*)srcPtr)[6];
			((UInt64*)dstPtr)[7] = ((UInt64*)srcPtr)[7];

			dstPtr += 64;
			srcPtr += 64;
			deltaX -= 64;
		}

		while (deltaX >= 32)
		{
			((UInt64*)dstPtr)[0] = ((UInt64*)srcPtr)[0];
			((UInt64*)dstPtr)[1] = ((UInt64*)srcPtr)[1];
			((UInt64*)dstPtr)[2] = ((UInt64*)srcPtr)[2];
			((UInt64*)dstPtr)[3] = ((UInt64*)srcPtr)[3];

			dstPtr += 32;
			srcPtr += 32;
			deltaX -= 32;
		}

		while (deltaX >= 16)
		{
			((UInt64*)dstPtr)[0] = ((UInt64*)srcPtr)[0];
			((UInt64*)dstPtr)[1] = ((UInt64*)srcPtr)[1];

			dstPtr += 16;
			srcPtr += 16;
			deltaX -= 16;
		}
	}

	while (deltaX >= 8)
	{
		*(UInt64*)dstPtr++ = *(UInt64*)srcPtr++;
		deltaX -= 8;
	}

	while (deltaX >= 4)
	{
		*(UInt32*)dstPtr++ = *(UInt32*)srcPtr++;
		deltaX -= 4;
	}

	while (deltaX >= 2)
	{
		*(UInt16*)dstPtr++ = *(UInt16*)srcPtr++;
		deltaX -= 2;
	}

	while (deltaX >= 1)
	{
		*dstPtr++ = *srcPtr++;
		deltaX--;
	}
}

OSStatus QDMP_Init(void)
{
	OSStatus err = noErr;

	gMP_Flag = MPLibraryIsLoaded();

#if __VEC__
	gAltiVec_Flag = false;	// assume no AltiVec
//	if (!gMP_Flag) // || !MPTaskIsPreemptive(MPCurrentTaskID()))
	{
		SInt32 response;

		err = Gestalt(gestaltNativeCPUtype,&response);
		if (err == noErr)
		{
			if (response == 0x010C)	// G4
				gAltiVec_Flag = true;
		}
	}
#endif __VEC__
	return err;
}

void QDMP_Set_Rect(Rect *pRectPtr,const SInt16 pLeft,const SInt16 pTop,const SInt16 pRight,const SInt16 pBottom)
{
	pRectPtr->left = pLeft;
	pRectPtr->top = pTop;
	pRectPtr->right = pRight;
	pRectPtr->bottom = pBottom;
}

void QDMP_Offset_Rect(Rect *pRectPtr, const SInt16 pDeltaH, const SInt16 pDeltaV)
{
	pRectPtr->top += pDeltaV;
	pRectPtr->bottom += pDeltaV;
	pRectPtr->right += pDeltaH;
	pRectPtr->left += pDeltaH;
}

void QDMP_Inset_Rect(Rect *pRectPtr, const SInt16 pDeltaH, const SInt16 pDeltaV)
{
	pRectPtr->top += pDeltaV;
	pRectPtr->bottom -= pDeltaV;
	pRectPtr->left += pDeltaH;
	pRectPtr->right -= pDeltaH;
}

Boolean QDMP_Intersect_Rects(const Rect* pRectA,const Rect* pRectB,Rect* pRectC)
{
	Rect tRect = *pRectA;

	if (tRect.top < pRectB->top)
		tRect.top = pRectB->top;

	if (tRect.bottom > pRectB->bottom)
		tRect.bottom = pRectB->bottom;

	if (tRect.left < pRectB->left)
		tRect.left = pRectB->left;

	if (tRect.right > pRectB->right)
		tRect.right = pRectB->right;

	if (pRectC != nil)
		*pRectC = tRect;

	if (tRect.top >= tRect.bottom)
		return false;
	else if (tRect.left >= tRect.right)
		return false;

	return true;
}

Point QDMP_Set_Point(Point* pPoint,const SInt16 pH,const SInt16 pV)
{
	Point tPoint;
	tPoint.h = pH;
	tPoint.v = pV;

	if (pPoint != nil)
		*pPoint = tPoint;

	return tPoint;
}

Boolean QDMP_PointInRect(const Point pPoint,const Rect* pRect)
{
	if (nil == pRect)
		return false;
	else if (pPoint.h < pRect->left)
		return false;
	else if (pPoint.v < pRect->top)
		return false;
	else if (pPoint.h >= pRect->right)
		return false;
	else if (pPoint.v >= pRect->bottom)
		return false;

	return true;
}

Point QDMP_MidPoint(const Point pPointA,const Point pPointB)
{
	Point tPoint;

	tPoint.h = (pPointA.h + pPointB.h) >> 1;
	tPoint.v = (pPointA.v + pPointB.v) >> 1;

	return tPoint;
}

Point QDMP_MidPointRect(const Rect* pRect)
{
	Point tPointA,tPointB;

	tPointA.h = pRect->left; tPointA.v = pRect->top;
	tPointB.h = pRect->right; tPointB.v = pRect->bottom;
	return QDMP_MidPoint(tPointA,tPointB);
}

RGBColor QDMP_Get_Pixel(PixMapHandle pPixMapHdl,const SInt16 pH,const SInt16 pV)
{
	RGBColor result = {0,0,0};

	if (pPixMapHdl && *pPixMapHdl)
	{
		switch ((*pPixMapHdl)->pixelSize)
		{
		case	8:
			result = FindColorByIndex(pPixMapHdl,QDMP_Get_Pixel8(pPixMapHdl,pH,pV));
			break;
		case	16:
			result = Color16ToRGBColor(QDMP_Get_Pixel16(pPixMapHdl,pH,pV));
			break;
		case	32:
			result = Color32ToRGBColor(QDMP_Get_Pixel32(pPixMapHdl,pH,pV));
			break;
		}
	}
	return result;
}

UInt8 QDMP_Get_Pixel8(PixMapHandle pPixMapHdl,
			const SInt16 pH,const SInt16 pV)
{
	UInt8 result = 0;

	if (pPixMapHdl && *pPixMapHdl)
	{
		if ((*pPixMapHdl)->pixelSize == 8)
		{
			Rect tRect = (*pPixMapHdl)->bounds;

			if ((pH >= tRect.left) && (pH < tRect.right) &&
				(pV >= tRect.top) && (pV < tRect.bottom))
			{
				UInt8 *baseAddr = (UInt8*) GET_PIXBASEADDR(pPixMapHdl);
				UInt32 rowBytes = GET_PIXROWBYTES(pPixMapHdl);

				result = *(UInt8*) (baseAddr + 
					(rowBytes * (pV - tRect.top) + (pH - tRect.left)));
			}
		}
	}
	return result;
}

UInt16 QDMP_Get_Pixel16(PixMapHandle pPixMapHdl,
			const SInt16 pH,const SInt16 pV)
{
	UInt16 result = 0;

	if (pPixMapHdl && *pPixMapHdl)
	{
		if ((*pPixMapHdl)->pixelSize == 16)
		{
			Rect tRect = (*pPixMapHdl)->bounds;

			if ((pH >= tRect.left) && (pH < tRect.right) && 
				(pV >= tRect.top) && (pV < tRect.bottom))
			{
				UInt8 *baseAddr = (UInt8*) GET_PIXBASEADDR(pPixMapHdl);
				UInt32 rowBytes = GET_PIXROWBYTES(pPixMapHdl);

				result = *(UInt16*) (baseAddr + 
					(rowBytes * (pV - tRect.top) + ((pH - tRect.left) << 1)));
			}
		}
	}
	return result;
}

UInt32 QDMP_Get_Pixel32(PixMapHandle pPixMapHdl,
			const SInt16 pH,const SInt16 pV)
{
	UInt32 result = 0;

	if (pPixMapHdl && *pPixMapHdl)
	{
		if ((*pPixMapHdl)->pixelSize == 32)
		{
			Rect tRect = (*pPixMapHdl)->bounds;

			if ((pH >= tRect.left) && (pH < tRect.right) && 
				(pV >= tRect.top) && (pV < tRect.bottom))
			{
				UInt8 *baseAddr = (UInt8*) GET_PIXBASEADDR(pPixMapHdl);
				UInt32 rowBytes = GET_PIXROWBYTES(pPixMapHdl);

				result = *(UInt32*) (baseAddr + 
					(rowBytes * (pV - tRect.top) + ((pH - tRect.left) << 2)));
			}
		}
	}
	return result;
}

RGBColor QDMP_Get_PixelP(PixMapHandle pPixMapHdl,const Point pPoint)
{
	return QDMP_Get_Pixel(pPixMapHdl,pPoint.h,pPoint.v);
}

UInt8 QDMP_Get_PixelP8(PixMapHandle pPixMapHdl,const Point pPoint)
{
	return QDMP_Get_Pixel8(pPixMapHdl,pPoint.h,pPoint.v);
}

UInt16 QDMP_Get_PixelP16(PixMapHandle pPixMapHdl,const Point pPoint)
{
	return QDMP_Get_Pixel16(pPixMapHdl,pPoint.h,pPoint.v);
}

UInt32 QDMP_Get_PixelP32(PixMapHandle pPixMapHdl,const Point pPoint)
{
	return QDMP_Get_Pixel32(pPixMapHdl,pPoint.h,pPoint.v);
}

void QDMP_Set_Pixel(PixMapHandle pPixMapHdl,const SInt16 pH,const SInt16 pV,const RGBColor* pColor)
{
	if (pPixMapHdl && *pPixMapHdl)
	{
		switch ((*pPixMapHdl)->pixelSize)
		{
		case	8:
			{
				UInt8 color8 = RGBColorToColor8(pPixMapHdl,pColor);
				QDMP_Set_Pixel8(pPixMapHdl,pH,pV,color8);
			}
			break;
		case	16:
			{
				UInt16 color16 = RGBColorToColor16(pColor);
				QDMP_Set_Pixel16(pPixMapHdl,pH,pV,color16);
			}
			break;
		case	32:
			{
				UInt32 color32 = RGBColorToColor32(pColor);

				QDMP_Set_Pixel32(pPixMapHdl,pH,pV,color32);
			}
			break;
		}
	}
}

void QDMP_Set_Pixel8(PixMapHandle pPixMapHdl,const SInt16 pH,const SInt16 pV,const UInt8 pColor)
{
	if (pPixMapHdl && *pPixMapHdl)
	{
		if ((*pPixMapHdl)->pixelSize == 8)
		{
			Rect tRect = (*pPixMapHdl)->bounds;

			if ((pH >= tRect.left) && (pH < tRect.right) && (pV >= tRect.top) && (pV < tRect.bottom))
			{
				UInt8 *baseAddr = (UInt8*) GET_PIXBASEADDR(pPixMapHdl);
				UInt32 rowBytes = GET_PIXROWBYTES(pPixMapHdl);

				*(UInt8*) (baseAddr + (rowBytes * (pV - tRect.top) + (pH - tRect.left))) = pColor;
			}
		}
	}
}

void QDMP_Set_Pixel16(PixMapHandle pPixMapHdl,const SInt16 pH,const SInt16 pV,const UInt16 pColor)
{
	if (pPixMapHdl && *pPixMapHdl)
	{
		if ((*pPixMapHdl)->pixelSize == 16)
		{
			Rect tRect = (*pPixMapHdl)->bounds;

			if ((pH >= tRect.left) && (pH < tRect.right) && (pV >= tRect.top) && (pV < tRect.bottom))
			{
				UInt8 *baseAddr = (UInt8*) GET_PIXBASEADDR(pPixMapHdl);
				UInt32 rowBytes = GET_PIXROWBYTES(pPixMapHdl);

				*(UInt16*) (baseAddr + (rowBytes * (pV - tRect.top) + ((pH - tRect.left) << 1))) = pColor;
			}
		}
	}
}

void QDMP_Set_Pixel32(PixMapHandle pPixMapHdl,const SInt16 pH,const SInt16 pV,const UInt32 pColor)
{
	if (pPixMapHdl && *pPixMapHdl)
	{
		if ((*pPixMapHdl)->pixelSize == 32)
		{
			Rect tRect = (*pPixMapHdl)->bounds;

			if ((pH >= tRect.left) && (pH < tRect.right) && (pV >= tRect.top) && (pV < tRect.bottom))
			{
				UInt8 *baseAddr = (UInt8*) GET_PIXBASEADDR(pPixMapHdl);
				UInt32 rowBytes = GET_PIXROWBYTES(pPixMapHdl);

				*(UInt32*) (baseAddr + (rowBytes * (pV - tRect.top) + ((pH - tRect.left) << 2))) = pColor;
			}
		}
	}
}

void QDMP_Set_PixelP(PixMapHandle pPixMapHdl,const Point pPoint,const RGBColor* pColor)
{
	QDMP_Set_Pixel(pPixMapHdl,pPoint.h,pPoint.v,pColor);
}

void QDMP_Set_PixelP8(PixMapHandle pPixMapHdl,const Point pPoint,const UInt8 pColor)
{
	QDMP_Set_Pixel8(pPixMapHdl,pPoint.h,pPoint.v,pColor);
}

void QDMP_Set_PixelP16(PixMapHandle pPixMapHdl,const Point pPoint,const UInt16 pColor)
{
	QDMP_Set_Pixel16(pPixMapHdl,pPoint.h,pPoint.v,pColor);
}

void QDMP_Set_PixelP32(PixMapHandle pPixMapHdl,const Point pPoint,const UInt32 pColor)
{
	QDMP_Set_Pixel32(pPixMapHdl,pPoint.h,pPoint.v,pColor);
}

void QDMP_Copy_Rect(PixMapHandle pSrcPixMapHdl,PixMapHandle pDstPixMapHdl,const Rect* pSrcRect,const Rect* pDstRect)
{
	if ((*pSrcPixMapHdl)->pixelSize == (*pDstPixMapHdl)->pixelSize)
	{
		switch ((*pSrcPixMapHdl)->pixelSize)
		{
		case	8:
			QDMP_Copy_Rect8(pSrcPixMapHdl,pDstPixMapHdl,pSrcRect,pDstRect);
			break;
		case	16:
			QDMP_Copy_Rect16(pSrcPixMapHdl,pDstPixMapHdl,pSrcRect,pDstRect);
			break;
		case	32:
			QDMP_Copy_Rect32(pSrcPixMapHdl,pDstPixMapHdl,pSrcRect,pDstRect);
			break;
		}
	}
}

void QDMP_Copy_Rect8(PixMapHandle pSrcPixMapHdl,PixMapHandle pDstPixMapHdl,const Rect* pSrcRect,const Rect* pDstRect)
{
	if (pSrcPixMapHdl && pDstPixMapHdl && pSrcRect && pDstRect)
	{
#if 0
		SInt16 srcY1 = pSrcRect->top;
		SInt16 srcY2 = pSrcRect->bottom;
		SInt16 dstY1 = pDstRect->top;
		SInt16 dstY2 = pDstRect->bottom;

		while ((srcY1 <= srcY2) && (dstY1 <= dstY2))
		{
			QDMP_Copy_Scan8(pSrcPixMapHdl,pDstPixMapHdl,srcY1,pSrcRect->left,pSrcRect->right,dstY1,pDstRect->left,pDstRect->right);
			srcY1++; dstY1++;
		}
#else
		Rect srcBounds = (*pSrcPixMapHdl)->bounds;
		Rect dstBounds = (*pDstPixMapHdl)->bounds;
		Rect srcRect = *pSrcRect;
		Rect dstRect = *pDstRect;

		// clip copy rects to pixmap rects
		if (QDMP_Intersect_Rects(&srcRect,&srcBounds,&srcRect) &&
			QDMP_Intersect_Rects(&dstRect,&dstBounds,&dstRect))
		{
			UInt8 *srcBaseAddr = (UInt8*) GET_PIXBASEADDR(pSrcPixMapHdl),*srcPixelPtr;
			UInt8 *dstBaseAddr = (UInt8*) GET_PIXBASEADDR(pDstPixMapHdl),*dstPixelPtr;

			UInt32 srcRowBytes = GET_PIXROWBYTES(pSrcPixMapHdl);
			UInt32 dstRowBytes = GET_PIXROWBYTES(pDstPixMapHdl);

			SInt16 srcY1 = srcRect.top;
			SInt16 srcY2 = srcRect.bottom;

			SInt16 dstY1 = dstRect.top;
			SInt16 dstY2 = dstRect.bottom;

			SInt16 srcX = srcRect.left,srcDx;
			SInt16 dstX = dstRect.left,dstDx;

			srcDx = srcRect.right - srcX;
			dstDx = dstRect.right - dstX;

			if (srcDx != dstDx)	// if these don't match...
				return;			// (we don't do any scaling here!)

			(Ptr) srcPixelPtr = (Ptr) srcBaseAddr + 
				(srcRowBytes * (srcY1 - srcBounds.top)) + 
				(srcX - srcBounds.left);
			(Ptr) dstPixelPtr = (Ptr) dstBaseAddr + 
				(dstRowBytes * (dstY1 - dstBounds.top)) + 
				(dstX - dstBounds.left);

			while ((srcY1 <= srcY2) && (dstY1 <= dstY2))
			{
				BLOCK_COPY(srcPixelPtr,dstPixelPtr,srcDx);

				(Ptr) srcPixelPtr += srcRowBytes;
				(Ptr) dstPixelPtr += dstRowBytes;

				srcY1++;
				dstY1++;
			}
		}
	#endif
	}
}

void QDMP_Copy_Rect16(PixMapHandle pSrcPixMapHdl,PixMapHandle pDstPixMapHdl,const Rect* pSrcRect,const Rect* pDstRect)
{
	if (pSrcPixMapHdl && pDstPixMapHdl && pSrcRect && pDstRect)
	{
#if 0
		SInt16 srcY1 = pSrcRect->top;
		SInt16 srcY2 = pSrcRect->bottom;
		SInt16 dstY1 = pDstRect->top;
		SInt16 dstY2 = pDstRect->bottom;

		while ((srcY1 <= srcY2) && (dstY1 <= dstY2))
		{
			QDMP_Copy_Scan16(pSrcPixMapHdl,pDstPixMapHdl,srcY1,pSrcRect->left,pSrcRect->right,dstY1,pDstRect->left,pDstRect->right);
			srcY1++; dstY1++;
		}
#else
		Rect srcBounds = (*pSrcPixMapHdl)->bounds;
		Rect dstBounds = (*pDstPixMapHdl)->bounds;
		Rect srcRect = *pSrcRect;
		Rect dstRect = *pDstRect;

		// clip copy rects to pixmap rects
		if (QDMP_Intersect_Rects(&srcRect,&srcBounds,&srcRect) &&
			QDMP_Intersect_Rects(&dstRect,&dstBounds,&dstRect))
		{
			UInt16 *srcBaseAddr = (UInt16*) GET_PIXBASEADDR(pSrcPixMapHdl),*srcPixelPtr;
			UInt16 *dstBaseAddr = (UInt16*) GET_PIXBASEADDR(pDstPixMapHdl),*dstPixelPtr;

			UInt32 srcRowBytes = GET_PIXROWBYTES(pSrcPixMapHdl);
			UInt32 dstRowBytes = GET_PIXROWBYTES(pDstPixMapHdl);

			SInt16 srcY1 = srcRect.top;
			SInt16 srcY2 = srcRect.bottom;

			SInt16 dstY1 = dstRect.top;
			SInt16 dstY2 = dstRect.bottom;

			SInt16 srcX = srcRect.left,srcDx;
			SInt16 dstX = dstRect.left,dstDx;

			srcDx = srcRect.right - srcX;
			dstDx = dstRect.right - dstX;

			if (srcDx != dstDx)	// if these don't match...
				return;			// (we don't do any scaling here!)

			(Ptr) srcPixelPtr = (Ptr) srcBaseAddr + 
				(srcRowBytes * (srcY1 - srcBounds.top)) + 
				((srcX - srcBounds.left) << 1);
			(Ptr) dstPixelPtr = (Ptr) dstBaseAddr + 
				(dstRowBytes * (dstY1 - dstBounds.top)) + 
				((dstX - dstBounds.left) << 1);

			srcDx <<= 1;	// # pixels (words) -> # bytes
			while ((srcY1 <= srcY2) && (dstY1 <= dstY2))
			{
				BLOCK_COPY(srcPixelPtr,dstPixelPtr,srcDx);

				(Ptr) srcPixelPtr += srcRowBytes;
				(Ptr) dstPixelPtr += dstRowBytes;

				srcY1++;
				dstY1++;
			}
		}
#endif
	}
}	// QDMP_Copy_Rect16

void QDMP_Copy_Rect32(PixMapHandle pSrcPixMapHdl,PixMapHandle pDstPixMapHdl,const Rect* pSrcRect,const Rect* pDstRect)
{
	if (pSrcPixMapHdl && pDstPixMapHdl && pSrcRect && pDstRect)
	{
#if 0
		SInt16 srcY1 = pSrcRect->top;
		SInt16 srcY2 = pSrcRect->bottom;
		SInt16 dstY1 = pDstRect->top;
		SInt16 dstY2 = pDstRect->bottom;

		while ((srcY1 <= srcY2) && (dstY1 <= dstY2))
		{
			QDMP_Copy_Scan32(pSrcPixMapHdl,pDstPixMapHdl,srcY1,pSrcRect->left,pSrcRect->right,dstY1,pDstRect->left,pDstRect->right);
			srcY1++; dstY1++;
		}
#else
		Rect srcBounds = (*pSrcPixMapHdl)->bounds;
		Rect dstBounds = (*pDstPixMapHdl)->bounds;
		Rect srcRect = *pSrcRect;
		Rect dstRect = *pDstRect;

		// clip copy rects to pixmap rects
		if (QDMP_Intersect_Rects(&srcRect,&srcBounds,&srcRect) &&
			QDMP_Intersect_Rects(&dstRect,&dstBounds,&dstRect))
		{
			UInt32 *srcBaseAddr = (UInt32*) GET_PIXBASEADDR(pSrcPixMapHdl),*srcPixelPtr;
			UInt32 *dstBaseAddr = (UInt32*) GET_PIXBASEADDR(pDstPixMapHdl),*dstPixelPtr;

			UInt32 srcRowBytes = GET_PIXROWBYTES(pSrcPixMapHdl);
			UInt32 dstRowBytes = GET_PIXROWBYTES(pDstPixMapHdl);

			SInt16 srcY1 = srcRect.top;
			SInt16 srcY2 = srcRect.bottom;

			SInt16 dstY1 = dstRect.top;
			SInt16 dstY2 = dstRect.bottom;

			SInt16 srcX = srcRect.left,srcDx;
			SInt16 dstX = dstRect.left,dstDx;

			srcDx = srcRect.right - srcX;
			dstDx = dstRect.right - dstX;

			if (srcDx != dstDx)	// if these don't match...
				return;			// (we don't do any scaling here!)

			(Ptr) srcPixelPtr = (Ptr) srcBaseAddr + 
				(srcRowBytes * (srcY1 - srcBounds.top)) + 
				((srcX - srcBounds.left) << 2);
			(Ptr) dstPixelPtr = (Ptr) dstBaseAddr + 
				(dstRowBytes * (dstY1 - dstBounds.top)) + 
				((dstX - dstBounds.left) << 2);

			srcDx <<= 2;	// # pixels (longs) -> # bytes
			while ((srcY1 <= srcY2) && (dstY1 <= dstY2))
			{
				BLOCK_COPY(srcPixelPtr,dstPixelPtr,srcDx);

				(Ptr) srcPixelPtr += srcRowBytes;
				(Ptr) dstPixelPtr += dstRowBytes;

				srcY1++;
				dstY1++;
			}
		}
#endif
	}
}	// QDMP_Copy_Rect32

void QDMP_Copy_Rect32A(PixMapHandle pSrcPixMapHdl,PixMapHandle pDstPixMapHdl,const Rect* pSrcRect,const Rect* pDstRect)
{
	if (pSrcPixMapHdl && pDstPixMapHdl && pSrcRect &&pDstRect)
	{
#if 0
		SInt16 srcY1 = pSrcRect->top;
		SInt16 srcY2 = pSrcRect->bottom;
		SInt16 dstY1 = pDstRect->top;
		SInt16 dstY2 = pDstRect->bottom;

		while ((srcY1 <= srcY2) && (dstY1 <= dstY2))
		{
			QDMP_Copy_Scan32A(pSrcPixMapHdl,pDstPixMapHdl,srcY1,pSrcRect->left,pSrcRect->right,dstY1,pDstRect->left,pDstRect->right);
			srcY1++; dstY1++;
		}
#else
		Rect srcRect = (*pSrcPixMapHdl)->bounds;
		Rect dstRect = (*pDstPixMapHdl)->bounds;

		UInt8 *srcBaseAddr = (UInt8*) GET_PIXBASEADDR(pSrcPixMapHdl),*srcPixelPtr;
		UInt8 *dstBaseAddr = (UInt8*) GET_PIXBASEADDR(pDstPixMapHdl),*dstPixelPtr;

		UInt32 srcRowBytes = GET_PIXROWBYTES(pSrcPixMapHdl);
		UInt32 dstRowBytes = GET_PIXROWBYTES(pDstPixMapHdl);

		SInt16 srcY1 = pSrcRect->top;
		SInt16 srcY2 = pSrcRect->bottom;

		SInt16 dstY1 = pDstRect->top;
		SInt16 dstY2 = pDstRect->bottom;

		SInt16 srcX = pSrcRect->left,srcDx;
		SInt16 dstX = pDstRect->left,dstDx;

		// clip to top
		if (srcY1 < srcRect.top)
		{
			dstY1 += srcRect.top - srcY1;
			srcY1 = srcRect.top;
		}

		if (dstY1 < dstRect.top)
		{
			srcY1 += dstRect.top - dstY1;
			dstY1 = dstRect.top;
		}

		// clip to bottom
		if (srcY2 >= srcRect.bottom)
		{
			dstY2 -= srcY2 - srcRect.bottom;
			srcY2 = srcRect.bottom;
		}

		if (dstY2 >= dstRect.bottom)
		{
			srcY2 -= dstY2 - dstRect.bottom;
			dstY2 = dstRect.bottom;
		}

		srcDx = pSrcRect->right - srcX;
		dstDx = pDstRect->right - dstX;

		// if horzontal delta is negative 
		if (srcDx < 0)	// source
		{
			srcX = pSrcRect->right;	// start at other edge
			srcDx *= -1;			// ABS the delta
		}

		if (dstDx < 0)	// destination
		{
			dstX = pDstRect->right;	// start at other edge
			dstDx *= -1;			// ABS the delta
		}

		if (srcDx != dstDx)	// if these don't match...
			return;			// (we don't do any scaling here!)

		srcPixelPtr = srcBaseAddr + (srcRowBytes * (srcY1 - srcRect.top)) + (srcX - srcRect.left);
		dstPixelPtr = dstBaseAddr + (dstRowBytes * (dstY1 - dstRect.top)) + (dstX - dstRect.left);

		while ((srcY1 <= srcY2) && (dstY1 <= dstY2))
		{
			UInt32* srcPtr = (UInt32*) srcPixelPtr;
			UInt32* dstPtr = (UInt32*) dstPixelPtr;
			SInt32 deltaX = srcDx;

			while (deltaX--)
			{
				UInt32 srcPixel = *srcPtr;

				UInt32 srcAlpha = srcPixel >> 24;	// get alpha info
				UInt32 srcNotAlpha = 255 - srcAlpha;

				UInt32 srcRed = (srcPixel >> 16) & 0xFF;
				UInt32 srcGreen = (srcPixel >> 8) & 0xFF;
				UInt32 srcBlue = srcPixel & 0xFF;

				UInt32 dstPixel = *dstPtr;
				UInt32 dstRed = (dstPixel >> 16) & 0xFF;
				UInt32 dstGreen = (dstPixel >> 8) & 0xFF;
				UInt32 dstBlue = dstPixel & 0xFF;

				dstRed = (srcRed * srcAlpha / 256) + (dstRed * srcNotAlpha / 256);
				dstGreen = (srcGreen * srcAlpha / 256) + (dstGreen * srcNotAlpha / 256);
				dstBlue = (srcBlue * srcAlpha / 256) + (dstBlue * srcNotAlpha / 256);

				*(UInt32*) dstPtr = (dstRed << 16) | (dstGreen << 8) | dstBlue;

				srcPtr++;
				dstPtr++;
		
			}
			srcPixelPtr += srcRowBytes;
			dstPixelPtr += dstRowBytes;

			srcY1++;
			dstY1++;
		}
#endif
	}
}

void QDMP_Copy_Scan(PixMapHandle pSrcPixMapHdl,PixMapHandle pDstPixMapHdl,const SInt16 pSrcY,const SInt16 pSrcX1,const SInt16 pSrcX2,const SInt16 pDstY,const SInt16 pDstX1,const SInt16 pDstX2)
{
	if ((*pSrcPixMapHdl)->pixelSize == (*pDstPixMapHdl)->pixelSize)
	{
		switch ((*pSrcPixMapHdl)->pixelSize)
		{
		case	8:
			QDMP_Copy_Scan8(pSrcPixMapHdl,pDstPixMapHdl,pSrcY,pSrcX1,pSrcX2,pDstY,pDstX1,pDstX2);
			break;
		case	16:
			QDMP_Copy_Scan16(pSrcPixMapHdl,pDstPixMapHdl,pSrcY,pSrcX1,pSrcX2,pDstY,pDstX1,pDstX2);
			break;
		case	32:
			QDMP_Copy_Scan32(pSrcPixMapHdl,pDstPixMapHdl,pSrcY,pSrcX1,pSrcX2,pDstY,pDstX1,pDstX2);
			break;
		}
	}
}

void QDMP_Copy_Scan8(PixMapHandle pSrcPixMapHdl,PixMapHandle pDstPixMapHdl,const SInt16 pSrcY,const SInt16 pSrcX1,const SInt16 pSrcX2,const SInt16 pDstY,const SInt16 pDstX1,const SInt16 pDstX2)
{
	Rect srcRect = (*pSrcPixMapHdl)->bounds;
	Rect dstRect = (*pDstPixMapHdl)->bounds;

	UInt8 *srcBaseAddr = (UInt8*) GET_PIXBASEADDR(pSrcPixMapHdl),*srcPixelPtr;
	UInt8 *dstBaseAddr = (UInt8*) GET_PIXBASEADDR(pDstPixMapHdl),*dstPixelPtr;

	UInt32 srcRowBytes = GET_PIXROWBYTES(pSrcPixMapHdl);
	UInt32 dstRowBytes = GET_PIXROWBYTES(pDstPixMapHdl);

	SInt16 srcX = pSrcX1,srcDx = pSrcX2 - srcX;
	SInt16 dstX = pDstX1,dstDx = pDstX2 - dstX;

	if ((pSrcY < srcRect.top) || (pSrcY >= srcRect.bottom))
		return;	// offscreen

	if ((pDstY < dstRect.top) || (pDstY >= dstRect.bottom))
		return;	// offscreen

	if (srcDx < 0)
	{
		srcX = pSrcX2;
		srcDx *= -1;
	}

	if (dstDx < 0)
	{
		dstX = pDstX2;
		dstDx *= -1;
	}

	if (srcDx != dstDx)
		return;		// Hey,we don't do any scaling here!

	srcPixelPtr = srcBaseAddr + (srcRowBytes * (pSrcY - srcRect.top)) + (srcX - srcRect.left);
	dstPixelPtr = dstBaseAddr + (dstRowBytes * (pDstY - dstRect.top)) + (dstX - dstRect.left);

	while (srcDx--)
	{
		*(UInt8*) dstPixelPtr = *(UInt8*) srcPixelPtr;
		srcPixelPtr++;
		dstPixelPtr++;
	}
}

void QDMP_Copy_Scan16(PixMapHandle pSrcPixMapHdl,PixMapHandle pDstPixMapHdl,const SInt16 pSrcY,const SInt16 pSrcX1,const SInt16 pSrcX2,const SInt16 pDstY,const SInt16 pDstX1,const SInt16 pDstX2)
{
	Rect srcRect = (*pSrcPixMapHdl)->bounds;
	Rect dstRect = (*pDstPixMapHdl)->bounds;

	UInt8 *srcBaseAddr = (UInt8*) GET_PIXBASEADDR(pSrcPixMapHdl),*srcPixelPtr;
	UInt8 *dstBaseAddr = (UInt8*) GET_PIXBASEADDR(pDstPixMapHdl),*dstPixelPtr;

	UInt32 srcRowBytes = GET_PIXROWBYTES(pSrcPixMapHdl);
	UInt32 dstRowBytes = GET_PIXROWBYTES(pDstPixMapHdl);

	SInt16 srcX = pSrcX1,srcDx = pSrcX2 - srcX;
	SInt16 dstX = pDstX1,dstDx = pDstX2 - dstX;

	if ((pSrcY < srcRect.top) || (pSrcY >= srcRect.bottom))
		return;	// offscreen

	if ((pDstY < dstRect.top) || (pDstY >= dstRect.bottom))
		return;	// offscreen

	if (srcDx < 0)
	{
		srcX = pSrcX2;
		srcDx *= -1;
	}

	if (dstDx < 0)
	{
		dstX = pDstX2;
		dstDx *= -1;
	}

	if (srcDx != dstDx)
		return;		// Hey,we don't do any scaling here!

	srcPixelPtr = srcBaseAddr + (srcRowBytes * (pSrcY - srcRect.top)) + ((srcX - srcRect.left) << 1);
	dstPixelPtr = dstBaseAddr + (dstRowBytes * (pDstY - dstRect.top)) + ((dstX - dstRect.left) << 1);

	while (srcDx--)
	{
		*(UInt16*) dstPixelPtr = *(UInt16*) srcPixelPtr;
		srcPixelPtr += 2;
		dstPixelPtr += 2;
	}
}

void QDMP_Copy_Scan32(PixMapHandle pSrcPixMapHdl,PixMapHandle pDstPixMapHdl,const SInt16 pSrcY,const SInt16 pSrcX1,const SInt16 pSrcX2,const SInt16 pDstY,const SInt16 pDstX1,const SInt16 pDstX2)
{
	Rect srcRect = (*pSrcPixMapHdl)->bounds;
	Rect dstRect = (*pDstPixMapHdl)->bounds;

	UInt8 *srcBaseAddr = (UInt8*) GET_PIXBASEADDR(pSrcPixMapHdl),*srcPixelPtr;
	UInt8 *dstBaseAddr = (UInt8*) GET_PIXBASEADDR(pDstPixMapHdl),*dstPixelPtr;

	UInt32 srcRowBytes = GET_PIXROWBYTES(pSrcPixMapHdl);
	UInt32 dstRowBytes = GET_PIXROWBYTES(pDstPixMapHdl);

	SInt16 srcX = pSrcX1,srcDx = pSrcX2 - srcX;
	SInt16 dstX = pDstX1,dstDx = pDstX2 - dstX;

	if ((pSrcY < srcRect.top) || (pSrcY >= srcRect.bottom))
		return;	// offscreen

	if ((pDstY < dstRect.top) || (pDstY >= dstRect.bottom))
		return;	// offscreen

	if (srcDx < 0)
	{
		srcX = pSrcX2;
		srcDx *= -1;
	}

	if (dstDx < 0)
	{
		dstX = pDstX2;
		dstDx *= -1;
	}

	if (srcDx != dstDx)
		return;		// Hey,we don't do any scaling here!

	srcPixelPtr = srcBaseAddr + (srcRowBytes * (pSrcY - srcRect.top)) + ((srcX - srcRect.left) << 2);
	dstPixelPtr = dstBaseAddr + (dstRowBytes * (pDstY - dstRect.top)) + ((dstX - dstRect.left) << 2);

	while (srcDx--)
	{
		*(UInt32*) dstPixelPtr = *(UInt32*) srcPixelPtr;
		srcPixelPtr += 4;
		dstPixelPtr += 4;
	}
}

void QDMP_Copy_Scan32A(PixMapHandle pSrcPixMapHdl,PixMapHandle pDstPixMapHdl,const SInt16 pSrcY,const SInt16 pSrcX1,const SInt16 pSrcX2,const SInt16 pDstY,const SInt16 pDstX1,const SInt16 pDstX2)
{
	if (pSrcPixMapHdl && *pSrcPixMapHdl && pDstPixMapHdl && *pDstPixMapHdl)
	{
		Rect srcRect = (*pSrcPixMapHdl)->bounds;
		Rect dstRect = (*pDstPixMapHdl)->bounds;

		UInt8 *srcBaseAddr = (UInt8*) GET_PIXBASEADDR(pSrcPixMapHdl),*srcPixelPtr;
		UInt8 *dstBaseAddr = (UInt8*) GET_PIXBASEADDR(pDstPixMapHdl),*dstPixelPtr;

		UInt32 srcRowBytes = GET_PIXROWBYTES(pSrcPixMapHdl);
		UInt32 dstRowBytes = GET_PIXROWBYTES(pDstPixMapHdl);

		SInt16 srcX = pSrcX1,srcDx = pSrcX2 - srcX;
		SInt16 dstX = pDstX1,dstDx = pDstX2 - dstX;

		if ((pSrcY < srcRect.top) || (pSrcY >= srcRect.bottom))
			return;	// offscreen

		if ((pDstY < dstRect.top) || (pDstY >= dstRect.bottom))
			return;	// offscreen

		if (srcDx < 0)
		{
			srcX = pSrcX2;
			srcDx *= -1;
		}

		if (dstDx < 0)
		{
			dstX = pDstX2;
			dstDx *= -1;
		}

		if (srcDx != dstDx)
			return;		// Hey,we don't do any scaling here!

		srcPixelPtr = srcBaseAddr + (srcRowBytes * (pSrcY - srcRect.top)) + ((srcX - srcRect.left) << 2);
		dstPixelPtr = dstBaseAddr + (dstRowBytes * (pDstY - dstRect.top)) + ((dstX - dstRect.left) << 2);

		while (srcDx--)
		{
			UInt32 dstPixel = *(UInt32*) dstPixelPtr;
			UInt32 srcPixel = *(UInt32*) srcPixelPtr;
			UInt32 srcAlpha = srcPixel >> 24;	// get alpha info
			UInt32 srcNotAlpha = 255 - srcAlpha;
			UInt32 srcRed = (srcPixel >> 16) & 0xFF;
			UInt32 srcGreen = (srcPixel >> 8) & 0xFF;
			UInt32 srcBlue = srcPixel & 0xFF;
			UInt32 dstRed = (dstPixel >> 16) & 0xFF;
			UInt32 dstGreen = (dstPixel >> 8) & 0xFF;
			UInt32 dstBlue = dstPixel & 0xFF;

			dstRed = (srcRed * srcAlpha / 256) + (dstRed * srcNotAlpha / 256);
			dstGreen = (srcGreen * srcAlpha / 256) + (dstGreen * srcNotAlpha / 256);
			dstBlue = (srcBlue * srcAlpha / 256) + (dstBlue * srcNotAlpha / 256);

			*(UInt32*) dstPixelPtr = (dstRed << 16) | (dstGreen << 8) | dstBlue;

			srcPixelPtr += 4;
			dstPixelPtr += 4;
		}
	}
}

void QDMP_Draw_Line(PixMapHandle pPixMapHdl,const SInt16 pX1,const SInt16 pY1,const SInt16 pX2,const SInt16 pY2,const RGBColor* pColor)
{
	if (pPixMapHdl && *pPixMapHdl)
	{
		switch ((*pPixMapHdl)->pixelSize)
		{
		case	8:
			{
				UInt8 color8 = RGBColorToColor8(pPixMapHdl,pColor);
				QDMP_Draw_Line8(pPixMapHdl,pX1,pY1,pX2,pY2,color8);
			}
			break;
		case	16:
			{
				UInt16 color16 = RGBColorToColor16(pColor);

				QDMP_Draw_Line16(pPixMapHdl,pX1,pY1,pX2,pY2,color16);
			}
			break;
		case	32:
			{
				UInt32 color32 = RGBColorToColor32(pColor);

				QDMP_Draw_Line32(pPixMapHdl,pX1,pY1,pX2,pY2,color32);
			}
			break;
		}
	}
}

void QDMP_Draw_Line8(PixMapHandle pPixMapHdl,const SInt16 pX1,const SInt16 pY1,const SInt16 pX2,const SInt16 pY2,const UInt8 pColor)
{
	if (pPixMapHdl && *pPixMapHdl)
	{
		Rect tRect = (*pPixMapHdl)->bounds;
		UInt8 *baseAddr = (UInt8*) GET_PIXBASEADDR(pPixMapHdl),*pixelPtr;
		UInt32 rowBytes = GET_PIXROWBYTES(pPixMapHdl);

		SInt16 x = pX1,y = pY1,xi = 1,yi = 1,dx = pX2 - x,dy = pY2 - y,i,j;

		if (dx < 0)
		{
			dx = -dx;
			xi = -1;
		}

		if (dy < 0)
		{
			dy = -dy;
			yi = -1;
		}

	//	QDMP_Set_Pixel8(pPixMapHdl,x,y,pColor);
		pixelPtr = baseAddr + (rowBytes * (y - tRect.top)) + (x - tRect.left);
		if ((y >= tRect.top) && (y < tRect.bottom) &&
			(x >= tRect.left) && (x < tRect.right))
			*(UInt8*) pixelPtr = pColor;

		if (dy > dx)
		{
			j = dy;
			i = dy >> 1;
			do
			{
				y += yi;
				pixelPtr += rowBytes * yi;
				if ((i += dx) > dy)
				{
					i -= dy;
					x += xi;
					pixelPtr += xi;
				}
	//			QDMP_Set_Pixel8(pPixMapHdl,x,y,pColor);
				if ((y >= tRect.top) && (y < tRect.bottom) &&
					(x >= tRect.left) && (x < tRect.right))
					*(UInt8*) pixelPtr = pColor;
			} while(--j);
		}
		else
		{
			j = dx;
			i = dx >> 1;
			while (j--)
			{
				x += xi;
				pixelPtr += xi;
				if ((i += dy) > dx)
				{
					i -= dx;
					y += yi;
					pixelPtr += rowBytes * yi;
				}
	//			QDMP_Set_Pixel8(pPixMapHdl,x,y,pColor);
				if ((y >= tRect.top) && (y < tRect.bottom) &&
					(x >= tRect.left) && (x < tRect.right))
					*(UInt8*) pixelPtr = pColor;
			}
		}
	}
}

void QDMP_Draw_Line16(PixMapHandle pPixMapHdl,const SInt16 pX1,const SInt16 pY1,const SInt16 pX2,const SInt16 pY2,const UInt16 pColor)
{
	if (pPixMapHdl && *pPixMapHdl)
	{
		Rect tRect = (*pPixMapHdl)->bounds;
		UInt8 *baseAddr = (UInt8*) GET_PIXBASEADDR(pPixMapHdl),*pixelPtr;
		UInt32 rowBytes = GET_PIXROWBYTES(pPixMapHdl);

		SInt16 x = pX1,y = pY1,xi = 1,yi = 1,dx = pX2 - x,dy = pY2 - y,i,j;

		if (dx < 0)
		{
			dx = -dx;
			xi = -1;
		}

		if (dy < 0)
		{
			dy = -dy;
			yi = -1;
		}

	//	QDMP_Set_Pixel16(pPixMapHdl,x,y,pColor);
		pixelPtr = baseAddr + (rowBytes * (y - tRect.top)) + ((x - tRect.left) << 1);
		if ((y >= tRect.top) && (y < tRect.bottom) &&
			(x >= tRect.left) && (x < tRect.right))
			*(UInt16*) pixelPtr = pColor;

		if (dy > dx)
		{
			j = dy;
			i = dy >> 1;
			do
			{
				y += yi;
				pixelPtr += rowBytes * yi;
				if ((i += dx) > dy)
				{
					i -= dy;
					x += xi;
					pixelPtr += xi << 1;
				}
	//			QDMP_Set_Pixel16(pPixMapHdl,x,y,pColor);
				if ((y >= tRect.top) && (y < tRect.bottom) &&
					(x >= tRect.left) && (x < tRect.right))
					*(UInt16*) pixelPtr = pColor;
			} while(--j);
		}
		else
		{
			j = dx;
			i = dx >> 1;
			while (j--)
			{
				x += xi;
				pixelPtr += xi << 1;
				if ((i += dy) > dx)
				{
					i -= dx;
					y += yi;
					pixelPtr += rowBytes * yi;
				}
	//			QDMP_Set_Pixel16(pPixMapHdl,x,y,pColor);
				if ((y >= tRect.top) && (y < tRect.bottom) &&
					(x >= tRect.left) && (x < tRect.right))
					*(UInt16*) pixelPtr = pColor;
			}
		}
	}
}

void QDMP_Draw_Line32(PixMapHandle pPixMapHdl,const SInt16 pX1,const SInt16 pY1,const SInt16 pX2,const SInt16 pY2,const UInt32 pColor)
{
	if (pPixMapHdl && *pPixMapHdl)
	{
		Rect tRect = (*pPixMapHdl)->bounds;
		UInt8 *baseAddr = (UInt8*) GET_PIXBASEADDR(pPixMapHdl),*pixelPtr;
		UInt32 rowBytes = GET_PIXROWBYTES(pPixMapHdl);

		SInt16 x = pX1,y = pY1,xi = 1,yi = 1,dx = pX2 - x,dy = pY2 - y,i,j;

		if (dx < 0)
		{
			dx = -dx;
			xi = -1;
		}

		if (dy < 0)
		{
			dy = -dy;
			yi = -1;
		}

	//	QDMP_Set_Pixel32(pPixMapHdl,x,y,pColor);
		pixelPtr = baseAddr + (rowBytes * (y - tRect.top)) + ((x - tRect.left) << 2);
		if ((y >= tRect.top) && (y < tRect.bottom) &&
			(x >= tRect.left) && (x < tRect.right))
			*(UInt32*) pixelPtr = pColor;

		if (dy > dx)
		{
			j = dy;
			i = dy >> 1;
			do
			{
				y += yi;
				pixelPtr += rowBytes * yi;
				if ((i += dx) > dy)
				{
					i -= dy;
					x += xi;
					pixelPtr += xi << 2;
				}
	//			QDMP_Set_Pixel32(pPixMapHdl,x,y,pColor);
				if ((y >= tRect.top) && (y < tRect.bottom) &&
					(x >= tRect.left) && (x < tRect.right))
					*(UInt32*) pixelPtr = pColor;
			} while(--j);
		}
		else
		{
			j = dx;
			i = dx >> 1;
			while (j--)
			{
				x += xi;
				pixelPtr += xi << 2;
				if ((i += dy) > dx)
				{
					i -= dx;
					y += yi;
					pixelPtr += rowBytes * yi;
				}
	//			QDMP_Set_Pixel32(pPixMapHdl,x,y,pColor);
				if ((y >= tRect.top) && (y < tRect.bottom) &&
					(x >= tRect.left) && (x < tRect.right))
					*(UInt32*) pixelPtr = pColor;
			}
		}
	}
}

void QDMP_Draw_LineP(PixMapHandle pPixMapHdl,const Point pPointA,const Point pPointB,const RGBColor* pColor)
{
	QDMP_Draw_Line(pPixMapHdl,pPointA.h,pPointA.v,pPointB.h,pPointB.v,pColor);
}

void QDMP_Draw_LineP8(PixMapHandle pPixMapHdl,const Point pPointA,const Point pPointB,const UInt8 pColor)
{
	QDMP_Draw_Line8(pPixMapHdl,pPointA.h,pPointA.v,pPointB.h,pPointB.v,pColor);
}

void QDMP_Draw_LineP16(PixMapHandle pPixMapHdl,const Point pPointA,const Point pPointB,const UInt16 pColor)
{
	QDMP_Draw_Line16(pPixMapHdl,pPointA.h,pPointA.v,pPointB.h,pPointB.v,pColor);
}

void QDMP_Draw_LineP32(PixMapHandle pPixMapHdl,const Point pPointA,const Point pPointB,const UInt32 pColor)
{
	QDMP_Draw_Line32(pPixMapHdl,pPointA.h,pPointA.v,pPointB.h,pPointB.v,pColor);
}

static inline void Draw_Scan8(PixMapHandle pPixMapHdl,const SInt16 pV,const SInt16 pX1,const SInt16 pX2,const UInt8 pColor)
{
	if (pPixMapHdl && *pPixMapHdl)
	{
		Rect tRect = (*pPixMapHdl)->bounds;
		UInt8 *baseAddr = (UInt8*) GET_PIXBASEADDR(pPixMapHdl),*pixelPtr;
		UInt32 rowBytes = GET_PIXROWBYTES(pPixMapHdl);

		SInt16 x = pX1,dx = pX2 - x;

		if ((pV < tRect.top) || (pV >= tRect.bottom))
			return;	// offscreen

		if (dx < 0)
		{
			x = pX2;
			dx = -dx;
		}

		pixelPtr = baseAddr + (rowBytes * (pV - tRect.top)) + (x - tRect.left);

		while (dx--)
		{
			*(UInt8*) pixelPtr = pColor;
			pixelPtr++;
		}
	}
}

static inline void Draw_Scan16(PixMapHandle pPixMapHdl,const SInt16 pV,const SInt16 pX1,const SInt16 pX2,const UInt16 pColor)
{
	if (pPixMapHdl && *pPixMapHdl)
	{
		Rect tRect = (*pPixMapHdl)->bounds;
		UInt8 *baseAddr = (UInt8*) GET_PIXBASEADDR(pPixMapHdl),*pixelPtr;
		UInt32 rowBytes = GET_PIXROWBYTES(pPixMapHdl);

		SInt16 x = pX1,dx = pX2 - x;

		if ((pV < tRect.top) || (pV >= tRect.bottom))
			return;	// offscreen

		if (dx < 0)
		{
			x = pX2;
			dx = -dx;
		}

		pixelPtr = baseAddr + (rowBytes * (pV - tRect.top)) + ((x - tRect.left) << 1);

		while (dx--)
		{
			*(UInt16*) pixelPtr = pColor;
			pixelPtr += 2;
		}
	}
}

static inline void Draw_Scan32(PixMapHandle pPixMapHdl,const SInt16 pV,const SInt16 pX1,const SInt16 pX2,const UInt32 pColor)
{
	if (pPixMapHdl && *pPixMapHdl)
	{
		Rect tRect = (*pPixMapHdl)->bounds;
		UInt8 *baseAddr = (UInt8*) GET_PIXBASEADDR(pPixMapHdl),*pixelPtr;
		UInt32 rowBytes = GET_PIXROWBYTES(pPixMapHdl);

		SInt16 x = pX1,dx = pX2 - x;

		if ((pV < tRect.top) || (pV >= tRect.bottom))
			return;	// offscreen

		if (dx < 0)
		{
			x = pX2;
			dx = -dx;
		}

		pixelPtr = baseAddr + (rowBytes * (pV - tRect.top)) + ((x - tRect.left) << 2);

		while (dx--)
		{
			*(UInt32*) pixelPtr = pColor;
			pixelPtr += 4;
		}
	}
}

static inline  void Draw_Scan(PixMapHandle pPixMapHdl,const SInt16 pV,const SInt16 pX1,const SInt16 pX2,const RGBColor* pColor)
{
	if (pPixMapHdl && *pPixMapHdl)
	{
		switch ((*pPixMapHdl)->pixelSize)
		{
		case	8:
			{
				UInt8 color8 = RGBColorToColor8(pPixMapHdl,pColor);
				Draw_Scan8(pPixMapHdl,pV,pX1,pX2,color8);
			}
			break;
		case	16:
			{
				UInt16 color16 = RGBColorToColor16(pColor);

				Draw_Scan16(pPixMapHdl,pV,pX1,pX2,color16);
			}
			break;
		case	32:
			{
				UInt32 color32 = RGBColorToColor32(pColor);

				Draw_Scan32(pPixMapHdl,pV,pX1,pX2,color32);
			}
			break;
		}
	}
}

void QDMP_Fill_Rect(PixMapHandle pPixMapHdl,const Rect pRect,const RGBColor* pColor)
{
	if (pPixMapHdl && *pPixMapHdl)
	{
		switch ((*pPixMapHdl)->pixelSize)
		{
		case	8:
			{
				UInt8 color8 = RGBColorToColor8(pPixMapHdl,pColor);
				QDMP_Fill_Rect8(pPixMapHdl,pRect,color8);
			}
			break;
		case	16:
			{
				UInt16 color16 = RGBColorToColor16(pColor);

				QDMP_Fill_Rect16(pPixMapHdl,pRect,color16);
			}
			break;
		case	32:
			{
				UInt32 color32 = RGBColorToColor32(pColor);

				QDMP_Fill_Rect32(pPixMapHdl,pRect,color32);
			}
			break;
		}
	}
}

void QDMP_Fill_Rect8(PixMapHandle pPixMapHdl,const Rect pRect,const UInt8 pColor)
{
	SInt16 r1,r2,c1,c2;

	r1 = pRect.top;
	r2 = pRect.bottom;
	c1 = pRect.left;
	c2 = pRect.right;

	while (r1 < r2)
	{
		QDMP_Draw_Line8(pPixMapHdl,r1,c1,r1,c2,pColor);
		r1++;
	}
}

void QDMP_Fill_Rect16(PixMapHandle pPixMapHdl,const Rect pRect,const UInt16 pColor)
{
	SInt16 r1,r2,c1,c2;

	r1 = pRect.top;
	r2 = pRect.bottom;
	c1 = pRect.left;
	c2 = pRect.right;

	while (r1 < r2)
	{
		QDMP_Draw_Line16(pPixMapHdl,r1,c1,r1,c2,pColor);
		r1++;
	}
}

void QDMP_Fill_Rect32(PixMapHandle pPixMapHdl,const Rect pRect,const UInt32 pColor)
{
	SInt16 r1,r2,c1,c2;

	r1 = pRect.top;
	r2 = pRect.bottom;
	c1 = pRect.left;
	c2 = pRect.right;

	while (r1 < r2)
	{
		Draw_Scan32(pPixMapHdl,r1,c1,c2,pColor);
		r1++;
	}
}

void QDMP_Draw_Circle(PixMapHandle pPixMapHdl,const SInt16 pH,const SInt16 pV,const UInt16 pRadius,const RGBColor* pColor)
{
	if (pPixMapHdl && *pPixMapHdl)
	{
		switch ((*pPixMapHdl)->pixelSize)
		{
		case	8:
			{
				UInt8 color8 = RGBColorToColor8(pPixMapHdl,pColor);
				QDMP_Draw_Circle8(pPixMapHdl,pH,pV,pRadius,color8);
			}
			break;
		case	16:
			{
				UInt16 color16 = RGBColorToColor16(pColor);

				QDMP_Draw_Circle16(pPixMapHdl,pH,pV,pRadius,color16);
			}
			break;
		case	32:
			{
				UInt32 color32 = RGBColorToColor32(pColor);

				QDMP_Draw_Circle32(pPixMapHdl,pH,pV,pRadius,color32);
			}
			break;
		}
	}
}

void QDMP_Draw_Circle8(PixMapHandle pPixMapHdl,const SInt16 pH,const SInt16 pV,const UInt16 pRadius,const UInt8 pColor)
{
	SInt16	x = 0,y = pRadius,	d = 3 - 2 * pRadius;

	do
	{
		QDMP_Set_Pixel8(pPixMapHdl,pH - x,pV - y,pColor);
		if (y)
			QDMP_Set_Pixel8(pPixMapHdl,pH - x,pV + y,pColor);

		if (x)
		{
			QDMP_Set_Pixel8(pPixMapHdl,pH + x,pV - y,pColor);
			if (y)
				QDMP_Set_Pixel8(pPixMapHdl,pH + x,pV + y,pColor);
		}

		if (x != y)
		{
			QDMP_Set_Pixel8(pPixMapHdl,pH - y,pV - x,pColor);
			if (x)
				QDMP_Set_Pixel8(pPixMapHdl,pH - y,pV + x,pColor);
			if (y)
			{
				QDMP_Set_Pixel8(pPixMapHdl,pH + y,pV - x,pColor);
				if (x)
					QDMP_Set_Pixel8(pPixMapHdl,pH + y,pV + x,pColor);
			}
		}
		x++;

		if (d < 0)
			d += 4 * x + 6;
		else
		{
			d += 4 * (x - y) + 10;
			y--;
		}
	} while (x <= y);
}

void QDMP_Draw_Circle16(PixMapHandle pPixMapHdl,const SInt16 pH,const SInt16 pV,const UInt16 pRadius,const UInt16 pColor)
{
	SInt16	x = 0,y = pRadius,	d = 3 - 2 * pRadius;

	do
	{
		QDMP_Set_Pixel16(pPixMapHdl,pH - x,pV - y,pColor);
		if (y)
			QDMP_Set_Pixel16(pPixMapHdl,pH - x,pV + y,pColor);

		if (x)
		{
			QDMP_Set_Pixel16(pPixMapHdl,pH + x,pV - y,pColor);
			if (y)
				QDMP_Set_Pixel16(pPixMapHdl,pH + x,pV + y,pColor);
		}

		if (x != y)
		{
			QDMP_Set_Pixel16(pPixMapHdl,pH - y,pV - x,pColor);
			if (x)
				QDMP_Set_Pixel16(pPixMapHdl,pH - y,pV + x,pColor);
			if (y)
			{
				QDMP_Set_Pixel16(pPixMapHdl,pH + y,pV - x,pColor);
				if (x)
					QDMP_Set_Pixel16(pPixMapHdl,pH + y,pV + x,pColor);
			}
		}
		x++;

		if (d < 0)
			d += 4 * x + 6;
		else
		{
			d += 4 * (x - y) + 10;
			y--;
		}
	} while (x <= y);
}

void QDMP_Draw_Circle32(PixMapHandle pPixMapHdl,const SInt16 pH,const SInt16 pV,const UInt16 pRadius,const UInt32 pColor)
{
	SInt16	x = 0,y = pRadius,	d = 3 - 2 * pRadius;

	do
	{
		QDMP_Set_Pixel32(pPixMapHdl,pH - x,pV - y,pColor);

		if (y)
			QDMP_Set_Pixel32(pPixMapHdl,pH - x,pV + y,pColor);

		if (x)
		{
			QDMP_Set_Pixel32(pPixMapHdl,pH + x,pV - y,pColor);
			if (y)
				QDMP_Set_Pixel32(pPixMapHdl,pH + x,pV + y,pColor);
		}

		if (x != y)
		{
			QDMP_Set_Pixel32(pPixMapHdl,pH - y,pV - x,pColor);
			if (x)
				QDMP_Set_Pixel32(pPixMapHdl,pH - y,pV + x,pColor);
			if (y)
			{
				QDMP_Set_Pixel32(pPixMapHdl,pH + y,pV - x,pColor);
				if (x)
					QDMP_Set_Pixel32(pPixMapHdl,pH + y,pV + x,pColor);
			}
		}

		x++;

		if (d < 0)
			d += 4 * x + 6;
		else
		{
			d += 4 * (x - y) + 10;
			y--;
		}
	} while (x <= y);
}

void QDMP_Draw_CircleP(PixMapHandle pPixMapHdl,const Point pCenter,const UInt16 pRadius,const RGBColor* pColor)
{
	QDMP_Draw_Circle(pPixMapHdl,pCenter.h,pCenter.v,pRadius,pColor);
}

void QDMP_Draw_CircleP8(PixMapHandle pPixMapHdl,const Point pCenter,const UInt16 pRadius,const UInt8 pColor)
{
	QDMP_Draw_Circle8(pPixMapHdl,pCenter.h,pCenter.v,pRadius,pColor);
}

void QDMP_Draw_CircleP16(PixMapHandle pPixMapHdl,const Point pCenter,const UInt16 pRadius,const UInt16 pColor)
{
	QDMP_Draw_Circle16(pPixMapHdl,pCenter.h,pCenter.v,pRadius,pColor);
}

void QDMP_Draw_CircleP32(PixMapHandle pPixMapHdl,const Point pCenter,const UInt16 pRadius,const UInt32 pColor)
{
	QDMP_Draw_Circle32(pPixMapHdl,pCenter.h,pCenter.v,pRadius,pColor);
}

void QDMP_Fill_Circle(PixMapHandle pPixMapHdl,const SInt16 pH,const SInt16 pV,const UInt16 pRadius,const RGBColor* pColor)
{
	if (pPixMapHdl && *pPixMapHdl)
	{
		switch ((*pPixMapHdl)->pixelSize)
		{
		case	8:
			{
				UInt8 color8 = RGBColorToColor8(pPixMapHdl,pColor);
				QDMP_Fill_Circle8(pPixMapHdl,pH,pV,pRadius,color8);
			}
			break;
		case	16:
			{
				UInt16 color16 = RGBColorToColor16(pColor);

				QDMP_Fill_Circle16(pPixMapHdl,pH,pV,pRadius,color16);
			}
			break;
		case	32:
			{
				UInt32 color32 = RGBColorToColor32(pColor);

				QDMP_Fill_Circle32(pPixMapHdl,pH,pV,pRadius,color32);
			}
			break;
		}
	}
}

void QDMP_Fill_Circle8(PixMapHandle pPixMapHdl,const SInt16 pH,const SInt16 pV,const UInt16 pRadius,const UInt8 pColor)
{
	SInt16	x = 0,y = pRadius,	d = 3 - 2 * pRadius;

	do
	{
		Draw_Scan8(pPixMapHdl,pV - y,pH - x,pH + x,pColor);
		if (y)	// so (y == 0) doesn't draw twice
			Draw_Scan8(pPixMapHdl,pV + y,pH - x,pH + x,pColor);

		Draw_Scan8(pPixMapHdl,pV - x,pH - y,pH + y,pColor);
		if (x)	// so (x == 0) doesn't draw twice
			Draw_Scan8(pPixMapHdl,pV + x,pH - y,pH + y,pColor);

		x++;

		if (d < 0)
			d += 4 * x + 6;
		else
		{
			d += 4 * (x - y) + 10;
			y--;
		}
	} while (x <= y);
}

void QDMP_Fill_Circle16(PixMapHandle pPixMapHdl,const SInt16 pH,const SInt16 pV,const UInt16 pRadius,const UInt16 pColor)
{
	SInt16	x = 0,y = pRadius,	d = 3 - 2 * pRadius;

	do
	{
		Draw_Scan16(pPixMapHdl,pV - y,pH - x,pH + x,pColor);
		if (y)	// so (y == 0) doesn't draw twice
			Draw_Scan16(pPixMapHdl,pV + y,pH - x,pH + x,pColor);

		Draw_Scan16(pPixMapHdl,pV - x,pH - y,pH + y,pColor);
		if (x)	// so (x == 0) doesn't draw twice
			Draw_Scan16(pPixMapHdl,pV + x,pH - y,pH + y,pColor);

		x++;

		if (d < 0)
			d += 4 * x + 6;
		else
		{
			d += 4 * (x - y) + 10;
			y--;
		}
	} while (x <= y);
}

void QDMP_Fill_Circle32(PixMapHandle pPixMapHdl,const SInt16 pH,const SInt16 pV,const UInt16 pRadius,const UInt32 pColor)
{
	SInt16	x = 0,y = pRadius,	d = 3 - 2 * pRadius;

	do
	{
		Draw_Scan32(pPixMapHdl,pV - y,pH - x,pH + x,pColor);
		if (y)	// so (y == 0) doesn't draw twice
			Draw_Scan32(pPixMapHdl,pV + y,pH - x,pH + x,pColor);

		Draw_Scan32(pPixMapHdl,pV - x,pH - y,pH + y,pColor);
		if (x)	// so (x == 0) doesn't draw twice
			Draw_Scan32(pPixMapHdl,pV + x,pH - y,pH + y,pColor);

		x++;

		if (d < 0)
			d += 4 * x + 6;
		else
		{
			d += 4 * (x - y) + 10;
			y--;
		}
	} while (x <= y);
}

void QDMP_Fill_CircleP(PixMapHandle pPixMapHdl,const Point pCenter,const UInt16 pRadius,const RGBColor* pColor)
{
	QDMP_Fill_Circle(pPixMapHdl,pCenter.h,pCenter.v,pRadius,pColor);
}

void QDMP_Fill_CircleP8(PixMapHandle pPixMapHdl,const Point pCenter,const UInt16 pRadius,const UInt8 pColor)
{
	QDMP_Fill_Circle8(pPixMapHdl,pCenter.h,pCenter.v,pRadius,pColor);
}

void QDMP_Fill_CircleP16(PixMapHandle pPixMapHdl,const Point pCenter,const UInt16 pRadius,const UInt16 pColor)
{
	QDMP_Fill_Circle16(pPixMapHdl,pCenter.h,pCenter.v,pRadius,pColor);
}

void QDMP_Fill_CircleP32(PixMapHandle pPixMapHdl,const Point pCenter,const UInt16 pRadius,const UInt32 pColor)
{
	QDMP_Fill_Circle32(pPixMapHdl,pCenter.h,pCenter.v,pRadius,pColor);
}

static inline UInt32 distance_squared(const Point pPointA,const Point pPointB)
{
	Point deltaPoint;

	deltaPoint.h = pPointA.h - pPointB.h;
	deltaPoint.v = pPointA.v - pPointB.v;

	return ((deltaPoint.h * deltaPoint.h) + (deltaPoint.v * deltaPoint.v));
}

void QDMP_Draw_Curve(PixMapHandle pPixMapHdl,const Point pPointA,const Point pPointB,const Point pPointC,const RGBColor* pColor)
{
	Point myMidPointAB,myMidPointBC,myMidPointABC;

	myMidPointAB = QDMP_MidPoint(pPointA,pPointB);
	myMidPointBC = QDMP_MidPoint(pPointB,pPointC);
	myMidPointABC = QDMP_MidPoint(myMidPointAB,myMidPointBC);

	if (distance_squared(pPointA,myMidPointABC) > 100)
		QDMP_Draw_Curve(pPixMapHdl,pPointA,myMidPointAB,myMidPointABC,pColor);
	else
	{
		QDMP_Draw_Line(pPixMapHdl,pPointA.h,pPointA.v,myMidPointAB.h,myMidPointAB.v,pColor);
		QDMP_Draw_Line(pPixMapHdl,myMidPointAB.h,myMidPointAB.v,myMidPointABC.h,myMidPointABC.v,pColor);
	}

	if (distance_squared(myMidPointABC,pPointB) > 100)
		QDMP_Draw_Curve(pPixMapHdl,myMidPointABC,myMidPointBC,pPointC,pColor);
	else
	{
		QDMP_Draw_Line(pPixMapHdl,myMidPointABC.h,myMidPointABC.v,myMidPointBC.h,myMidPointBC.v,pColor);
		QDMP_Draw_Line(pPixMapHdl,myMidPointBC.h,myMidPointBC.v,pPointC.h,pPointC.v,pColor);
	}
}

/**\
|**|	Create quarter size (MipMap) image
\**/

void QDMP_DownMip32A(PixMapHandle pSrcPixMapHdl,PixMapHandle pDstPixMapHdl)
{
	if (pSrcPixMapHdl && *pSrcPixMapHdl && pDstPixMapHdl && *pDstPixMapHdl)
	{
		Rect	srcRect = (*pSrcPixMapHdl)->bounds;
		Rect	dstRect = (*pSrcPixMapHdl)->bounds;
		UInt32* srcBaseAddr = (UInt32*) GET_PIXBASEADDR(pSrcPixMapHdl);
		UInt32* dstBaseAddr = (UInt32*) GET_PIXBASEADDR(pDstPixMapHdl);
		UInt32 srcRowBytes = GET_PIXROWBYTES(pSrcPixMapHdl);
		UInt32 srcRowBytes2 = srcRowBytes << 1;
		UInt32 dstRowBytes = GET_PIXROWBYTES(pDstPixMapHdl);
		SInt32 rowIndex,rowDelta = srcRect.bottom - srcRect.top;
		SInt32 colIndex,colDelta = srcRect.right - srcRect.left;

		// (Ptr) forces pointer math; xxxBaseAddr now points to top left pixel of src/dst rect.
		(Ptr) srcBaseAddr += (srcRect.top * srcRowBytes) + (srcRect.left << 2);
		(Ptr) dstBaseAddr += (dstRect.top * dstRowBytes) + (dstRect.left << 2);

		// step thru source two rows at a time
		for (rowIndex = rowDelta; rowIndex > 0 ; rowIndex -= 2)
		{
			UInt32* srcPtr0 = srcBaseAddr;	// Ptr to first source row
			UInt32* srcPtr1 = 				// Ptr to second source row
				(UInt32*) ((Ptr)srcPtr0 + srcRowBytes);
			UInt32* dstPtr = dstBaseAddr;	// Ptr to dest row

			// step thru source two cols at a time
			for (colIndex = colDelta; colIndex > 0; colIndex -= 2)
			{
				// total of two pixels in first source row
				UInt32	total1 = ((srcPtr0[0] & 0xfefefefe) >> 1) +
								 ((srcPtr0[1] & 0xfefefefe) >> 1);
				// total of two pixels in second source row
				UInt32	total2 = ((srcPtr1[0] & 0xfefefefe) >> 1) +
								 ((srcPtr1[1] & 0xfefefefe) >> 1);

				// total of totals
				*dstPtr = ((total1 & 0xfefefefe) >> 1) + ((total2 & 0xfefefefe) >> 1);
				srcPtr0 += 2;
				srcPtr1 += 2;
				dstPtr++;
			}
			(Ptr) srcBaseAddr += srcRowBytes2;
			(Ptr) dstBaseAddr += dstRowBytes;
		}
	}
}

/**\
|**|	generate a random number +/- 32768
\**/

SInt16 QDMP_RandomSInt16(void)
{
	static SInt32 state = 314159;

	state = ((state * 1103515245) + 12345 ) & 0x7fffffff;

	return ((state >> 6) & 0xffff);
}

SInt16 QDMP_Random_RangeSInt16(const SInt16 pMin,const SInt16 pMax)
{
	UInt16 number = 32767 + QDMP_RandomSInt16();

	number = pMin + (number % (pMax - pMin));

	return number;
}

SInt32 QDMP_Random_RangeSInt32(const SInt32 pMin,const SInt32 pMax)
{
	UInt32 random = (QDMP_RandomSInt16() << 16) ^ (QDMP_RandomSInt16() << 8) ^ QDMP_RandomSInt16();
	SInt32 result = pMin + (random % (pMax - pMin + 1));

	if (result < pMin)
		result = pMin;
	else if (result > pMax)
		result = pMax;

	return result;
}

/* ======================================================= */
/* Routine: QDMP_Mix_RGBColors */
/* Purpose: mix two rgb colors according to the frac part of a float */

RGBColor QDMP_Mix_RGBColors(
	const RGBColor pRGBColorA,
	const RGBColor pRGBColorB,
	const float pMix)
{
	RGBColor tRGBColor;
	SInt32 mix = pMix;
	float fracB = pMix - mix;
	float fracA = 1.0f - fracB;

	tRGBColor.red = (pRGBColorA.red * fracA) + (pRGBColorB.red * fracB);
	tRGBColor.green = (pRGBColorA.green * fracA) + (pRGBColorB.green * fracB);
	tRGBColor.blue = (pRGBColorA.blue * fracA) + (pRGBColorB.blue * fracB);

	return tRGBColor;
}

/* ======================================================= */
/* Routine: QDMP_RGBColorDistance */
/* Purpose: Caclulate the distance between two RGBColors */

UInt32 QDMP_RGBColorDistance(
	const RGBColor pRGBColorA,
	const RGBColor pRGBColorB)
{
	SInt32 red,green,blue;

	red = pRGBColorA.red - pRGBColorB.red;
	green = pRGBColorA.green - pRGBColorB.green;
	blue = pRGBColorA.blue - pRGBColorB.blue;

	red *= red; green *= green; blue *= blue;
	return (sqrt(red + green + blue));
}
