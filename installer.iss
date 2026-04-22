; MD Viewer Installer
; Inno Setup Script for mdviewer

[Setup]
AppName=MD Viewer
AppVersion=1.0.0
AppVerName=MD Viewer 1.0.0
DefaultDirName={autopf64}\MD Viewer
DefaultGroupName=MD Viewer
OutputDir=.
OutputBaseFilename=mdviewer-setup
SetupIconFile=icon.ico
UninstallDisplayIcon={app}\mdviewer.exe
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
Name: "fileassoc"; Description: "Associate MD Viewer with Markdown files"; GroupDescription: "Other tasks:"

[Files]
; Main executable
Source: "release\mdviewer.exe"; DestDir: "{app}"; Flags: ignoreversion

; Qt6 Core DLLs
Source: "release\Qt6Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "release\Qt6Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "release\Qt6Network.dll"; DestDir: "{app}"; Flags: ignoreversion
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
Name: "{group}\MD Viewer"; Filename: "{app}\mdviewer.exe"
Name: "{group}\{cm:UninstallProgram,MD Viewer}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\MD Viewer"; Filename: "{app}\mdviewer.exe"; Tasks: desktopicon

[Registry]
; File associations for .md
Root: HKA; Subkey: "Software\Classes\.md"; ValueType: string; ValueName: ""; ValueData: "MDViewer.Document"; Flags: uninsdeletevalue; Tasks: fileassoc
Root: HKA; Subkey: "Software\Classes\MDViewer.Document"; ValueType: string; ValueName: ""; ValueData: "Markdown Document"; Flags: uninsdeletekey; Tasks: fileassoc
Root: HKA; Subkey: "Software\Classes\MDViewer.Document\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\mdviewer.exe,0"; Tasks: fileassoc
Root: HKA; Subkey: "Software\Classes\MDViewer.Document\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\mdviewer.exe"" ""%1"""; Tasks: fileassoc

; File associations for .markdown
Root: HKA; Subkey: "Software\Classes\.markdown"; ValueType: string; ValueName: ""; ValueData: "MDViewer.Document"; Flags: uninsdeletevalue; Tasks: fileassoc

; File associations for .mdx
Root: HKA; Subkey: "Software\Classes\.mdx"; ValueType: string; ValueName: ""; ValueData: "MDViewer.Document"; Flags: uninsdeletevalue; Tasks: fileassoc

[Run]
Filename: "{app}\mdviewer.exe"; Description: "{cm:LaunchProgram,MD Viewer}"; Flags: nowait postinstall skipifsilent
