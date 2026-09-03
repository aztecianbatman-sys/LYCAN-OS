[Setup]
AppName=LYCAN OS
AppVersion=1.0.0
DefaultDirName={autopf}\LYCAN OS
DefaultGroupName=LYCAN OS
OutputDir=artifacts
OutputBaseFilename=LycanOS-Setup-1.0.0
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64
Compression=lzma
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\lycan-vm.exe

[Files]
Source: "..\build\Release\lycan-vm.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\Release\lycan-cli.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\store\catalog.json"; DestDir: "{app}\store"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion

[Dirs]
Name: "{app}\data"
Name: "{app}\logs"
Name: "{localappdata}\LycanOS\data"

[Icons]
Name: "{group}\LYCAN OS"; Filename: "{app}\lycan-vm.exe"; WorkingDir: "{app}"
Name: "{commondesktop}\LYCAN OS"; Filename: "{app}\lycan-vm.exe"; WorkingDir: "{app}"

[Run]
Filename: "{app}\lycan-vm.exe"; Description: "Launch LYCAN OS"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}\logs"
Type: filesandordirs; Name: "{app}\data"
