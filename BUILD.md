# Building Sunset Delay

Sunset Delay is the product name. The CMake target is still `GuitarDelay`, so the build directories keep that prefix. On disk the plugin is `Sunset Delay.vst3` and the standalone is `Sunset Delay`. Version 1.2.0.

产品名是 Sunset Delay。CMake 目标名仍是 `GuitarDelay`，所以构建目录沿用这个前缀。磁盘上的插件是 `Sunset Delay.vst3`，独立程序是 `Sunset Delay`。版本 1.2.0。

JUCE 9.0.4 is downloaded on the first configure. It is not stored in this repository. Pass `-DFETCHCONTENT_SOURCE_DIR_JUCE=/path/to/JUCE` if you already have that tag checked out.

## Windows, Visual Studio 2022

Install Visual Studio 2022 with the **Desktop development with C++** workload, plus CMake, Git, and [Inno Setup 6](https://jrsoftware.org/isdl.php). From the repository root:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target GuitarDelay_VST3 GuitarDelay_Standalone
installer\windows\build-windows.cmd
```

Outputs:

- VST3: `build\GuitarDelay_artefacts\Release\VST3\Sunset Delay.vst3`
- Standalone: `build\GuitarDelay_artefacts\Release\Standalone\Sunset Delay.exe`
- Installer: `build\dist\SunsetDelay-Setup-1.2.0.exe`
- Manual-copy zip: `build\dist\SunsetDelay-VST3-1.2.0.zip`

The zip contains one folder, `Sunset Delay.vst3`. That folder is the plugin. `build-windows.cmd` writes it with PowerShell `Compress-Archive` after Inno Setup succeeds.

JUCE 9 does not build with MinGW. Use the Visual Studio toolchain on Windows.

## Linux

Packages on Debian or Ubuntu:

```bash
sudo apt install build-essential cmake ninja-build pkg-config git \
  libasound2-dev libfreetype6-dev libfontconfig1-dev \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxext-dev \
  libxcomposite-dev libcurl4-openssl-dev
```

`/usr/bin/c++` is sometimes Clang and then fails to link libstdc++. Point CMake at GCC:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build build -j
```

Outputs:

- VST3: `build/GuitarDelay_artefacts/Release/VST3/Sunset Delay.vst3`
- Standalone: `build/GuitarDelay_artefacts/Release/Standalone/Sunset Delay`

Install the Linux VST3 for a local host:

```bash
mkdir -p ~/.vst3
cp -a "build/GuitarDelay_artefacts/Release/VST3/Sunset Delay.vst3" ~/.vst3/
```

WebKit is not required. JACK is off. ALSA is on for the standalone.

## macOS

Install Xcode command-line tools and CMake. Configure with the Xcode or Ninja generator. `CMakeLists.txt` adds AU when `APPLE` is set.

- VST3: `~/Library/Audio/Plug-Ins/VST3`
- AU: `~/Library/Audio/Plug-Ins/Components`

## Tests

```bash
cmake --build build --target GuitarDelayRenderTest GuitarDelayHostCheck -j
./build/GuitarDelayRenderTest_artefacts/Release/GuitarDelayRenderTest
./build/GuitarDelayHostCheck_artefacts/GuitarDelayHostCheck
```

`GuitarDelayRenderTest` renders impulses, noise, and tones through each engine. It checks timing, feedback, ping-pong, the bucket-brigade brightness law, tape high-frequency loss, finite dimed feedback, the 8× oversampler, the 120 Hz repeat low-cut, tape time glide, digital time crossfade, and mode-switch trails.

`GuitarDelayHostCheck` is the insert-effect check. It rejects a disabled or empty audio input and surround. On a stereo insert, the default mix (28%) keeps the dry pick at the equal-power level, mix 0 is full dry, mix 1 is the delayed wet, and bypass passes dry while the repeats already in the delay line ring out. Mono in / stereo out copies the guitar to both channels without dropping the dry signal.

A single-config Ninja build may omit the `Release/` directory segment. If a path above is missing, look next to it under `build/`.

## Cross-compile a Windows installer from Linux

JUCE 9 cannot use MinGW. The toolchain is `cmake/toolchains/windows-clang-cl.cmake`: Clang's `clang-cl` driver plus a Windows SDK unpacked with [xwin](https://github.com/Jake-Shadle/xwin).

```bash
xwin --accept-license splat --output /tmp/xwin-sdk
./installer/windows/build-windows.sh
```

That configures `build-windows`, builds the VST3 and the standalone, then runs `installer/windows/stage-and-pack.sh`. The pack script refuses a bundle whose `moduleinfo.json` lists an Instrument subcategory, or whose binary still contains the factory string `Fx|Instrument|Delay`. The category for this plugin is `Fx|Delay`.

The script prefers Inno Setup's `ISCC.exe` (including a Wine install) and falls back to NSIS `makensis`. Both installers ask for a folder, default to `C:\Program Files\Common Files\VST3`, and register an uninstaller. Inno Setup is what `SunsetDelay-Setup-1.2.0.exe` is built with. Compression is `lzma2`.

Results land in `build-windows/dist/`:

- `SunsetDelay-Setup-1.2.0.exe`
- `SunsetDelay-VST3-1.2.0.zip`

## What not to commit

Build trees (`build/`, `build-windows/`), the fetched JUCE checkout, the Windows SDK, `*.vst3` bundles, and the installer executable stay out of git. This repository is the source.
