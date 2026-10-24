#define MyAppName "Tauben-Box"
#define MyAppPublisher "taubenopa"
#ifndef MyAppVersion
  #define MyAppVersion "0.1.0"
#endif

[Setup]
AppId={{7D3F2A9E-5B14-4C8E-9A61-3E0B7C2D4F18}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\taubenopa\Tauben-Box
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
WizardImageFile=wizard_large.bmp
WizardSmallImageFile=wizard_small.bmp
SetupIconFile=taubenbox.ico
OutputBaseFilename=Tauben-Box-Setup-{#MyAppVersion}
Compression=lzma2/max
SolidCompression=yes
UninstallDisplayName={#MyAppName} (VST3)

[Languages]
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
german.WelcomeLabel1=Gurr gurr! Willkommen bei Tauben-Box
german.WelcomeLabel2=Dieser Assistent installiert [name/ver] von taubenopa als VST3-Plugin auf deinem Computer.%n%nSchließe vor der Installation am besten deine DAW.
german.FinishedHeadingLabel=Die Tauben sind gelandet!
german.FinishedLabel=[name] wurde installiert. Starte deine DAW und scanne nach neuen VST3-Plugins.
english.WelcomeLabel1=Coo coo! Welcome to Tauben-Box
english.WelcomeLabel2=This wizard installs [name/ver] by taubenopa as a VST3 plugin on your computer.%n%nPlease close your DAW before continuing.
english.FinishedHeadingLabel=The pigeons have landed!
english.FinishedLabel=[name] has been installed. Start your DAW and rescan your VST3 plugins.

[Files]
Source: "build\TaubenBox_artefacts\Release\VST3\Tauben-Box.vst3\*"; DestDir: "{commoncf64}\VST3\Tauben-Box.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Run]
Filename: "{win}\explorer.exe"; Parameters: """{commoncf64}\VST3"""; Description: "VST3-Ordner öffnen / Open VST3 folder"; Flags: postinstall nowait skipifsilent unchecked
