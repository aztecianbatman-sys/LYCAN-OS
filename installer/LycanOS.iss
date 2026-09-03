#define AppName "LYCAN OS"
#define AppVersion "0.6.0"
#define AppPublisher "LYCAN"
#define AppExeName "lycan-vm.exe"

[Setup]
AppId={{B7C9E0F7-7E1D-4D84-9C7D-5E8A6A9D0422}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\LYCAN OS
DefaultGroupName=LYCAN OS
OutputDir=Output
OutputBaseFilename=LycanOS-Setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
WizardStyle=modern
UninstallDisplayIcon={app}\{#AppExeName}
DisableProgramGroupPage=yes

[Files]
Source: "payload\lycan-vm.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "payload\lycan-cli.exe"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "payload\store\*"; DestDir: "{app}\store"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "payload\docs\*"; DestDir: "{app}\docs"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist

[Dirs]
Name: "{app}\data"
Name: "{app}\data\packages"
Name: "{app}\logs"
Name: "{localappdata}\LYCAN OS"

[Icons]
Name: "{autoprograms}\LYCAN OS"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"
Name: "{autodesktop}\LYCAN OS"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Run]
Filename: "{app}\{#AppExeName}"; Description: "Launch LYCAN OS"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}\logs"
