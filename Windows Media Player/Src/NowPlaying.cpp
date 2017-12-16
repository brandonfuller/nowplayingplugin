/////////////////////////////////////////////////////////////////////////////
//
// NowPlaying.cpp : Implementation of CNowPlaying
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Defines.h"
#include "strsafe.h"
#include "iTunesAPI.h"
#include "Utilities.h"
#include "NowPlaying.h"

CComPtr<IWMPCore> g_spCore = NULL;

/////////////////////////////////////////////////////////////////////////////
// CNowPlaying::CNowPlaying
// Constructor

CNowPlaying::CNowPlaying()
{
    lstrcpyn(m_szPluginText, _T("Now Playing Plugin"), sizeof(m_szPluginText) / sizeof(m_szPluginText[0]));
    m_dwAdviseCookie = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CNowPlaying::~CNowPlaying
// Destructor

CNowPlaying::~CNowPlaying()
{	
}

/////////////////////////////////////////////////////////////////////////////
// CNowPlaying:::FinalConstruct
// Called when an plugin is first loaded. Use this function to do one-time
// intializations that could fail instead of doing this in the constructor,
// which cannot return an error.

HRESULT CNowPlaying::FinalConstruct()
{
	HKEY hKey;
	TCHAR szPath[1024];
	DWORD dwBufLen = sizeof( szPath );

	if ( RegOpenKeyEx( HKEY_CLASSES_ROOT, "CLSID\\{5098F2B5-7326-42dd-86C7-DB99EEE4AA34}\\InprocServer32", 0, KEY_QUERY_VALUE, &hKey ) == ERROR_SUCCESS )
	{
		if ( RegQueryValueEx( hKey, NULL, NULL, NULL, (LPBYTE) szPath, &dwBufLen ) != ERROR_SUCCESS )
		{
			RegCloseKey( hKey );
			return E_FAIL;
		}

		RegCloseKey( hKey );
	}
	else
	{
		return E_FAIL;
	}

	if ( DoInit( MEDIA_PLAYER_WMP, szPath ) )
	{
		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNowPlaying:::FinalRelease
// Called when a plugin is unloaded. Use this function to free any
// resources allocated in FinalConstruct.

void CNowPlaying::FinalRelease()
{
	DoCleanup();

    ReleaseCore();
}

/////////////////////////////////////////////////////////////////////////////
// CNowPlaying::SetCore
// Set WMP core interface

HRESULT CNowPlaying::SetCore(IWMPCore *pCore)
{
    HRESULT hr = S_OK;

    // release any existing WMP core interfaces
    ReleaseCore();

    // If we get passed a NULL core, this  means
    // that the plugin is being shutdown.

    if (pCore == NULL)
    {
        return S_OK;
    }

    m_spCore = pCore;
	g_spCore = pCore;

    // connect up the event interface
    CComPtr<IConnectionPointContainer>  spConnectionContainer;

    hr = m_spCore->QueryInterface( &spConnectionContainer );

    if (SUCCEEDED(hr))
    {
        hr = spConnectionContainer->FindConnectionPoint( __uuidof(IWMPEvents), &m_spConnectionPoint );
    }

    if (SUCCEEDED(hr))
    {
        hr = m_spConnectionPoint->Advise( GetUnknown(), &m_dwAdviseCookie );

        if ((FAILED(hr)) || (0 == m_dwAdviseCookie))
        {
            m_spConnectionPoint = NULL;
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CNowPlaying::ReleaseCore
// Release WMP core interfaces

void CNowPlaying::ReleaseCore()
{
    if (m_spConnectionPoint)
    {
        if (0 != m_dwAdviseCookie)
        {
            m_spConnectionPoint->Unadvise(m_dwAdviseCookie);
            m_dwAdviseCookie = 0;
        }
        m_spConnectionPoint = NULL;
    }

    if ( m_spCore )
    {
        m_spCore = NULL;
    }

	if ( g_spCore )
	{
		g_spCore = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CNowPlaying::DisplayPropertyPage
// Display property page for plugin

HRESULT CNowPlaying::DisplayPropertyPage(HWND hwndParent)
{
	DoPropertySheet( hwndParent );

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CNowPlaying::GetProperty
// Get plugin property

HRESULT CNowPlaying::GetProperty(const WCHAR *pwszName, VARIANT *pvarProperty)
{
    if (NULL == pvarProperty)
    {
        return E_POINTER;
    }

    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CNowPlaying::SetProperty
// Set plugin property

HRESULT CNowPlaying::SetProperty(const WCHAR *pwszName, const VARIANT *pvarProperty)
{
    return E_NOTIMPL;
}

bool ExportArtwork(LPTSTR lpszFileLocal, size_t cbFileLocal, LPTSTR lpszFileRemote, size_t cbFileRemote)
{
	return false;
}
