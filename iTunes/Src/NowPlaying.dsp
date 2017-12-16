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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NowPlaying_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I "..\..\Common\Src" /I "..\..\Common\Lib" /I "..\..\Common\Include" /I "..\..\Common\Include\iTunesVisualAPIW32\\" /I "..\..\Common\Include\iTunesCOMWindowsSDK" /I "..\..\Common\Include\gdwin32" /D "NDEBUG" /D "TARGET_OS_WIN32" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NOWPLAYING_EXPORTS" /D "PODCAST" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "." /i "..\..\Common\Src" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib comctl32.lib version.lib shlwapi.lib bgd.lib /nologo /dll /pdb:none /map /machine:I386 /libpath:"..\..\Common\Lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Build Installer
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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NowPlaying_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "." /I "..\..\Common\Src" /I "..\..\Common\Lib" /I "..\..\Common\Include" /I "..\..\Common\Include\iTunesVisualAPIW32\\" /I "..\..\Common\Include\iTunesCOMWindowsSDK" /I "..\..\Common\Include\gdwin32" /D "_DEBUG" /D "TARGET_OS_WIN32" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NOWPLAYING_EXPORTS" /D "PODCAST" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "." /i "..\..\Common\Src" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wininet.lib comctl32.lib version.lib shlwapi.lib bgd.lib /nologo /dll /map /debug /machine:I386 /pdbtype:sept /libpath:"..\..\Common\Lib"

!ENDIF 

# Begin Target

# Name "NowPlaying - Win32 Release"
# Name "NowPlaying - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\Common\Src\AmLog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\Date.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\Encrypt.c
# End Source File
# Begin Source File

SOURCE=..\..\Common\Include\iTunesVisualAPIW32\iTunesAPI.c
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\License.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\MD5.c
# End Source File
# Begin Source File

SOURCE=.\NowPlaying.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\NowPlaying.rc
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\Utilities.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Common\Src\AmLog.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\Date.h
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

SOURCE=..\..\Common\Src\resource.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\strsafe.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Src\Utilities.h
# End Source File
# End Group
# Begin Group "Install Files"

# PROP Default_Filter "nsi"
# Begin Source File

SOURCE=.\EULA.txt
# End Source File
# Begin Source File

SOURCE=.\NowPlaying.nsi
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "rgs"
# Begin Source File

SOURCE=.\NowPlaying.rgs
# End Source File
# End Group
# End Target
# End Project
