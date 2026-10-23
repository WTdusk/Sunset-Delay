; Sunset Delay Windows installer (Inno Setup 6).
;
; Default folder is the 64-bit VST3 directory hosts scan on their own:
;   C:\Program Files\Common Files\VST3\Sunset Delay.vst3
; The directory page can point that bundle somewhere else.
; The optional standalone app always goes to:
;   C:\Program Files\White Tea\Sunset Delay\Sunset Delay.exe
; Uninstall is registered with Add/Remove Programs (Settings → Apps).

#ifndef PayloadDir
  #define PayloadDir "payload"
#endif
#ifndef OutputDir
  #define OutputDir "output"
#endif

#define MyAppName "Sunset Delay"
#define MyAppVersion "1.1.0"
#define MyAppPublisher "White Tea"
#define MyAppId "{{B7E4C2A1-9F63-4D18-A5C0-6E8D3F1B72A4}"

[Setup]
AppId={#MyAppId}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
VersionInfoVersion=1.1.0.0
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription=Sunset Delay VST3 and standalone installer
VersionInfoProductName={#MyAppName}
DefaultDirName={commoncf64}\VST3
DisableDirPage=no
DirExistsWarning=no
UsePreviousAppDir=yes
AppendDefaultDirName=no
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
DisableWelcomePage=yes
InfoBeforeFile=InstallInfo.txt
OutputDir={#OutputDir}
OutputBaseFilename=SunsetDelay-Setup-{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
MinVersion=10.0
UninstallDisplayName={#MyAppName}
UninstallFilesDir={autopf}\White Tea\Sunset Delay
CloseApplications=yes
RestartApplications=no
SetupLogging=yes

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "zh"; MessagesFile: "languages\ChineseSimplified.isl"

[CustomMessages]
en.ExtraGroup=Optional:
zh.ExtraGroup=可选：
en.StandaloneTask=Install the standalone app and a Start menu shortcut
zh.StandaloneTask=安装独立程序，并在开始菜单创建快捷方式

[Messages]
en.SelectDirDesc=Where should the VST3 be installed?
en.SelectDirLabel3=Sunset Delay.vst3 is created in this folder. The default is the folder most Windows hosts scan automatically. If you change it, add that folder to your DAW's VST3 search path.
en.SelectTasksDesc=Which extra pieces should be installed?
en.FinishedHeadingLabel=Sunset Delay is installed
en.FinishedLabel=The plugin is in the folder you chose, inside Sunset Delay.vst3. Rescan plugins in your host.%n%nUninstall later from Settings → Apps → Sunset Delay.
zh.SelectDirDesc=要把 VST3 安装到哪个文件夹？
zh.SelectDirLabel3=安装程序会在此文件夹中创建 Sunset Delay.vst3。默认位置是多数 Windows 宿主自动扫描的目录。如果改了路径，请把该文件夹加入宿主的 VST3 扫描路径。
zh.SelectTasksDesc=还要安装哪些内容？
zh.FinishedHeadingLabel=Sunset Delay 已安装
zh.FinishedLabel=插件位于你选择的文件夹中的 Sunset Delay.vst3。请在宿主里重新扫描插件。%n%n之后可在「设置 → 应用」中卸载 Sunset Delay。

[Tasks]
Name: "standalone"; Description: "{cm:StandaloneTask}"; GroupDescription: "{cm:ExtraGroup}"; Flags: checkedonce

[InstallDelete]
Type: filesandordirs; Name: "{app}\Guitar Delay.vst3"
Type: files; Name: "{autopf}\White Tea\Guitar Delay\Guitar Delay.exe"

[Files]
Source: "{#PayloadDir}\Sunset Delay.vst3\*"; DestDir: "{app}\Sunset Delay.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#PayloadDir}\Sunset Delay.exe"; DestDir: "{autopf}\White Tea\Sunset Delay"; Flags: ignoreversion skipifsourcedoesntexist; Tasks: standalone

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{autopf}\White Tea\Sunset Delay\Sunset Delay.exe"; Tasks: standalone
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"

[UninstallDelete]
Type: filesandordirs; Name: "{app}\Sunset Delay.vst3"
Type: filesandordirs; Name: "{app}\Guitar Delay.vst3"
Type: dirifempty; Name: "{autopf}\White Tea\Sunset Delay"
Type: dirifempty; Name: "{autopf}\White Tea"
