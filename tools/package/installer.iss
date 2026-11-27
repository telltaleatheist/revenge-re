; The Windows installer for players: Inno Setup (jrsoftware.org, free), built
; by tools/build_package.py --installer from the package folder it has just
; staged:
;   iscc /DAppVersion=<version> /DDist=<package folder> /DStem=<file stem> /O<out dir> tools\package\installer.iss
; It installs to Program Files\Revenge Rebuild with a Start-menu shortcut (a
; desktop one ticked by default) and an uninstaller. The game writes nothing
; there: the player's settings (settings.txt, setup.txt), the copy of the
; game the setup makes (install\), saves and story\ live in
; %APPDATA%\BattleTech\Revenge Rebuild, which the uninstaller and every
; update leave alone.
; No file of the original game is in the installer.

#ifndef AppVersion
  #define AppVersion "dev"
#endif
#ifndef Dist
  #error Give /DDist=<the package folder> (build_package.py does).
#endif
#ifndef Stem
  #define Stem "CHR-Rebuild-" + AppVersion
#endif

[Setup]
; This project's own AppId, the same every version: a new installer upgrades
; the old install in place.
AppId={{332207BE-F98B-4055-A1AF-5DCD78F47FEB}
AppName=Revenge Rebuild
AppVersion={#AppVersion}
AppVerName=Revenge Rebuild {#AppVersion}
AppPublisher=Revenge Rebuild (a fan project)
AppComments=BattleTech: The Crescent Hawks' Revenge (1990), rebuilt on Elthial's SDL port. It needs your own copy of the game.
DefaultDirName={autopf}\Revenge Rebuild
; The folder page every time, an upgrade's too, offering the folder already
; installed to.
DisableDirPage=no
DefaultGroupName=Revenge Rebuild
DisableProgramGroupPage=yes
; Program Files needs administrator rights; a player without them can pick
; "only for me" (the user's own Programs folder) in the first dialog.
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
SetupIconFile=revenge.ico
UninstallDisplayIcon={app}\chr.exe
UninstallDisplayName=Revenge Rebuild
OutputBaseFilename={#Stem}-Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
#if FileExists(AddBackslash(Dist) + "LICENSE")
LicenseFile={#Dist}\LICENSE
#endif

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "{#Dist}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[InstallDelete]
; data\ is ours and replaced whole, so a file an earlier version shipped and
; this one dropped doesn't linger. Nothing else in {app} is touched.
Type: filesandordirs; Name: "{app}\data"

[Icons]
Name: "{autoprograms}\Revenge Rebuild"; Filename: "{app}\chr.exe"; WorkingDir: "{app}"; Comment: "The Crescent Hawks' Revenge (1990), rebuilt on Elthial's SDL port"
Name: "{autoprograms}\Revenge Rebuild - Read me"; Filename: "{app}\README.txt"
Name: "{autodesktop}\Revenge Rebuild"; Filename: "{app}\chr.exe"; WorkingDir: "{app}"; Comment: "The Crescent Hawks' Revenge (1990), rebuilt on Elthial's SDL port"; Tasks: desktopicon

[Run]
Filename: "{app}\chr.exe"; Description: "{cm:LaunchProgram,Revenge Rebuild}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Only what the installer put in {app}; the player's folder in %APPDATA% stays.
Type: filesandordirs; Name: "{app}\data"
Type: dirifempty; Name: "{app}"

[Messages]
UninstalledAll=%1 was removed from your computer.%n%nYour settings, saved games and the copy of the game it set up are kept in %APPDATA%\BattleTech\Revenge Rebuild. Delete that folder to remove them too. Your own copy of the game was never changed.
