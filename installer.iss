; Vibe-MD Installer
; Inno Setup Script for Vibe-MD

[Setup]
AppName=Vibe-MD
AppVersion=1.1
AppVerName=Vibe-MD 1.1
DefaultDirName={autopf64}\Vibe-MD
DefaultGroupName=Vibe-MD
OutputDir=.
OutputBaseFilename=vibe-md-setup
SetupIconFile=icon.ico
UninstallDisplayIcon={app}\vibe-md.exe
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
Name: "fileassoc"; Description: "Associate Vibe-MD with Markdown files"; GroupDescription: "Other tasks:"

[Files]
; Main executable
Source: "release\vibe-md.exe"; DestDir: "{app}"; Flags: ignoreversion

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
Name: "{group}\Vibe-MD"; Filename: "{app}\vibe-md.exe"
Name: "{group}\{cm:UninstallProgram,Vibe-MD}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Vibe-MD"; Filename: "{app}\vibe-md.exe"; Tasks: desktopicon

[Registry]
; File associations for .md
Root: HKA; Subkey: "Software\Classes\.md"; ValueType: string; ValueName: ""; ValueData: "VibeMD.Document"; Flags: uninsdeletevalue; Tasks: fileassoc
Root: HKA; Subkey: "Software\Classes\VibeMD.Document"; ValueType: string; ValueName: ""; ValueData: "Markdown Document"; Flags: uninsdeletekey; Tasks: fileassoc
Root: HKA; Subkey: "Software\Classes\VibeMD.Document\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\vibe-md.exe,0"; Tasks: fileassoc
Root: HKA; Subkey: "Software\Classes\VibeMD.Document\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\vibe-md.exe"" ""%1"""; Tasks: fileassoc

; File associations for .markdown
Root: HKA; Subkey: "Software\Classes\.markdown"; ValueType: string; ValueName: ""; ValueData: "VibeMD.Document"; Flags: uninsdeletevalue; Tasks: fileassoc

; File associations for .mdx
Root: HKA; Subkey: "Software\Classes\.mdx"; ValueType: string; ValueName: ""; ValueData: "VibeMD.Document"; Flags: uninsdeletevalue; Tasks: fileassoc

[Run]
Filename: "{app}\vibe-md.exe"; Description: "{cm:LaunchProgram,Vibe-MD}"; Flags: nowait postinstall skipifsilent
