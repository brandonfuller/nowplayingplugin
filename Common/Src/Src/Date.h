// Date.h : Declaration of the CDate
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

#ifndef __DATE_H_
#define __DATE_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CDate
class  CDate
{
public:
	CDate()
	{
	}

public:
	HRESULT GetPreviousYear(/*[in]*/ DATE dtDate, /*[out]*/ DATE* pdtDate);
	HRESULT GetPreviousWeek(/*[in]*/ DATE dtDate, /*[out]*/ DATE* pdtDate);
	HRESULT GetPreviousDay(/*[in]*/ DATE dtDate, /*[out]*/ DATE* pdtDate);
	HRESULT GetNextWeek(/*[in]*/ DATE dtDate, /*[out]*/ DATE* pdtDate);
	HRESULT GetNextYear(/*[in]*/ DATE dtDate, /*[out]*/ DATE* pdtDate);
	HRESULT GetNextDay(/*[in]*/ DATE dtDate, /*[out]*/ DATE* pdtDate);
	HRESULT GetNextMonth(/*[in]*/ DATE dtDate, /*[out]*/ DATE* pdtDate);
	HRESULT GetPreviousMonth(/*[in]*/ DATE dtDate, /*[out]*/ DATE* pdtDate);
	HRESULT GetFirstDay(/*[in]*/ DATE dtDate, /*[out]*/ DATE* pdtDate);
	HRESULT GetLastDay(/*[in]*/ DATE dtDate, /*[out]*/ DATE* pdtDate);
	HRESULT Int2OLE(/*[in]*/ short nMonth, /*[in]*/ short nDay, /*[in]*/ short nYear, /*[out]*/ DATE* pdtDate);
	HRESULT Now(/*[out]*/ DATE* pdtDate);

protected:
	BOOL IsLeapYear(const int &nYear);
};

#endif //__DATE_H_
