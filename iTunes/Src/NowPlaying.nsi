;--------------------------------
; NowPlaying.nsi
;--------------------------------

!include "MUI.nsh"
!include "UpgradeDLL.nsh"
!include "UpgradeDLL2.nsh"

;
; The name of the installer
;

Name "Now Playing for iTunes"

;
; Version
;

!define VERSION_NUMBER "3.6.4.1"

;
; License file
;

LicenseData EULA.txt

;
; The file to write
;

OutFile "..\NowPlaying-iTunes.exe"

;
; The default installation directory
;

InstallDirRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\iTunes.exe" ""

;
; Uninstall
;

!define UNINSTALL_GUID "{2FBF04DC-404C-4FA4-BA28-99903080D2C4}"
!define UNINSTALL_EXE "NowPlaying-Uninstall.exe"

;
; Modern UI Options;
;

!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_NOAUTOCLOSE

;
; Pages
;

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE EULA.txt
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

;
; Languages
;
 
!insertmacro MUI_LANGUAGE "English"

;
; Version Information
;

VIProductVersion "${VERSION_NUMBER}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "Now Playing"
VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" "Kick Ass!"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "Brandon Fuller"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks" "iTunes is a registered trademark of Apple Computer, Inc."
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "Copyright � 2004-2008 Brandon Fuller"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "Now Playing is an iTunes Plugin"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${VERSION_NUMBER}"

;
; The stuff to install
;

Section "" ; No components page, name is not important

	;
	; Create the plug-in directory
	;

	IfFileExists "$INSTDIR\Plug-ins\Now Playing\*.*" InstallDirExists
	CreateDirectory "$INSTDIR\Plug-ins\Now Playing"
	InstallDirExists:

	;
	; Copy my program files
	;

	!define UPGRADEDLL_NOREGISTER
		!insertmacro UpgradeDLL "Release\NowPlaying.dll" "$INSTDIR\Plug-ins\NowPlaying.dll" "$INSTDIR\Plug-ins"
	!undef UPGRADEDLL_NOREGISTER

	;
	; Copy SFTP stuff
	;

	!insertmacro UpgradeDLL "..\..\Common\Lib\wodSFTP.dll" "$SYSDIR\wodSFTP.dll" "$SYSDIR"
	!insertmacro UpgradeDLL "..\..\Common\Lib\wodSFTP.ocx" "$SYSDIR\wodSFTP.ocx" "$SYSDIR"

	;
	; Copy XML stuff
	;

	!insertmacro UpgradeDLL "..\..\Common\Lib\msxml3.dll" "$SYSDIR\msxml3.dll" "$SYSDIR"
	!define UPGRADEDLL_NOREGISTER
		!insertmacro UpgradeDLL "..\..\Common\Lib\msxml3r.dll" "$SYSDIR\msxml3r.dll" "$SYSDIR"
		!insertmacro UpgradeDLL "..\..\Common\Lib\msxml3a.dll" "$SYSDIR\msxml3a.dll" "$SYSDIR"
	!undef UPGRADEDLL_NOREGISTER

	;
	; Copy GD stuff
	;

	!define UPGRADEDLL_NOREGISTER
		!insertmacro UpgradeDLL2 "..\..\Common\Include\gdwin32\bgd.dll" "$SYSDIR\bgd.dll" "$SYSDIR"
	!undef UPGRADEDLL_NOREGISTER

	;
	; Only increase DLL count on new installation
	;

	IfFileExists "$INSTDIR\Plug-ins\Now Playing\${UNINSTALL_EXE}" skipAddSharedDLL

	Push "$SYSDIR\wodSFTP.dll"
	Call AddSharedDLL
	Push "$SYSDIR\wodSFTP.ocx"
	Call AddSharedDLL
	Push "$SYSDIR\msxml3.dll"
	Call AddSharedDLL
	Push "$SYSDIR\msxml3r.dll"
	Call AddSharedDLL
	Push "$SYSDIR\msxml3a.dll"
	Call AddSharedDLL
	Push "$SYSDIR\bgd.dll"
	Call AddSharedDLL

	skipAddSharedDLL:

	;
	; Write the uninstall keys for Windows
	;

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTALL_GUID}" "DisplayName" "Now Playing: An iTunes for Windows Plugin"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTALL_GUID}" "UninstallString" '"$INSTDIR\Plug-ins\Now Playing\${UNINSTALL_EXE}"'
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTALL_GUID}" "Publisher" "Brandon Fuller"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTALL_GUID}" "URLInfoAbout" "http://brandon.fuller.name/"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTALL_GUID}" "URLUpdateInfo" "http://brandon.fuller.name/archives/hacks/nowplaying/"	
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTALL_GUID}" "HelpLink" "http://brandon.fuller.name/archives/hacks/nowplaying/"	
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTALL_GUID}" "DisplayVersion" "${VERSION_NUMBER}"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTALL_GUID}" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTALL_GUID}" "NoRepair" 1
	WriteUninstaller "$INSTDIR\Plug-ins\Now Playing\${UNINSTALL_EXE}"

	;
	; Cleanup old stuff
	;

	Delete /REBOOTOK "$INSTDIR\Plug-ins\${UNINSTALL_EXE}"
	Delete /REBOOTOK "$INSTDIR\Plug-ins\NowPlaying-Log.txt"
	Delete /REBOOTOK "$INSTDIR\Plug-ins\now_playing.xml"
	Delete /REBOOTOK "$INSTDIR\Plug-ins\Now Playing\*.pyd"
	Delete /REBOOTOK "$INSTDIR\Plug-ins\Now Playing\bittorrent.ico"
	Delete /REBOOTOK "$INSTDIR\Plug-ins\Now Playing\library.zip"
	Delete /REBOOTOK "$INSTDIR\Plug-ins\Now Playing\NowPlaying-BitTorrent.exe"
	Delete /REBOOTOK "$INSTDIR\Plug-ins\Now Playing\python23.dll"
	Delete /REBOOTOK "$INSTDIR\Plug-ins\Now Playing\w9xpopen.exe"
 
SectionEnd ; end the section

Function .onInit
	
	; CheckPath:
		IfFileExists "$INSTDIR\iTunes.exe" CheckPath_Success
		MessageBox MB_ICONSTOP "iTunes is not installed on this computer. Please install iTunes and then run this installation again."
		Abort
		CheckPath_Success:

	CheckProcess:
		Processes::FindProcess "iTunes.exe"
		Pop $R0
		StrCmp $R0 "0" CheckProcess_Success
		MessageBox MB_RETRYCANCEL "iTunes is currently running on this computer. Please stop iTunes and then continue." IDRETRY CheckProcess
		Abort
		CheckProcess_Success:

FunctionEnd

Function .onInstSuccess

	IfRebootFlag RebootNeeded RebootNotNeeded

	RebootNeeded:
		MessageBox MB_ICONEXCLAMATION "Some files could not be copied because they were in use.  You will need to reboot to complete the installation."
	
	RebootNotNeeded:

FunctionEnd

;
; Uninstaller
;

Section "Uninstall"

	;
	; Decrement the shared DLL count but never remove the files, because the shared DLL count is not reliable enough for such important files.
	;

	Push "$SYSDIR\wodSFTP.dll"
	Call un.DecrementSharedDLL
	Push "$SYSDIR\wodSFTP.ocx"
	Call un.DecrementSharedDLL
	Push "$SYSDIR\msxml3.dll"
	Call un.DecrementSharedDLL
	Push "$SYSDIR\msxml3r.dll"
	Call un.DecrementSharedDLL
	Push "$SYSDIR\msxml3a.dll"
	Push "$SYSDIR\bgd.dll"
	Call un.DecrementSharedDLL
  
	;
	; Remove registry keys
	;

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${UNINSTALL_GUID}"
	DeleteRegKey HKCU "Software\Orion Software Development\Now Playing"
	DeleteRegKey /ifempty HKCU "Software\Orion Software Development"

	;
	; Remove files and uninstaller. $INSTDIR is where the plugin is.
	;

	Delete /REBOOTOK "$INSTDIR\..\NowPlaying.dll"
	Delete /REBOOTOK "$INSTDIR\*.*"
	RMDir /REBOOTOK "$INSTDIR"	

SectionEnd

Function AddSharedDLL

	Exch $R1
	Push $R0
	ReadRegDword $R0 HKLM Software\Microsoft\Windows\CurrentVersion\SharedDLLs $R1
    IntOp $R0 $R0 + 1
	WriteRegDWORD HKLM Software\Microsoft\Windows\CurrentVersion\SharedDLLs $R1 $R0
	Pop $R0
	Pop $R1

FunctionEnd

Function un.DecrementSharedDLL

	Exch $R1
	Push $R0
	ReadRegDword $R0 HKLM Software\Microsoft\Windows\CurrentVersion\SharedDLLs $R1
	StrCmp $R0 "" done
	IntOp $R0 $R0 - 1
	IntCmp $R0 0 rk rk uk
    
	rk:
		DeleteRegValue HKLM Software\Microsoft\Windows\CurrentVersion\SharedDLLs $R1
		Goto done
    
	uk:
		WriteRegDWORD HKLM Software\Microsoft\Windows\CurrentVersion\SharedDLLs $R1 $R0
		Goto done
	
	done:
		Pop $R0
		Pop $R1

FunctionEnd
