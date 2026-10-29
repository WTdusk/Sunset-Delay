; NSIS fallback for the same install layout as GuitarDelay.iss.
; Used when Inno Setup's compiler is not available (makensis runs on Linux).
; Default folder: $COMMONFILES64\VST3  (C:\Program Files\Common Files\VST3)
; Optional standalone: $PROGRAMFILES64\White Tea\Sunset Delay\Sunset Delay.exe
; Uninstaller is registered for Settings → Apps.

Unicode True
ManifestDPIAware true

!include MUI2.nsh
!include x64.nsh
!include LogicLib.nsh
!include FileFunc.nsh

!ifndef PAYLOAD
  !define PAYLOAD "payload"
!endif
!ifndef OUTFILE
  !define OUTFILE "SunsetDelay-Setup-1.2.0.exe"
!endif

Name "Sunset Delay"
OutFile "${OUTFILE}"
InstallDir "$COMMONFILES64\VST3"
InstallDirRegKey HKLM "Software\White Tea\Sunset Delay" "Vst3Dir"
RequestExecutionLevel admin
BrandingText "White Tea"

!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

!define MUI_WELCOMEPAGE_TITLE "Sunset Delay"
!define MUI_WELCOMEPAGE_TEXT "Installs the Sunset Delay VST3.$\r$\n$\r$\nThe next pages choose the VST3 folder (default: the standard Windows VST3 directory) and whether to add the standalone app.$\r$\n$\r$\n简体中文：接下来选择 VST3 文件夹，默认为 Windows 标准 VST3 目录。插件会装成 Sunset Delay.vst3。可以同时安装独立程序。卸载请打开「设置 → 应用」，找到 Sunset Delay。"

!define MUI_COMPONENTSPAGE_TEXT_TOP "VST3 is always installed. Clear the standalone box to skip the app and its Start menu shortcut.$\r$\n$\r$\nVST3 一定会安装。取消独立程序即可不安装应用和开始菜单快捷方式。"
!define MUI_DIRECTORYPAGE_TEXT_TOP "Sunset Delay.vst3 is created in this folder. Leave the default unless you want a custom VST3 path. If you change it, add the folder to your host's plugin search path.$\r$\n$\r$\n将在此文件夹创建 Sunset Delay.vst3。默认是宿主会自动扫描的位置。改了路径后，请把该文件夹加入宿主的 VST3 扫描路径。"
!define MUI_DIRECTORYPAGE_TEXT_DEST "VST3 folder"
!define MUI_FINISHPAGE_TITLE "Sunset Delay is installed"
!define MUI_FINISHPAGE_TEXT "Rescan plugins in your host. The bundle is Sunset Delay.vst3 inside the folder you chose.$\r$\n$\r$\nUninstall from Settings → Apps → Sunset Delay. That removes the VST3, the standalone app, and the Start menu shortcuts.$\r$\n$\r$\n请在宿主里重新扫描。卸载：设置 → 应用 → Sunset Delay。"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "SimpChinese"

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecVst} "Copies Sunset Delay.vst3 into the folder chosen on the next page."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecApp} "Installs Sunset Delay.exe under Program Files\White Tea\Sunset Delay and adds a Start menu shortcut."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section "VST3 plugin (required)" SecVst
  SectionIn RO
  RMDir /r "$INSTDIR\Guitar Delay.vst3"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Guitar Delay"
  SetOutPath "$INSTDIR\Sunset Delay.vst3"
  File /r "${PAYLOAD}/Sunset Delay.vst3/*.*"

  CreateDirectory "$PROGRAMFILES64\White Tea\Sunset Delay"
  WriteUninstaller "$PROGRAMFILES64\White Tea\Sunset Delay\Uninstall.exe"

  SetRegView 64
  WriteRegStr HKLM "Software\White Tea\Sunset Delay" "Vst3Dir" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sunset Delay" "DisplayName" "Sunset Delay"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sunset Delay" "DisplayVersion" "1.2.0"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sunset Delay" "Publisher" "White Tea"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sunset Delay" "InstallLocation" "$INSTDIR\Sunset Delay.vst3"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sunset Delay" "UninstallString" '"$PROGRAMFILES64\White Tea\Sunset Delay\Uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sunset Delay" "QuietUninstallString" '"$PROGRAMFILES64\White Tea\Sunset Delay\Uninstall.exe" /S'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sunset Delay" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sunset Delay" "NoRepair" 1
  ${GetSize} "$INSTDIR\Sunset Delay.vst3" "/S=0K" $0 $1 $2
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sunset Delay" "EstimatedSize" "$0"
SectionEnd

Section "Standalone application" SecApp
  SetOutPath "$PROGRAMFILES64\White Tea\Sunset Delay"
  File "${PAYLOAD}/Sunset Delay.exe"
  CreateDirectory "$SMPROGRAMS\Sunset Delay"
  CreateShortcut "$SMPROGRAMS\Sunset Delay\Sunset Delay.lnk" "$PROGRAMFILES64\White Tea\Sunset Delay\Sunset Delay.exe"
  CreateShortcut "$SMPROGRAMS\Sunset Delay\Uninstall Sunset Delay.lnk" "$PROGRAMFILES64\White Tea\Sunset Delay\Uninstall.exe"
  WriteRegDWORD HKLM "Software\White Tea\Sunset Delay" "Standalone" 1
SectionEnd

Section "un.Uninstall"
  SetRegView 64
  ReadRegStr $0 HKLM "Software\White Tea\Sunset Delay" "Vst3Dir"
  ${If} $0 != ""
    RMDir /r "$0\Sunset Delay.vst3"
    RMDir /r "$0\Guitar Delay.vst3"
  ${EndIf}

  Delete "$SMPROGRAMS\Sunset Delay\Sunset Delay.lnk"
  Delete "$SMPROGRAMS\Sunset Delay\Uninstall Sunset Delay.lnk"
  RMDir "$SMPROGRAMS\Sunset Delay"

  Delete "$PROGRAMFILES64\White Tea\Sunset Delay\Sunset Delay.exe"
  Delete "$PROGRAMFILES64\White Tea\Sunset Delay\Uninstall.exe"
  RMDir "$PROGRAMFILES64\White Tea\Sunset Delay"
  RMDir "$PROGRAMFILES64\White Tea"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Sunset Delay"
  DeleteRegKey HKLM "Software\White Tea\Sunset Delay"
SectionEnd

Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_OK|MB_ICONSTOP "Sunset Delay needs 64-bit Windows.$\r$\nSunset Delay 需要 64 位 Windows。"
    Abort
  ${EndIf}
  SetRegView 64
FunctionEnd
