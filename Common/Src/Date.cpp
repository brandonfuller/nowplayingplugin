// Date.cpp : implementation file
//
// IDate Implementation
//
// Written by Paul E. Bible <pbible@littlefishsoftware.com>
// Copyright (c) 2000. All Rights Reserved.
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed unmodified by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name and all copyright 
// notices remains intact. If the source code in this file is used in 
// any  commercial application then a statement along the lines of 
// "Portions copyright (c) Paul E. Bible, 2000" must be included in
// the startup banner, "About" box -OR- printed documentation. An email 
// letting me know that you are using it would be nice as well. That's 
// not much to ask considering the amount of work that went into this.
// If even this small restriction is a problem send me an email.
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability for any damage/loss of business that
// this product may cause.
//
// Expect bugs!
// 
// Please use and enjoy, and let me know of any bugs/mods/improvements 
// that you have found/implemented and I will fix/incorporate them into 
// this file. 
//
/////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "Date.h"

/////////////////////////////////////////////////////////////////////////////
// CDate Private Methods
//

BOOL CDate::IsLeapYear(const int &nYear)
{
	return ((nYear % 4 == 0 && nYear % 100 != 0) || (nYear % 400 == 0)) ? TRUE : FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CDate Conversion Methods (Public)
//

HRESULT CDate::Now(DATE *pdtDate)
{
	SYSTEMTIME stDate;
	GetLocalTime(&stDate);
	DATE dtDate;
	SystemTimeToVariantTime(&stDate, &dtDate);
	*pdtDate = dtDate;
	return S_OK;
}

HRESULT CDate::Int2OLE(short nMonth, short nDay, short nYear, DATE *pdtDate)
{
	SYSTEMTIME stDate;
	stDate.wDay		= static_cast<WORD>(nDay);
	stDate.wMonth	= static_cast<WORD>(nMonth);
	stDate.wYear	= static_cast<WORD>(nYear);
	stDate.wHour	= 0;
	stDate.wMinute	= 0;
	stDate.wSecond	= 1;

	DATE dtDate;
	if (SystemTimeToVariantTime(&stDate, &dtDate) == 0)
		return E_FAIL;
	*pdtDate = dtDate;
	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CDate Calculation Methods (Public)
//

HRESULT CDate::GetLastDay(DATE dtDate, DATE *pdtDate)
{
	SYSTEMTIME stDate;
	VariantTimeToSystemTime(dtDate, &stDate);
	int nMonth  = static_cast<int>(stDate.wMonth);
	int nYear	= static_cast<int>(stDate.wYear);
	int nDay	= 0;

	// Calculate the last day of the month
	switch (nMonth)
	{
		case 1:
		case 3:
		case 5:
		case 7:
		case 8:
		case 10:
		case 12:
			nDay = 31;
			break;
		case 4:
		case 6:
		case 9:
		case 11:
			nDay = 30;
			break;
		case 2:
			if (IsLeapYear(nYear))
				nDay = 29;
			else
				nDay = 28;
			break;
	}

	// Update SYSTEMTIME variable
	stDate.wDay = static_cast<WORD>(nDay);

	// Calc and assign new date
	DATE dtNew;
	SystemTimeToVariantTime(&stDate, &dtNew);
	*pdtDate = dtNew;
	return S_OK;
}

HRESULT CDate::GetFirstDay(DATE dtDate, DATE *pdtDate)
{
	SYSTEMTIME stDate;
	VariantTimeToSystemTime(dtDate, &stDate);
	stDate.wDay = static_cast<WORD>(1);
	DATE dtNew;
	SystemTimeToVariantTime(&stDate, &dtNew);
	*pdtDate = dtNew;
	return S_OK;
}

HRESULT CDate::GetPreviousDay(DATE dtDate, DATE *pdtDate)
{
	SYSTEMTIME stDate;
	VariantTimeToSystemTime(dtDate, &stDate);
	
	// Adjust day (and month/year)
	if (stDate.wDay == 1)
	{
		stDate.wMonth--;
		if (stDate.wMonth < 1)
		{
			stDate.wMonth = 12;
			stDate.wYear--;
		}

		// Get the last day of the previous month
		DATE dtCheck;
		Int2OLE(static_cast<short>(stDate.wMonth), 1, static_cast<short>(stDate.wYear), &dtCheck);
		DATE dtLast;
		GetLastDay(dtCheck, &dtLast);
		SYSTEMTIME stCheck;
		VariantTimeToSystemTime(dtLast, &stCheck);
		stDate.wDay = stCheck.wDay;
	}
	else
		stDate.wDay--;

	// Assign the new date
	DATE dtNew;
	SystemTimeToVariantTime(&stDate, &dtNew);
	*pdtDate = dtNew;
	return S_OK;
}

HRESULT CDate::GetPreviousWeek(DATE dtDate, DATE *pdtDate)
{
	SYSTEMTIME stDate;
	VariantTimeToSystemTime(dtDate, &stDate);
	
	// Adjust day (and month/year)
	if (stDate.wDay <= 7)
	{
		int nDiff = 7 - static_cast<int>(stDate.wDay);

		stDate.wMonth--;
		if (stDate.wMonth < 1)
		{
			stDate.wMonth = 12;
			stDate.wYear--;
		}

		// Check for month-end exceeded
		DATE dtCheck;
		Int2OLE(static_cast<short>(stDate.wMonth), 1, static_cast<short>(stDate.wYear), &dtCheck);
		DATE dtLast;
		GetLastDay(dtCheck, &dtLast);
		SYSTEMTIME stCheck;
		VariantTimeToSystemTime(dtLast, &stCheck);
		stDate.wDay = stCheck.wDay - nDiff;
	}
	else
		stDate.wDay = stDate.wDay - 7;

	// Assign the new date
	DATE dtNew;
	SystemTimeToVariantTime(&stDate, &dtNew);
	*pdtDate = dtNew;
	return S_OK;
}

HRESULT CDate::GetPreviousMonth(DATE dtDate, DATE *pdtDate)
{
	SYSTEMTIME stDate;
	VariantTimeToSystemTime(dtDate, &stDate);
	
	// Adjust month (and year)
	if (stDate.wMonth == 1)
	{
		stDate.wMonth = 12;
		stDate.wYear--;
	}
	else
		stDate.wMonth--;

	// Check for month-end exceeded
	DATE dtCheck;
	Int2OLE(static_cast<short>(stDate.wMonth), 1, static_cast<short>(stDate.wYear), &dtCheck);
	DATE dtLast;
	GetLastDay(dtCheck, &dtLast);
	SYSTEMTIME stCheck;
	VariantTimeToSystemTime(dtLast, &stCheck);
	if (stDate.wDay > stCheck.wDay)
		stDate.wDay = stCheck.wDay;

	// Assign the new date
	DATE dtNew;
	SystemTimeToVariantTime(&stDate, &dtNew);
	*pdtDate = dtNew;
	return S_OK;
}

HRESULT CDate::GetPreviousYear(DATE dtDate, DATE *pdtDate)
{
	SYSTEMTIME stDate;
	VariantTimeToSystemTime(dtDate, &stDate);
	
	// Adjust the year
	stDate.wYear--;

	// Check for month-end exceeded
	DATE dtCheck;
	Int2OLE(static_cast<short>(stDate.wMonth), 1, static_cast<short>(stDate.wYear), &dtCheck);
	DATE dtLast;
	GetLastDay(dtCheck, &dtLast);
	SYSTEMTIME stCheck;
	VariantTimeToSystemTime(dtLast, &stCheck);
	if (stDate.wDay > stCheck.wDay)
		stDate.wDay = stCheck.wDay;

	// Assign the new date
	DATE dtNew;
	SystemTimeToVariantTime(&stDate, &dtNew);
	*pdtDate = dtNew;
	return S_OK;
}

HRESULT CDate::GetNextMonth(DATE dtDate, DATE *pdtDate)
{
	SYSTEMTIME stDate;
	VariantTimeToSystemTime(dtDate, &stDate);
	
	// Adjust month (and year)
	if (stDate.wMonth == 12)
	{
		stDate.wMonth = 1;
		stDate.wYear++;
	}
	else
		stDate.wMonth++;

	// Check for month-end exceeded
	DATE dtCheck;
	Int2OLE(static_cast<short>(stDate.wMonth), 1, static_cast<short>(stDate.wYear), &dtCheck);
	DATE dtLast;
	GetLastDay(dtCheck, &dtLast);
	SYSTEMTIME stCheck;
	VariantTimeToSystemTime(dtLast, &stCheck);
	if (stDate.wDay > stCheck.wDay)
		stDate.wDay = stCheck.wDay;

	// Assign the new date
	DATE dtNew;
	SystemTimeToVariantTime(&stDate, &dtNew);
	*pdtDate = dtNew;
	return S_OK;
}

HRESULT CDate::GetNextDay(DATE dtDate, DATE *pdtDate)
{
	SYSTEMTIME stDate;
	VariantTimeToSystemTime(dtDate, &stDate);
	DATE dtLastDay;
	GetLastDay(dtDate, &dtLastDay);

	// Increment the day
	stDate.wDay++;
	SYSTEMTIME stLastDay;
	VariantTimeToSystemTime(dtLastDay, &stLastDay);

	// Check if past month-end
	if (stDate.wDay > stLastDay.wDay)
	{
		stDate.wDay = 1;
		stDate.wMonth++;
		if (stDate.wMonth == 13)
		{
			stDate.wMonth = 1;
			stDate.wYear++;
		}
	}

	// Assign the new date
	DATE dtNew;
	SystemTimeToVariantTime(&stDate, &dtNew);
	*pdtDate = dtNew;
	return S_OK;
}

HRESULT CDate::GetNextYear(DATE dtDate, DATE *pdtDate)
{
	SYSTEMTIME stDate;
	VariantTimeToSystemTime(dtDate, &stDate);
	
	// Adjust year
	stDate.wYear++;

	// Check for leap year and adjust day (if necessary)
	if (!IsLeapYear(static_cast<int>(stDate.wYear)))
	{
		if ((stDate.wMonth == 2) && (stDate.wDay == 29))
			stDate.wDay--;
	}

	// Assign the new date
	DATE dtNew;
	SystemTimeToVariantTime(&stDate, &dtNew);
	*pdtDate = dtNew;
	return S_OK;
}

HRESULT CDate::GetNextWeek(DATE dtDate, DATE *pdtDate)
{
	SYSTEMTIME stDate;
	VariantTimeToSystemTime(dtDate, &stDate);
	DATE dtLastDay;
	GetLastDay(dtDate, &dtLastDay);

	// Increment the day by 7
	stDate.wDay = stDate.wDay + 7;
	SYSTEMTIME stLastDay;
	VariantTimeToSystemTime(dtLastDay, &stLastDay);

	// Check if past month-end
	if (stDate.wDay > stLastDay.wDay)
	{
		WORD wDiff = stDate.wDay - stLastDay.wDay;

		stDate.wDay = wDiff;
		stDate.wMonth++;
		if (stDate.wMonth == 13)
		{
			stDate.wMonth = 1;
			stDate.wYear++;
		}
	}

	// Assign the new date
	DATE dtNew;
	SystemTimeToVariantTime(&stDate, &dtNew);
	*pdtDate = dtNew;
	return S_OK;
}

