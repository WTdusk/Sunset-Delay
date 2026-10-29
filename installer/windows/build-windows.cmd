@echo off
setlocal EnableExtensions
rem Package a Release Windows build with Inno Setup.
rem From a Visual Studio build:
rem   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
rem   cmake --build build --config Release --target GuitarDelay_VST3 GuitarDelay_Standalone
rem   installer\windows\build-windows.cmd
rem Optional argument: path to the CMake build directory (default: build).

set "ROOT=%~dp0..\.."
pushd "%ROOT%"
set "BUILD=%CD%\build"
if not "%~1"=="" set "BUILD=%~1"

set "STAGE=%BUILD%\windows-stage"
set "DIST=%BUILD%\dist"
set "VST3=%BUILD%\GuitarDelay_artefacts\Release\VST3\Sunset Delay.vst3"
set "APP=%BUILD%\GuitarDelay_artefacts\Release\Standalone\Sunset Delay.exe"

if not exist "%VST3%\Contents" (
    echo Could not find "%VST3%".
    echo Build the Release VST3 target first. The repository does not contain a prebuilt plugin.
    popd
    exit /b 1
)

if exist "%STAGE%" rmdir /s /q "%STAGE%"
mkdir "%STAGE%"
if not exist "%DIST%" mkdir "%DIST%"

xcopy "%VST3%" "%STAGE%\Sunset Delay.vst3\" /E /I /Y >nul
if exist "%APP%" copy /Y "%APP%" "%STAGE%\Sunset Delay.exe" >nul

set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" (
    echo Inno Setup 6 is not installed.
    echo Download it from https://jrsoftware.org/isdl.php and rerun this script.
    popd
    exit /b 1
)

"%ISCC%" "/DPayloadDir=%STAGE%" "/DOutputDir=%DIST%" "%CD%\installer\windows\GuitarDelay.iss"
set "ERR=%ERRORLEVEL%"
if not "%ERR%"=="0" (
    popd
    exit /b %ERR%
)

set "VERSION=1.2.0"
for /f "tokens=3" %%V in ('findstr /B /C:"project(GuitarDelay VERSION " CMakeLists.txt') do set "VERSION=%%V"
set "VST3_ZIP=%DIST%\SunsetDelay-VST3-%VERSION%.zip"
if exist "%VST3_ZIP%" del /f "%VST3_ZIP%"
powershell -NoProfile -Command "Compress-Archive -Path '%STAGE%\Sunset Delay.vst3' -DestinationPath '%VST3_ZIP%' -Force"
set "ERR=%ERRORLEVEL%"
if not "%ERR%"=="0" (
    echo Could not write "%VST3_ZIP%".
    popd
    exit /b %ERR%
)
echo VST3 zip: %VST3_ZIP%
set "SUM=%DIST%\SunsetDelay-%VERSION%.sha256"
powershell -NoProfile -Command "Get-FileHash -Algorithm SHA256 '%DIST%\SunsetDelay-Setup-%VERSION%.exe','%DIST%\SunsetDelay-VST3-%VERSION%.zip' | ForEach-Object { '{0}  {1}' -f $_.Hash.ToLower(), (Split-Path $_.Path -Leaf) } | Set-Content -Encoding ascii '%SUM%'"
echo Checksums: %SUM%
popd
exit /b 0
