; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "Pegasus Astro Pocket Powerbox Advance X2 PlugIn"
#define MyAppVersion "1.19"
#define MyAppPublisher "RTI-Zone"
#define MyAppURL "https://rti-zone.org"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{076FD2E6-56FD-4343-A1D9-6B37CAB8DBC0}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={code:TSXInstallDir}\Resources\Common
DefaultGroupName={#MyAppName}

; Need to customise these
; First is where you want the installer to end up
OutputDir=installer
; Next is the name of the installer
OutputBaseFilename=PegasusPPBA_X2_Installer
; Final one is the icon you would like on the installer. Comment out if not needed.
SetupIconFile=rti_zone_logo.ico
Compression=lzma
SolidCompression=yes
; We don't want users to be able to select the drectory since read by TSXInstallDir below
DisableDirPage=yes
; Uncomment this if you don't want an uninstaller.
;Uninstallable=no
CloseApplications=yes
DirExistsWarning=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Dirs]
Name: "{app}\Plugins\FocuserPlugins";
Name: "{app}\Plugins64\FocuserPlugins";
Name: "{app}\Plugins\PowerControlPlugIns";
Name: "{app}\Plugins64\PowerControlPlugIns";

[Files]
; Focuser plugin files
Source: "focuserlist PegasusPPBAExtFocuser.txt";                                DestDir: "{app}\Miscellaneous Files"; Flags: ignoreversion
Source: "focuserlist PegasusPPBAExtFocuser.txt";                                DestDir: "{app}\Miscellaneous Files"; DestName: "focuserlist64 PegasusPPBAExtFocuser.txt"; Flags: ignoreversion
; 32 bits
Source: "libPegasusPPBAExtFocuser\Win32\Release\libPegasusPPBAExtFocuser.dll";  DestDir: "{app}\Plugins\FocuserPlugins"; Flags: ignoreversion
Source: "PegasusPPBAExtFocuser.ui";                                             DestDir: "{app}\Plugins\FocuserPlugins"; Flags: ignoreversion
Source: "PegasusAstro.png";                                                     DestDir: "{app}\Plugins\FocuserPlugins"; Flags: ignoreversion
; 64 bits
Source: "libPegasusPPBAExtFocuser\x64\Release\libPegasusPPBAExtFocuser.dll";    DestDir: "{app}\Plugins64\FocuserPlugins"; Flags: ignoreversion; Check: DirExists(ExpandConstant('{app}\Plugins64\FocuserPlugins'))
Source: "PegasusPPBAExtFocuser.ui";                                             DestDir: "{app}\Plugins64\FocuserPlugins"; Flags: ignoreversion; Check: DirExists(ExpandConstant('{app}\Plugins64\FocuserPlugins'))
Source: "PegasusAstro.png";                                                     DestDir: "{app}\Plugins64\FocuserPlugins"; Flags: ignoreversion; Check: DirExists(ExpandConstant('{app}\Plugins64\FocuserPlugins'))

; Power plugin files
Source: "powercontrollist PegasusPPBA.txt";                             DestDir: "{app}\Miscellaneous Files"; Flags: ignoreversion
Source: "powercontrollist PegasusPPBA.txt";                             DestDir: "{app}\Miscellaneous Files"; Flags: ignoreversion; DestName: "powercontrollist64 PegasusPPBA.txt"
; 32 bit
Source: "libPegasusPPBAPower\Win32\Release\libPegasusPPBAPower.dll";    DestDir: "{app}\Plugins\PowerControlPlugIns"; Flags: ignoreversion
Source: "PegasusPPBA.ui";                                               DestDir: "{app}\Plugins\PowerControlPlugIns"; Flags: ignoreversion
Source: "PegasusAstro.png";                                             DestDir: "{app}\Plugins\PowerControlPlugIns"; Flags: ignoreversion
; 64 bit
Source: "libPegasusPPBAPower\x64\Release\libPegasusPPBAPower.dll";      DestDir: "{app}\Plugins64\PowerControlPlugIns"; Flags: ignoreversion; Check: DirExists(ExpandConstant('{app}\Plugins64\PowerControlPlugIns'))
Source: "PegasusPPBA.ui";                                               DestDir: "{app}\Plugins64\PowerControlPlugIns"; Flags: ignoreversion; Check: DirExists(ExpandConstant('{app}\Plugins64\PowerControlPlugIns'))
Source: "PegasusAstro.png";                                             DestDir: "{app}\Plugins64\PowerControlPlugIns"; Flags: ignoreversion; Check: DirExists(ExpandConstant('{app}\Plugins64\PowerControlPlugIns'))

[Code]
{* Below is a function to read TheSkyXInstallPath.txt and confirm that the directory does exist
   This is then used in the DefaultDirName above
   *}
var
  Location: String;
  LoadResult: Boolean;

function TSXInstallDir(Param: String) : String;
begin
  LoadResult := LoadStringFromFile(ExpandConstant('{userdocs}') + '\Software Bisque\TheSkyX Professional Edition\TheSkyXInstallPath.txt', Location);
  if not LoadResult then
    LoadResult := LoadStringFromFile(ExpandConstant('{userdocs}') + '\Software Bisque\TheSky Professional Edition 64\TheSkyXInstallPath.txt', Location);
    if not LoadResult then
      LoadResult := BrowseForFolder('Please locate the installation path for TheSkyX', Location, False);
      if not LoadResult then
        RaiseException('Unable to find the installation path for TheSkyX');
  if not DirExists(Location) then
    RaiseException('TheSkyX installation directory ' + Location + ' does not exist');
  Result := Location;
end;
