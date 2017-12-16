/////////////////////////////////////////////////////////////////////////////
//
// NowPlayingEvents.cpp : Implementation of CNowPlaying events
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <time.h>
#include "strsafe.h"
#include "Defines.h"
#include "iTunesAPI.h"
#include "Utilities.h"
#include "AmLog.h"
#include "NowPlaying.h"

void CNowPlaying::DoTrackChange()
{
	HRESULT hr = S_OK;
	CPlaylistItem x;
	CComPtr<IWMPMedia> spMedia = NULL;

	ZeroMemory( &x, sizeof( x ) );

	x.m_trackInfo.numTracks = PROP_UNSUPPORTED;
	x.m_trackInfo.discNumber = PROP_UNSUPPORTED;
	x.m_trackInfo.numDiscs = PROP_UNSUPPORTED;

	USES_CONVERSION;

	if ( m_spCore )
	{
		hr = m_spCore->get_currentMedia( &spMedia );
		if ( SUCCEEDED( hr ) )
		{		
			CComBSTR bstrName;
			CComBSTR bstrValue;	

			//
			// Album
			//

			bstrName = "WM/AlbumTitle";
			hr = spMedia->getItemInfo( bstrName, &bstrValue );			
			if ( SUCCEEDED( hr ) )
			{
				x.m_trackInfo.album[0] = bstrValue.Length();
				StringCbCopyW( (LPWSTR) &x.m_trackInfo.album[1], sizeof( x.m_trackInfo.album ) - 1, OLE2W( bstrValue ) );
			}

			//
			// Title
			//

			bstrName = "Title";
			hr = spMedia->getItemInfo( bstrName, &bstrValue );			
			if ( SUCCEEDED( hr ) )
			{
				x.m_trackInfo.name[0] = bstrValue.Length();
				StringCbCopyW( (LPWSTR) &x.m_trackInfo.name[1], sizeof( x.m_trackInfo.name ) - 1, OLE2W( bstrValue ) );
			}

			//
			// Artist
			//

			bstrName = "Author";
			hr = spMedia->getItemInfo( bstrName, &bstrValue );			
			if ( SUCCEEDED( hr ) )
			{
				x.m_trackInfo.artist[0] = bstrValue.Length();
				StringCbCopyW( (LPWSTR) &x.m_trackInfo.artist[1], sizeof( x.m_trackInfo.artist ) - 1, OLE2W( bstrValue ) );
			}
			
			//
			// Comment
			//

			bstrName = "Comment";
			hr = spMedia->getItemInfo( bstrName, &bstrValue );			
			if ( SUCCEEDED( hr ) )
			{
				x.m_trackInfo.comments[0] = bstrValue.Length();
				StringCbCopyW( (LPWSTR) &x.m_trackInfo.comments[1], sizeof( x.m_trackInfo.comments ) - 1, OLE2W( bstrValue ) );
			}

			//
			// Composer 
			//

			bstrName = "WM/Composer";
			hr = spMedia->getItemInfo( bstrName, &bstrValue );
			if ( SUCCEEDED( hr ) )
			{
				x.m_trackInfo.composer[0] = bstrValue.Length();
				StringCbCopyW( (LPWSTR) &x.m_trackInfo.composer[1], sizeof( x.m_trackInfo.composer ) - 1, OLE2W( bstrValue ) );
			}

			//
			// Genre
			//

			bstrName = "WM/Genre";
			hr = spMedia->getItemInfo( bstrName, &bstrValue );
			if ( SUCCEEDED( hr ) )
			{
				x.m_trackInfo.genre[0] = bstrValue.Length();
				StringCbCopyW( (LPWSTR) &x.m_trackInfo.genre[1], sizeof( x.m_trackInfo.genre ) - 1, OLE2W( bstrValue ) );
			}

			//
			// Grouping
			//

			bstrName = "WM/ContentGroupDescription";
			hr = spMedia->getItemInfo( bstrName, &bstrValue );			
			if ( SUCCEEDED( hr ) )
			{
				TRACE( "Grouping = %s", OLE2T( bstrValue ) );
				x.m_trackInfo.grouping[0] = bstrValue.Length();
				StringCbCopyW( (LPWSTR) &x.m_trackInfo.grouping[1], sizeof( x.m_trackInfo.grouping ) - 1, OLE2W( bstrValue ) );
			}

			//
			// Kind
			//

			bstrName = "FileType";
			hr = spMedia->getItemInfo( bstrName, &bstrValue );
			if ( SUCCEEDED( hr ) )
			{
				x.m_trackInfo.kind[0] = bstrValue.Length();
				StringCbCopyW( (LPWSTR) &x.m_trackInfo.kind[1], sizeof( x.m_trackInfo.kind ) - 1, OLE2W( bstrValue ) );
			}

			//
			// Duration
			//

			bstrName = "Duration";
			hr = spMedia->getItemInfo( bstrName, &bstrValue );			
			if ( SUCCEEDED( hr ) )
			{
				float flTemp = 0;
				sscanf( OLE2T( bstrValue ), "%f", &flTemp );
				x.m_trackInfo.totalTimeInMS = (int) flTemp * 1000;
			}
			
			//
			// Bitrate
			//

			bstrName = "Bitrate";
			hr = spMedia->getItemInfo( bstrName, &bstrValue );			
			if ( SUCCEEDED( hr ) )
			{
				int intTemp = 0;
				sscanf( OLE2T( bstrValue ), "%d", &intTemp );
				x.m_trackInfo.bitRate = intTemp / 1000;
			}

			//
			// FileSize 
			//

			bstrName = "FileSize";
			hr = spMedia->getItemInfo( bstrName, &bstrValue );			
			if ( SUCCEEDED( hr ) )
			{
				int intTemp = 0;
				sscanf( OLE2T( bstrValue ), "%d", &intTemp );
				x.m_trackInfo.sizeInBytes = intTemp;
			}

			//
			// Year 
			//

			bstrName = "WM/Year";
			hr = spMedia->getItemInfo( bstrName, &bstrValue );			
			if ( SUCCEEDED( hr ) )
			{
				int intTemp = 0;
				sscanf( OLE2T( bstrValue ), "%d", &intTemp );
				x.m_trackInfo.year = intTemp;
			}

			//
			// Rating
			//

			bstrName = "UserRating";
			hr = spMedia->getItemInfo( bstrName, &bstrValue );			
			if ( SUCCEEDED( hr ) )
			{
				int intTemp = 0;
				sscanf( OLE2T( bstrValue ), "%d", &intTemp );

				switch ( intTemp )
				{
					case 0:		// Unrated
						x.m_trackInfo.userRating = 0;
						break;
					case 1:		// 1 star
						x.m_trackInfo.userRating = 20;
						break;
					case 25:	// 2 stars
						x.m_trackInfo.userRating = 40;
						break;
					case 50:	// 3 stars
						x.m_trackInfo.userRating = 60;
						break;
					case 75:	// 4 stars
						x.m_trackInfo.userRating = 80;
						break;
					case 99:	// 5 stars
						x.m_trackInfo.userRating = 100;
						break;
					default:
						x.m_trackInfo.userRating = 0;
						break;
				}
			}

			//
			// TrackNumber 
			//

			bstrName = "WM/TrackNumber";
			hr = spMedia->getItemInfo( bstrName, &bstrValue );			
			if ( SUCCEEDED( hr ) )
			{
				int intTemp = 0;
				sscanf( OLE2T( bstrValue ), "%d", &intTemp );
				x.m_trackInfo.trackNumber = intTemp;
			}

			//
			// UserPlayCount 
			//

			bstrName = "UserPlayCount";
			hr = spMedia->getItemInfo( bstrName, &bstrValue );			
			if ( SUCCEEDED( hr ) )
			{
				int intTemp = 0;
				sscanf( OLE2T( bstrValue ), "%d", &intTemp );
				x.m_trackInfo.playCount = intTemp;
			}	

			//
			// Source URL
			//

			hr = spMedia->get_sourceURL( &bstrValue );
			if ( SUCCEEDED( hr ) && _strnicmp( OLE2T( bstrValue ), "http://", 7 ) == 0 )
			{
				StringCbCopyA( &x.m_szUrlSource[0], sizeof( x.m_szUrlSource ), OLE2T( bstrValue ) );
				TRACE( "Source URL = %s", OLE2T( bstrValue ) );
			}			

			//
			// Send it off
			//

			DoPlay( &x );
		}
	}
}

void CNowPlaying::OpenStateChange( long NewState )
{
    switch ( NewState )
    {
		case wmposUndefined:
			break;
		case wmposPlaylistChanging:
			break;
		case wmposPlaylistLocating:
			break;
		case wmposPlaylistConnecting:
			break;
		case wmposPlaylistLoading:
			break;
		case wmposPlaylistOpening:
			break;
		case wmposPlaylistOpenNoMedia:
			break;
		case wmposPlaylistChanged:
			break;
		case wmposMediaChanging:
			break;
		case wmposMediaLocating:
			break;
		case wmposMediaConnecting:
			break;
		case wmposMediaLoading:
			break;
		case wmposMediaOpening:
			break;
		case wmposMediaOpen:
			break;
		case wmposBeginCodecAcquisition:
			break;
		case wmposEndCodecAcquisition:
			break;
		case wmposBeginLicenseAcquisition:
			break;
		case wmposEndLicenseAcquisition:
			break;
		case wmposBeginIndividualization:
			break;
		case wmposEndIndividualization:
			break;
		case wmposMediaWaiting:
			break;
		case wmposOpeningUnknownURL:
			break;
		default:
			break;
    }
}

void CNowPlaying::PlayStateChange( long NewState )
{
	TRACE( "PlayStateChange = %d", NewState );

    switch ( NewState )
    {
		case wmppsUndefined:
			break;
		case wmppsStopped:
			DoStop();
			break;
		case wmppsPaused:
			break;
		case wmppsPlaying:
			DoTrackChange();
			break;
		case wmppsScanForward:
			break;
		case wmppsScanReverse:
			break;
		case wmppsBuffering:
			break;
		case wmppsWaiting:
			break;
		case wmppsMediaEnded:
			break;
		case wmppsTransitioning:
			break;
		case wmppsReady:
			break;
		case wmppsReconnecting:
			break;
		case wmppsLast:
			break;
		default:
			break;
    }
}

void CNowPlaying::AudioLanguageChange( long LangID )
{
}

void CNowPlaying::StatusChange()
{
}

void CNowPlaying::ScriptCommand( BSTR scType, BSTR Param )
{
}

void CNowPlaying::NewStream()
{
}

void CNowPlaying::Disconnect( long Result )
{
}

void CNowPlaying::Buffering( VARIANT_BOOL Start )
{
}

void CNowPlaying::Error()
{
    CComPtr<IWMPError>      spError;
    CComPtr<IWMPErrorItem>  spErrorItem;
    HRESULT                 dwError = S_OK;
    HRESULT                 hr = S_OK;

    if (m_spCore)
    {
        hr = m_spCore->get_error(&spError);

        if (SUCCEEDED(hr))
        {
            hr = spError->get_item(0, &spErrorItem);
        }

        if (SUCCEEDED(hr))
        {
            hr = spErrorItem->get_errorCode( (long *) &dwError );
        }
    }
}

void CNowPlaying::Warning( long WarningType, long Param, BSTR Description )
{
}

void CNowPlaying::EndOfStream( long Result )
{
}

void CNowPlaying::PositionChange( double oldPosition, double newPosition)
{
}

void CNowPlaying::MarkerHit( long MarkerNum )
{
}

void CNowPlaying::DurationUnitChange( long NewDurationUnit )
{
}

void CNowPlaying::CdromMediaChange( long CdromNum )
{
}

void CNowPlaying::PlaylistChange( IDispatch * Playlist, WMPPlaylistChangeEventType change )
{
    switch (change)
    {
		case wmplcUnknown:
			break;
		case wmplcClear:
			break;
		case wmplcInfoChange:
			break;
		case wmplcMove:
			break;
		case wmplcDelete:
			break;
		case wmplcInsert:
			break;
		case wmplcAppend:
			break;
		case wmplcPrivate:
			break;
		case wmplcNameChange:
			break;
		case wmplcMorph:
			break;
		case wmplcSort:
			break;
		case wmplcLast:
			break;
		default:
			break;
    }
}

void CNowPlaying::CurrentPlaylistChange( WMPPlaylistChangeEventType change )
{
    switch (change)
    {
    case wmplcUnknown:
        break;
	case wmplcClear:
        break;
	case wmplcInfoChange:
        break;
	case wmplcMove:
        break;
	case wmplcDelete:
        break;
	case wmplcInsert:
        break;
	case wmplcAppend:
        break;
	case wmplcPrivate:
        break;
	case wmplcNameChange:
        break;
	case wmplcMorph:
        break;
	case wmplcSort:
        break;
	case wmplcLast:
        break;
    default:
        break;
    }
}

void CNowPlaying::CurrentPlaylistItemAvailable( BSTR bstrItemName )
{
}

void CNowPlaying::MediaChange( IDispatch * Item )
{
}

void CNowPlaying::CurrentMediaItemAvailable( BSTR bstrItemName )
{
}

void CNowPlaying::CurrentItemChange( IDispatch *pdispMedia)
{
}

void CNowPlaying::MediaCollectionChange()
{
}

void CNowPlaying::MediaCollectionAttributeStringAdded( BSTR bstrAttribName,  BSTR bstrAttribVal )
{
}

void CNowPlaying::MediaCollectionAttributeStringRemoved( BSTR bstrAttribName,  BSTR bstrAttribVal )
{
}

void CNowPlaying::MediaCollectionAttributeStringChanged( BSTR bstrAttribName, BSTR bstrOldAttribVal, BSTR bstrNewAttribVal)
{
}

void CNowPlaying::PlaylistCollectionChange()
{
}

void CNowPlaying::PlaylistCollectionPlaylistAdded( BSTR bstrPlaylistName)
{
}

void CNowPlaying::PlaylistCollectionPlaylistRemoved( BSTR bstrPlaylistName)
{
}

void CNowPlaying::PlaylistCollectionPlaylistSetAsDeleted( BSTR bstrPlaylistName, VARIANT_BOOL varfIsDeleted)
{
}

void CNowPlaying::ModeChange( BSTR ModeName, VARIANT_BOOL NewValue)
{
}

void CNowPlaying::MediaError( IDispatch * pMediaObject)
{
}

void CNowPlaying::OpenPlaylistSwitch( IDispatch *pItem )
{
}

void CNowPlaying::DomainChange( BSTR strDomain)
{
}

void CNowPlaying::SwitchedToPlayerApplication()
{
}

void CNowPlaying::SwitchedToControl()
{
}

void CNowPlaying::PlayerDockedStateChange()
{
}

void CNowPlaying::PlayerReconnect()
{
}

void CNowPlaying::Click( short nButton, short nShiftState, long fX, long fY )
{
}

void CNowPlaying::DoubleClick( short nButton, short nShiftState, long fX, long fY )
{
}

void CNowPlaying::KeyDown( short nKeyCode, short nShiftState )
{
}

void CNowPlaying::KeyPress( short nKeyAscii )
{
}

void CNowPlaying::KeyUp( short nKeyCode, short nShiftState )
{
}

void CNowPlaying::MouseDown( short nButton, short nShiftState, long fX, long fY )
{
}

void CNowPlaying::MouseMove( short nButton, short nShiftState, long fX, long fY )
{
}

void CNowPlaying::MouseUp( short nButton, short nShiftState, long fX, long fY )
{
}
