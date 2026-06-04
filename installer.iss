; PlainMD Installer
; Inno Setup Script for PlainMD

[Setup]
AppName=PlainMD
AppVersion=1.4.2
AppVerName=PlainMD 1.4.2
VersionInfoVersion=1.4.2.0
VersionInfoTextVersion=1.4.2
AppCopyright=Copyright (C) 2026 BrainByteZ
AppPublisher=BrainByteZ
DefaultDirName={autopf64}\PlainMD
DefaultGroupName=PlainMD
OutputDir=dist
OutputBaseFilename=plainmd-1.4.2-x64-setup
SetupIconFile=icon.ico
UninstallDisplayIcon={app}\plainmd.exe
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: "fileassoc"; Description: "Associate PlainMD with Markdown files"; GroupDescription: "Other tasks:"

[Files]
; Main executable
Source: "release\plainmd.exe"; DestDir: "{app}"; Flags: ignoreversion

; Qt6 Core DLLs
Source: "release\Qt6Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "release\Qt6Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "release\Qt6Network.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "release\Qt6PrintSupport.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "release\Qt6Svg.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "release\Qt6Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion

; Additional runtime libraries
Source: "release\D3Dcompiler_47.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "release\opengl32sw.dll"; DestDir: "{app}"; Flags: ignoreversion

; Qt Plugins - Platforms
Source: "release\platforms\qwindows.dll"; DestDir: "{app}\platforms"; Flags: ignoreversion

; Qt Plugins - Image formats
Source: "release\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion

; Qt Plugins - TLS/SSL backends
Source: "release\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion

; Qt Plugins - Network information
Source: "release\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion

; Qt Plugins - Generic
Source: "release\generic\*"; DestDir: "{app}\generic"; Flags: ignoreversion

; Qt Plugins - Icon engines
Source: "release\iconengines\*"; DestDir: "{app}\iconengines"; Flags: ignoreversion

; Qt Plugins - Styles
Source: "release\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion

; Translations
; Source: "release\translations\*"; DestDir: "{app}\translations"; Flags: ignoreversion

[Icons]
Name: "{group}\PlainMD"; Filename: "{app}\plainmd.exe"
Name: "{group}\{cm:UninstallProgram,PlainMD}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\PlainMD"; Filename: "{app}\plainmd.exe"; Tasks: desktopicon

[Registry]
; File associations for .md
Root: HKA; Subkey: "Software\Classes\.md"; ValueType: string; ValueName: ""; ValueData: "PlainMD.Document"; Flags: uninsdeletevalue; Tasks: fileassoc
Root: HKA; Subkey: "Software\Classes\PlainMD.Document"; ValueType: string; ValueName: ""; ValueData: "Markdown Document"; Flags: uninsdeletekey; Tasks: fileassoc
Root: HKA; Subkey: "Software\Classes\PlainMD.Document\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\plainmd.exe,0"; Tasks: fileassoc
Root: HKA; Subkey: "Software\Classes\PlainMD.Document\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\plainmd.exe"" ""%1"""; Tasks: fileassoc

; File associations for .markdown
Root: HKA; Subkey: "Software\Classes\.markdown"; ValueType: string; ValueName: ""; ValueData: "PlainMD.Document"; Flags: uninsdeletevalue; Tasks: fileassoc

; File associations for .mdx
Root: HKA; Subkey: "Software\Classes\.mdx"; ValueType: string; ValueName: ""; ValueData: "PlainMD.Document"; Flags: uninsdeletevalue; Tasks: fileassoc

[Run]
Filename: "{app}\plainmd.exe"; Description: "{cm:LaunchProgram,PlainMD}"; Flags: nowait postinstall skipifsilent
