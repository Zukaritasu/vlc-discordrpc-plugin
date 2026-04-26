#ifndef MyAppVersion
  #define MyAppVersion "1.2.2"
#endif

#define MyAppName "Discord Rich Presence for VLC"
#define MyAppPublisher "Zukaritasu"
#define MyAppExeName "vlcrcedit.exe"
#define MyDLLName "libdiscordrpc_plugin.dll"

[Setup]
AppId={{VLC-DiscordRPC-Plugin}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={commonpf64}\vlc-discordrpc-plugin
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=..\LICENSE
OutputDir=..\releases\windows\{#MyAppVersion}
OutputBaseFilename=vlc-discordrpc-plugin_setup_{#MyAppVersion}_win64
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
SetupIconFile=..\.images\vlc_icon.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName={#MyAppName}
AppPublisherURL=https://github.com/Zukaritasu/vlc-discordrpc-plugin
AppSupportURL=https://github.com/Zukaritasu/vlc-discordrpc-plugin/issues
AppUpdatesURL=https://github.com/Zukaritasu/vlc-discordrpc-plugin/releases
AppComments=This is a plugin specifically developed to display the user's activity in VLC on Discord through rich presence
AppContact=zukaritasu@gmail.com
VersionInfoCopyright=Copyright (c) 2026 Zukaritasu. All rights reserved.

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\NEWS.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\inst\vlcrcedit.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\releases\windows\{#MyAppVersion}\{#MyDLLName}"; DestDir: "{code:GetVLCPath}\plugins\misc"; Flags: ignoreversion

[Run]
Filename: "{app}\{#MyAppExeName}"; Parameters: "--install"; StatusMsg: "Configuring VLC interface..."; Flags: runhidden
Filename: "{code:GetVLCPath}\vlc-cache-gen.exe"; Parameters: """{code:GetVLCPath}\plugins"""; StatusMsg: "Refreshing VLC plugin cache..."; Flags: runhidden

[UninstallRun]
Filename: "{app}\{#MyAppExeName}"; Parameters: "--uninstall"; StatusMsg: "Cleaning up VLC configuration..."; Flags: runhidden; RunOnceId: "VlcRpcUninstall"
Filename: "{code:GetVLCPath}\vlc-cache-gen.exe"; Parameters: """{code:GetVLCPath}\plugins"""; StatusMsg: "Refreshing VLC plugin cache..."; Flags: runhidden; RunOnceId: "VlcCacheRefreshUninstall"

[Code]
var
  VLCPath: String;

function GetVLCPath(Param: String): String;
begin
  if VLCPath = '' then
  begin
    if not RegQueryStringValue(HKLM64, 'Software\VideoLAN\VLC', 'InstallDir', VLCPath) then
    begin
      if not RegQueryStringValue(HKLM32, 'Software\VideoLAN\VLC', 'InstallDir', VLCPath) then
      begin
        VLCPath := '';
      end;
    end;
  end;
  Result := VLCPath;
end;

function IsVlcRunning(): Boolean;
var
  WMI, Processes: Variant;
begin
  Result := False;
  try
    WMI := CreateOleObject('WbemScripting.SWbemLocator');
    WMI := WMI.ConnectServer('.', 'root\CIMV2');
    Processes := WMI.ExecQuery('SELECT * FROM Win32_Process WHERE Name = ''vlc.exe''');
    Result := Processes.Count > 0;
  except
    // Fallback or ignore error
  end;
end;

function InitializeSetup(): Boolean;
begin
  Result := True;
  if GetVLCPath('') = '' then
  begin
    MsgBox('VLC does not appear to be installed. Please install it before continuing.', mbCriticalError, MB_OK);
    Result := False;
    Exit;
  end;

  while IsVlcRunning() do
  begin
    if MsgBox('VLC is open. Please close it before continuing.', mbConfirmation, MB_RETRYCANCEL) <> IDRETRY then
    begin
      Result := False;
      Exit;
    end;
  end;
end;

function InitializeUninstall(): Boolean;
begin
  Result := True;
  while IsVlcRunning() do
  begin
    if MsgBox('VLC is open. Please close it before continuing.', mbConfirmation, MB_RETRYCANCEL) <> IDRETRY then
    begin
      Result := False;
      Exit;
    end;
  end;
end;
