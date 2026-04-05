!include "MUI2.nsh"
!include "x64.nsh"
!include "LogicLib.nsh"
!include "FileFunc.nsh"
!include "WordFunc.nsh"

!define DLLNAME "libdiscordrpc_plugin.dll"
!define MUI_ICON ".images\vlc_icon.ico"
!define MUI_UNICON ".images\vlc_icon.ico"

Name "Discord Rich Presence for VLC"
OutFile "releases\windows\${VERSION}\vlc-discordrpc-setup_${VERSION}_win_x64.exe"
InstallDir "$PROGRAMFILES64\vlc-discordrpc-plugin"
RequestExecutionLevel admin

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section "Install plugin"
	SetRegView 64
	ReadRegStr $R0 HKLM "Software\VideoLAN\VLC" "InstallDir"

	${If} $R0 == ""
		MessageBox MB_OK|MB_ICONSTOP "VLC does not appear to be installed. Please install it before continuing."
		Abort
	${EndIf}

	check_vlc_running:

	nsExec::ExecToStack 'tasklist /FI "IMAGENAME eq vlc.exe" /NH'
	Pop $0
	Pop $1

	ClearErrors
	${WordFind} "$1" "vlc.exe" "E+1" $2

	IfErrors ProcessNotFound ProcessFound

	ProcessFound:
	MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "VLC is open. Please close it before continuing." /SD IDRETRY IDRETRY retry
	Abort

	retry:
	Goto check_vlc_running

	ProcessNotFound:

	SetOutPath "$INSTDIR"

	File "README.md"
	File "NEWS.txt"
	File "LICENSE"
	File "nsis\vlcrcedit.exe"

	StrCpy $R1 "$R0\plugins\misc"
	SetOutPath "$R1"
	File "releases\windows\${VERSION}\${DLLNAME}"

	DetailPrint "Configuring VLC interface..."
	nsExec::ExecToLog '"$INSTDIR\vlcrcedit.exe" --install'

	WriteUninstaller "$INSTDIR\uninstall.exe"

	!define UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\VLC-DiscordRPC"

	WriteRegStr HKLM "${UNINST_KEY}" "DisplayName" "Discord Rich Presence for VLC"
	WriteRegStr HKLM "${UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegStr HKLM "${UNINST_KEY}" "DisplayIcon" "$INSTDIR\uninstall.exe"
	WriteRegStr HKLM "${UNINST_KEY}" "DisplayVersion" "${VERSION}"
	WriteRegStr HKLM "${UNINST_KEY}" "Publisher" "Zukaritasu"
	WriteRegDWORD HKLM "${UNINST_KEY}" "NoModify" 1
	WriteRegDWORD HKLM "${UNINST_KEY}" "NoRepair" 1

	DetailPrint "Refreshing VLC plugin cache..."
	nsExec::ExecToLog '"$R0\vlc-cache-gen.exe" "$R0\plugins"'
SectionEnd

Section "Uninstall"

	check_vlc_running:
	nsExec::ExecToStack 'tasklist /FI "IMAGENAME eq vlc.exe" /NH'
	Pop $0
	Pop $1

	ClearErrors
	${WordFind} "$1" "vlc.exe" "E+1" $2

	IfErrors ProcessNotFound ProcessFound

	ProcessFound:
	MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "VLC is open. Please close it before continuing." /SD IDRETRY IDRETRY retry
	Abort

	retry:
	Goto check_vlc_running

	ProcessNotFound:

	DetailPrint "Cleaning up VLC configuration..."
	nsExec::ExecToLog '"$INSTDIR\vlcrcedit.exe" --uninstall'

	SetRegView 64
	ReadRegStr $R0 HKLM "Software\VideoLAN\VLC" "InstallDir"
	StrCpy $R1 "$R0\plugins\misc\${DLLNAME}"

	${If} ${FileExists} "$R1"
		Delete "$R1"
		DetailPrint "Removed plugin: $R1"
	${EndIf}

	Delete "$INSTDIR\README.md"
	Delete "$INSTDIR\NEWS.txt"
	Delete "$INSTDIR\LICENSE"
	Delete "$INSTDIR\vlcrcedit.exe"
	Delete "$INSTDIR\uninstall.exe"
	RMDir "$INSTDIR"

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\VLC-DiscordRPC"

	DetailPrint "Refreshing VLC plugin cache..."

	${If} $R0 != ""
		nsExec::ExecToLog '"$R0\vlc-cache-gen.exe" "$R0\plugins"'
	${EndIf}
SectionEnd
