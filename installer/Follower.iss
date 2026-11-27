[Setup]
AppName=Follower
AppVersion=1.1.0
AppPublisher=Hanging Puppet Audio
DefaultDirName={autopf64}\Hanging Puppet Audio\Follower
DefaultGroupName=Follower
OutputDir=installer_out
OutputBaseFilename=Follower-1.1.0-Windows-Setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
Compression=lzma2
SolidCompression=yes

[Components]
Name: "vst3"; Description: "VST3 Plugin (64-bit)"; Types: full compact custom; Flags: fixed
Name: "standalone"; Description: "Standalone Application"; Types: full custom
Name: "presets"; Description: "Factory Presets"; Types: full compact custom

[Files]
; VST3 bundle (recursive copy of the .vst3 folder)
Source: "..\build\Follower_artefacts\Release\VST3\Follower.vst3\*"; DestDir: "{commoncf64}\VST3\Hanging Puppet Audio\Follower.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs

; Standalone executable
Source: "..\build\Follower_artefacts\Release\Standalone\Follower.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

; Factory XML presets -> Documents\VST3 Presets\Hanging Puppet Audio\Follower
Source: "..\Presets\*.xml"; DestDir: "{userdocs}\VST3 Presets\Hanging Puppet Audio\Follower"; Components: presets; Flags: onlyifdoesntexist

[Icons]
Name: "{group}\Follower"; Filename: "{app}\Follower.exe"; Components: standalone