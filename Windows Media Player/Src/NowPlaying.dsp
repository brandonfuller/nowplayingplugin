# Microsoft Developer Studio Project File - Name="NowPlaying" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=NowPlaying - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "NowPlaying.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NowPlaying.mak" CFG="NowPlaying - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NowPlaying - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "NowPlaying - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "Perforce Project"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "NowPlaying - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I "..\..\Common\Src" /I "..\..\Common\Lib" /I "..\..\Common\Include" /I "..\..\Common\Include\iTunesVisualAPIW32" /I "..\..\Common\Include\gdwin32" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "." /i "..\..\Common\Src" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib wininet.lib version.lib comctl32.lib shlwapi.lib bgd.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\Common\Lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=c:\orion\tools\nsis\makensis.exe NowPlaying.nsi
# End Special Build Tool

!ELSEIF  "$(CFG)" == "NowPlaying - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "." /I "..\..\Common\Src" /I "..\..\Common\Lib" /I "..\..\Common\Include" /I "..\..\Common\Include\iTunesVisualAPIW32" /I "..\..\Common\Include\gdwin32" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "." /i "..\..\Common\Src" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib wininet.lib version.lib comctl32.lib shlwapi.lib bgd.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\Common\Lib"
# Begin Custom Build - Registering "$(TargetPath)"
OutDir=.\Debug
TargetPath=.\Debug\NowPlaying.dll
InputPath=.\Debug\NowPlaying.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "NowPlaying - Win32 Release"
# Name "NowPlaying - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\Common\Src\AmLog.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\Encrypt.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\License.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\MD5.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\NowPlaying.cpp
# End Source File
# Begin Source File

SOURCE=.\NowPlaying.def
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\NowPlaying.rc
# End Source File
# Begin Source File

SOURCE=.\NowPlayingdll.cpp
# End Source File
# Begin Source File

SOURCE=.\NowPlayingEvents.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\Utilities.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\Common\Src\AmLog.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\Defines.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\Encrypt.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\Global.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Include\iTunesVisualAPIW32\iTunesAPI.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Include\iTunesVisualAPIW32\iTunesVisualAPI.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\License.h
# End Source File
# Begin Source File

SOURCE=.\LocalDefines.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\MD5.h
# End Source File
# Begin Source File

SOURCE=.\NowPlaying.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\strsafe.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\Utilities.h
# End Source File
# Begin Source File

SOURCE=.\wmp.h
# End Source File
# Begin Source File

SOURCE=.\wmpplug.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\NowPlaying.rgs
# End Source File
# End Group
# Begin Group "Install Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\EULA.txt
# End Source File
# Begin Source File

SOURCE=.\NowPlaying.nsi
# End Source File
# End Group
# End Target
# End Project
