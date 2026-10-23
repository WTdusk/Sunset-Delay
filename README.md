# Sunset Delay

Sunset Delay is a guitar delay from White Tea, version 1.1.0. Three engines sit behind one faceplate: tape, analog bucket-brigade, and digital. It is for an electric guitar in front of an amp. Repeats sit behind the pick, and feedback dies out instead of running away.

Load it as an **insert effect** on an audio track. Audio comes in. Dry plus delayed wet goes out. The VST3 category is `Fx|Delay`, so the host keeps the track input connected. The default mix is 28%, and the dry pick stays in front. Mix at zero is the full dry guitar. Bypass passes the dry signal and lets repeats already in the delay line ring out.

Sunset Delay 是 White Tea 的电吉他延迟，版本 1.1.0。面板后面是三套独立引擎：磁带、模拟斗链（bucket-brigade）、数字。它是给音箱前面的电吉他用的。重复落在拨弦后面，反馈会自己衰减，而不是越叠越响。

请把它当作音轨上的**插入效果**来加载。声音进去，出来的是干声加上延迟后的湿声。VST3 类别是 `Fx|Delay`，宿主会保持音轨的音频输入。默认混音 28%，干声仍在前面。混音拧到 0 是完全干声。旁路会放回干声，并让已经写进延迟线的重复自己响完。

Built with [JUCE](https://juce.com) 9.0.4 and CMake. This repository is the source. The Windows release is two files, shipped beside the source rather than inside it:

- `SunsetDelay-Setup-1.1.0.exe` — installer for 64-bit Windows 10 or later
- `SunsetDelay-VST3-1.1.0.zip` — the `Sunset Delay.vst3` folder, for copying by hand

The faceplate is soft clay: raised and pressed pills, hero numbers, and one dusty-orange accent, over a muted dusk coast. The painting is embedded in the plugin. A light veil and vignette sit on it. The two middle cards are a thinner clay frost over a soft blur of that same coast, so the hills and water show through while the knobs stay clear. The standalone frame, mute strip, and audio settings use that same clay, Inter, and orange. An edited factory preset keeps its name and adds an amber star.

## Features / 功能

- **Three engines / 三套引擎.** Tape, analog bucket-brigade, and digital each keep their own delay memory and feedback loop. 磁带、模拟斗链、数字，各自有独立的延迟记忆和反馈回路。
- **Stereo insert / 立体声插入.** Stereo in and stereo out, or mono in and stereo out. Stereo into mono is refused, because that would drop a channel. 支持立体声进立体声出，以及单声道进、立体声出。立体声进、单声道出不被接受，以免丢掉一个声道。
- **Dry stays put / 干声留在原位.** Equal-power mix. At the default 28% the pick is still loud. At 0% the output is the dry guitar. 等功率混合。默认 28% 时拨弦仍然清楚。0% 时输出就是干声。
- **Bypass with trails / 旁路保留尾音.** The dry guitar returns, and repeats already in the loop decay instead of being cut. 干声回来，回路里已有的重复会自己衰减，而不是被切断。
- **Tap and host sync / 点拍与宿主同步.** Tap sets a free time from 50 ms to 1 s. Sync follows the host tempo from a whole note down to a sixteenth-note triplet. 点拍得到 50 ms 到 1 秒的自由时间。同步跟随宿主速度，从全音符到十六分音符三连音。
- **Eleven factory presets / 十一套出厂预设.** Slapbacks, dotted eighths, worn tape, a dark analog lead, and a hard ping-pong. 包含短回声、附点八分、旧磁带、暗的模拟主音，以及硬的左右交替。
- **120 Hz on the repeats only / 只有重复经过 120 Hz.** A highpass trims low end on the way into the loop. The dry guitar is untouched. 高通只修剪进入回路的低频。干声本身不动。

## Windows 安装 / Install on Windows

The published build is **64-bit** and needs **Windows 10 or later**. A 32-bit host cannot load it. 发布的程序是 **64 位**，需要 **Windows 10 或更高版本**。32 位宿主无法加载。

Two ways to install. Both end at the same bundle. 有两种装法，最后都是同一个插件包。

### 安装包 / Installer

Run `SunsetDelay-Setup-1.1.0.exe`. The wizard asks for a folder. The default is the directory most hosts scan on their own:

```text
C:\Program Files\Common Files\VST3
```

The installer creates `Sunset Delay.vst3` inside the folder you chose. The optional standalone, checked by default, goes here, with a Start menu shortcut:

```text
C:\Program Files\White Tea\Sunset Delay\Sunset Delay.exe
```

Clear that checkbox if you only want the plugin.

运行 `SunsetDelay-Setup-1.1.0.exe`。向导会询问文件夹，默认是多数宿主自己会扫描的目录：

```text
C:\Program Files\Common Files\VST3
```

安装程序会在你选的文件夹里创建 `Sunset Delay.vst3`。可选的独立程序默认勾选，装到下面的位置，并在开始菜单放上快捷方式：

```text
C:\Program Files\White Tea\Sunset Delay\Sunset Delay.exe
```

只要插件的话，取消这个勾选。

### 手动复制 / Copy the VST3 folder yourself

Unzip `SunsetDelay-VST3-1.1.0.zip`. The archive contains one folder:

```text
Sunset Delay.vst3\
    Contents\
        x86_64-win\Sunset Delay.vst3
        Resources\moduleinfo.json
```

Copy that **whole folder** to:

```text
C:\Program Files\Common Files\VST3\Sunset Delay.vst3
```

Writing there needs an administrator. If you would rather keep it somewhere else, copy the folder to a directory you own and add that directory to the host's VST3 search path.

The plugin is the folder. Copying only the file `Contents\x86_64-win\Sunset Delay.vst3` does not give the host a plugin it can scan.

解压 `SunsetDelay-VST3-1.1.0.zip`。压缩包里只有一个文件夹 `Sunset Delay.vst3`。把**整个文件夹**复制到：

```text
C:\Program Files\Common Files\VST3\Sunset Delay.vst3
```

写入这个位置需要管理员权限。想放在别的地方时，把文件夹复制到你有权限的目录，再把该目录加入宿主的 VST3 搜索路径。

插件就是这个文件夹。只复制里面的 `Contents\x86_64-win\Sunset Delay.vst3` 文件，宿主扫描不到插件。

### 卸载 / Uninstall

If you used the installer: **Settings → Apps → Installed apps → Sunset Delay → Uninstall**. That removes `Sunset Delay.vst3` from the folder you chose, the standalone, the Start menu shortcuts, and the Apps entry.

如果用的是安装包：**设置 → 应用 → 已安装的应用 → Sunset Delay → 卸载**。这会删掉你所选文件夹里的 `Sunset Delay.vst3`、独立程序、开始菜单快捷方式，以及应用列表中的登记项。

If you copied the zip by hand, delete the `Sunset Delay.vst3` folder you copied. The Apps list does not know about a manual copy, so it cannot remove one.

如果是手动复制的压缩包，删掉你复制出去的 `Sunset Delay.vst3` 文件夹即可。应用列表里没有这一项，也无法替你删除手动复制的文件。

### 宿主里怎么找到它 / Finding it in a DAW

Rescan VST3 plugins after installing or copying. Menu names move between versions. These are the usual places:

- **REAPER:** Options → Preferences → Plug-ins → VST → Re-scan. The default VST3 path is already on the list.
- **Ableton Live:** Preferences → Plug-Ins. Turn on **Use VST3 Plug-In System Folders**, then Rescan.
- **FL Studio:** Options → Manage plugins → Find more plugins. Confirm the scan includes `C:\Program Files\Common Files\VST3`.
- **Cubase:** Studio → VST Plug-in Manager → Rescan. Look under VST3 effects, in the delay group.

Put Sunset Delay on an **audio track or an effect track**, in an effect slot. Do not load it in an instrument slot. Open a guitar (or any audio) clip, leave the factory Pedal preset alone, and the dry pick should be audible with the repeats behind it. Turn Mix to 0% and the track is dry. The plugin's own bypass returns the dry guitar and keeps the trail. Some hosts implement their bypass by skipping the plugin entirely; that also restores the untouched track, which is the host skipping the insert rather than the plugin's bypass.

安装或复制之后，在宿主里重新扫描 VST3。不同版本的菜单位置略有差别，常见入口是：

- **REAPER：** 选项 → 偏好设置 → 插件 → VST → 重新扫描。默认的 VST3 路径一般已经在列表里。
- **Ableton Live：** 偏好设置 → 插件。打开 **Use VST3 Plug-In System Folders**（使用 VST3 系统文件夹），然后重新扫描。
- **FL Studio：** 选项 → 管理插件 → 查找更多插件。确认扫描范围包含 `C:\Program Files\Common Files\VST3`。
- **Cubase：** Studio → VST 插件管理器 → 重新扫描。在 VST3 效果、延迟一类里找。

把 Sunset Delay 放在**音频轨或效果轨**的效果槽里。不要放进乐器槽。打开一段吉他（或任何音频），保持出厂的 Pedal 预设，干声应该听得见，重复在它后面。混音拧到 0% 就是干声。插件自己的旁路会放回干吉他并留下尾音。有的宿主实现旁路的方式是整段跳过插件，这时听到的是完全未处理的音轨，那是宿主跳过了插入，不是插件自己的旁路。

### 不支持的用法 / Not supported

YBK Instrument Player, and other hosts that only list instruments, are not supported. Sunset Delay is an effect. It needs the track's audio input. Marking it as an instrument makes those hosts disable the input, and the insert then outputs silence. This build does not do that. The category is `Fx|Delay` only.

不支持 YBK Instrument Player，也不支持只列出乐器的宿主。Sunset Delay 是效果器，必须用到音轨的音频输入。如果把它标成乐器，这类宿主会关掉输入，插入之后音轨就会没有声音。这个版本不会那样做。类别只有 `Fx|Delay`。

## 从源码构建 / Build from source

On Windows, install Visual Studio 2022 with the Desktop development with C++ workload, CMake, Git, and [Inno Setup 6](https://jrsoftware.org/isdl.php). From the repository root:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target GuitarDelay_VST3 GuitarDelay_Standalone
installer\windows\build-windows.cmd
```

The installer is written to `build\dist\SunsetDelay-Setup-1.1.0.exe`. The same script writes `build\dist\SunsetDelay-VST3-1.1.0.zip`.

在 Windows 上安装 Visual Studio 2022（「使用 C++ 的桌面开发」）、CMake、Git 和 [Inno Setup 6](https://jrsoftware.org/isdl.php)。在仓库根目录执行上面的命令。安装包在 `build\dist\SunsetDelay-Setup-1.1.0.exe`，手动复制用的压缩包在 `build\dist\SunsetDelay-VST3-1.1.0.zip`。

Linux, macOS, the test commands, and the clang-cl cross compile are in [BUILD.md](BUILD.md). JUCE 9 does not build with MinGW. AU is macOS only. Linux 与 macOS 的构建、测试命令，以及用 clang-cl 交叉编译 Windows 安装包的步骤，写在 [BUILD.md](BUILD.md)。JUCE 9 不能用 MinGW。AU 只在 macOS 上构建。

How to send a change: [CONTRIBUTING.md](CONTRIBUTING.md). 参与修改见 [CONTRIBUTING.md](CONTRIBUTING.md)。

## What you hear

默认听感是可以一直开着的磁带回声：时间 375 ms，反馈 34%，混音 28%，宽度 20%。时间旋钮从 50 ms 到 1 秒；跟随宿主时可以到 40 ms–4 秒。三种模式不是「同一个延迟再加一个滤波器」：磁带的读头会晃，模拟是 4096 级斗链，数字是不带调制的分数延迟。下面是这三套引擎的具体做法。

The default patch is a tape echo a guitarist can leave on:

| Control | Default | Why |
| --- | --- | --- |
| Mode | Tape | Worn-head echo, not a clean digital slap |
| Time | 375 ms | Dotted eighth at 120 BPM, in the 80–600 ms guitar range |
| Feedback | 34% | A handful of repeats, then it fades |
| Mix | 28% | Dry pick stays in front (equal-power blend) |
| Width | 20% | A little stereo, still fine if the outs get summed |

Time on the knob runs **50 ms–1 s**, with more resolution around slap and quarter-note echoes. Host sync can reach **40 ms–4 s** (a whole note at 60 BPM). Above that, the time clamps.

## Why the three modes are not one delay with a filter switch

Each mode owns its delay memory and its loop. They share only small pieces that are not the delay itself: one-pole and biquad filters, a DC blocker, an 8× halfband oversampler around the nonlinear stages, and the dry/wet mix in the processor.

### Tape — moving playback head

`Source/DSP/TapeDelay.cpp`

An audio-rate circular buffer. The write head steps one sample at a time, like tape moving across a fixed record head. Wow and flutter move the **read** head: a slow drifting sine (capstan wow) plus a faster sine and a little filtered noise (scrape / flutter). Age does not change the clock. It lowers a lowpass in the regeneration loop, soft-clips playback and the record sum with a touch of even-order bias, and compresses the repeats so they don't pile up. The clip is `tanh(drive · x) / drive`, so the small-signal gain is 1 and a dimed feedback knob still decays. Both waveshapers run at 8× (three cascaded Kaiser halfbands, about 1.7 ms of group delay each) because a naive `tanh` on a bright guitar note throws inharmonic images down into the mids. A 120 Hz highpass sits on the signal entering the loop, so the dry pick keeps its low end while the repeats are trimmed. A small presence boost hits the record side, then each pass through the loss filter darkens the echo. That is the Echoplex / Space Echo shape: pitch drift, squash, and highs that die as the trail gets longer.

### Analog — bucket brigade

`Source/DSP/AnalogDelay.cpp`

Not a fractional audio-rate delay. A fixed **4096-stage** shift register, the same length as an MN3005. Delay time sets the clock:

```text
tick rate = 4096 / delay_seconds
```

which is the datasheet relationship `Td = N / (2 * f_clock)` once you count both phases of the clock as sample shifts. Short delays clock fast and stay bright. Long delays clock slowly, so the anti-alias and reconstruction filters (4th-order Butterworth, cutoff as a fraction of the clock Nyquist) close down. Tone opens or shuts that fraction. The same 120 Hz highpass trims the guitar's low end before it enters the chips. Drive and a feedback-dependent clip (`tanh` at 8×, unity small-signal gain) push the "capacitors." A light compander squishes the loop the way a Memory Man's NE570 does; its envelope is slow, so it stays at the base rate. A very low hiss is added after reconstruction and sits in the stereo field as width comes up. Width also offsets the right-hand clock by a few milliseconds, like two chips that don't match. The output soft clip is a C1 knee above 0.9 so its derivative does not jump.

### Digital — fixed fractional read

`Source/DSP/DigitalDelay.cpp`

A second circular buffer, read with a 4-point Lagrange interpolator and **no** modulation. Time is the number you asked for. A 2-pole hi-cut lives in the feedback loop so bright rhythmic parts (dotted eighths, sixteenths) don't hiss, and it can open to 16 kHz. The same 120 Hz highpass trims whatever enters the loop. Diffuse runs four short Schroeder allpasses in that same loop; at zero they are bypassed, so the repeat stays hard. Width crossfades dual-mono feedback into ping-pong: at full width a mono guitar is written into the left line only, so the first repeat is left and the next is right. A safety limiter only wakes up above full scale, so ordinary guitar stays clean and a dimed feedback knob still can't run away.

Host plumbing (parameters, tempo, bypass trails, equal-power mix, and the crossfade when you change mode) lives in `Source/PluginProcessor.cpp`. It calls one of the three processors. It does not retune a shared delay line.

## Time moves, and changing mode

A linear ramp of the delay time is the wrong gesture for every engine. Sliding 150 ms to 520 ms in a few hundred milliseconds makes the tape head almost stop, and the pitch collapses. Tape and analog therefore slew the delay: at most **0.30** delay-samples per sample on tape (about a fifth of pitch shift) and **0.34** on the brigade. Turning the Time knob glides, the way a varispeed capstan or a BBD clock does.

Digital keeps the read pointer fixed. When the target jumps by more than half a sample, a second fixed read fades in over about **28 ms** (equal power) and the old read fades out. The pitch stays put; the splice is a short smear of the two copies. Moves smaller than that, including sync jitter, track the active read directly. Hi-cut, diffusion, and the safety limiter run once on the blend.

Switching mode is a **0.55 s** equal-power crossfade in the processor. The incoming engine is cleared and snapped to the new time, so it starts clean. The outgoing engine is fed silence and keeps regenerating, so its trail rings under the new mode instead of being chopped. The footswitch bypass does the same kind of thing for the dry path: send fades out, dry returns, and whatever is already in the loop decays.

## Controls

- **Presets** — eleven factory programs (see below). The name plate opens the list. The arrows step to the next and previous program and wrap around. A recall writes mode, sync, division, and every knob through the host in one pass, mode last, so the engines crossfade instead of clicking. The program index is stored with the plugin state; loading a session restores the parameters themselves and does not re-stamp the preset on top of them. After a knob move or a tap, the name grows a star (`Worn Tape *`). Picking that program again reloads the factory values.
- **Tap** — a rubber pad in the tempo row. Two or more taps set Time and turn Sync off. Gaps under 40 ms are ignored, a gap over 2 s starts again, and up to four intervals are averaged. The result is clamped to the knob, **50 ms–1 s**. The lamp stays lit while a tap is armed, then flashes when the time lands. The first tap only arms the clock.
- **Time, Feedback, Mix, Width** — on every mode.
- **Sync** and the division box — host tempo. Divisions run from a whole note to a sixteenth-note triplet. The default division is a dotted eighth (`1/8.`).
- **Tape:** Wow, Flutter, Age.
- **Analog:** Tone, Drive.
- **Digital:** Hi-Cut, Diffuse.
- **Footswitch** — bypass with trails. The dry guitar comes back immediately and the repeats ring out. The same parameter is the host bypass, so a DAW bypass does this too when the host keeps calling `processBlock`.

Every engine highpasses the signal **entering the repeats** at 120 Hz. The dry guitar is untouched, and there is no extra low-cut knob.

Double-click a knob to return it to the default. Continuous controls are smoothed or slewed in the character of the mode. Tap, the preset arrows, and the preset list are editor actions that write the existing parameters.

## Factory presets

Flipping the arrows walks the modes in a guitar order: a tape slap, a dark analog slap, a clean digital slap, two dotted-eighth throws, two tape trails, two analog voices, then a hard ping-pong.

| Program | Mode | What it is |
| --- | --- | --- |
| Pedal | Tape | The default patch, 375 ms, a little wow and age |
| Rockabilly Slap | Tape | 96 ms, almost dry, young tape, nearly mono |
| Dark Slap | Analog | 118 ms, tone closed, some drive. A dark brigade slap |
| Studio Slap | Digital | 88 ms, hi-cut open, diffuse at zero, mono and hard |
| Dotted 8th | Digital | Host sync, dotted eighth, tight width, hi-cut at 9.2 kHz |
| Bright Dotted | Digital | The same division with more repeats, wider, and a little smear |
| Worn Tape | Tape | 420 ms, wow, flutter, and age nearly full |
| Tape Wash | Tape | 640 ms, wide, long feedback, moderate drift. A space-echo wash |
| Dark Analog Lead | Analog | 340 ms, tone almost closed, drive high, narrow |
| Ambient Swell | Analog | 820 ms, very wide, dark tone, the long trail |
| Clean Ping-Pong | Digital | 280 ms, width full, hi-cut open, no diffuse |

## Linux and macOS / Linux 与 macOS

Linux and macOS builds come from source. The commands, package lists, and where the artefacts land are in [BUILD.md](BUILD.md). On Linux, copy `Sunset Delay.vst3` to `~/.vst3/` and rescan. On macOS the same CMake file also builds AU. Install with:

- VST3: `~/Library/Audio/Plug-Ins/VST3`
- AU: `~/Library/Audio/Plug-Ins/Components`

Put it on a guitar track as a mono-to-stereo insert, or on a stereo bus.

Linux 和 macOS 需要从源码构建，命令和依赖写在 [BUILD.md](BUILD.md)。Linux 上把 `Sunset Delay.vst3` 复制到 `~/.vst3/` 再扫描。macOS 上同一份 CMake 还会构建 AU，安装位置见上面两条路径。把它放在吉他轨上，作为单声道进、立体声出的插入，或放在立体声总线上。

The DSP checks and the insert-effect check:

```bash
./build/GuitarDelayRenderTest_artefacts/Release/GuitarDelayRenderTest
./build/GuitarDelayHostCheck_artefacts/GuitarDelayHostCheck
```

`GuitarDelayRenderTest` renders impulses, noise, and tones through each engine: timing, feedback, ping-pong, the bucket-brigade brightness law, tape high-frequency loss, finite dimed feedback, the 8× oversampler, the 120 Hz repeat low-cut, tape time glide, digital time crossfade, and mode-switch trails.

`GuitarDelayHostCheck` is the DAW insert contract. A disabled audio input is rejected. At the default mix an 0.8 pick comes back at the equal-power dry level (about 0.724) on both channels. Mix 0 returns the pick unchanged. Mix at full drops the dry sample and produces the delayed repeat. Bypass passes the dry pick and lets the trail ring.

The demo tool writes 24-bit WAVs of a short DI phrase through the real plugin. See [BUILD.md](BUILD.md).

## Layout

```text
CMakeLists.txt
BUILD.md                      build, tests, Windows packaging
CONTRIBUTING.md               insert-effect rules for changes
LICENSE                       MIT for this source; JUCE is separate
Source/PluginProcessor.cpp    host, mix, sync, bypass trails, mode crossfade, presets
Source/PluginEditor.cpp       pedal faceplate, preset arrows, tap pad
Source/Presets.h              factory guitar programs
Source/Parameters.h           automatable parameters
Source/Tempo.h                note divisions
Source/DSP/TapeDelay.cpp      modulated read head, slewed time, 8× tape curve
Source/DSP/AnalogDelay.cpp    clocked 4096-stage brigade, slewed clock, 8× clip
Source/DSP/DigitalDelay.cpp   Lagrange delay, crossfaded time, hi-cut, ping-pong
Source/DSP/Filters.h          one-poles, biquads, DC block, 8× halfband oversampler
tests/DspRenderTest.cpp       engine render checks
tests/HostCompatibility.cpp   stereo insert, dry/wet, bypass trails
tools/RenderDemos.cpp         DI phrase through the plugin, WAV demos
installer/windows/            Windows installer (Inno Setup, NSIS fallback) and VST3 zip
cmake/toolchains/             clang-cl cross compile against a Windows SDK
```

## Known limitations / 已知限制

- The release binaries are **Windows x64** only (Windows 10 or later). 32-bit hosts are not supported. Linux and macOS are build-from-source. AU is macOS only. 发布的二进制只面向 **Windows 64 位**（Windows 10 及以上）。不支持 32 位宿主。Linux 和 macOS 需要自行构建。AU 只有 macOS。
- **YBK Instrument Player is not supported**, and neither is any host that can only load instruments. Sunset Delay is an `Fx|Delay` insert. It will not advertise an Instrument category. **不支持 YBK Instrument Player**，也不支持只能加载乐器的宿主。Sunset Delay 是 `Fx|Delay` 插入效果，不会把自己标成乐器。
- There is no VST2 build. 没有 VST2 版本。
- The standalone needs an audio device from its menu. A machine with no ALSA device (`/dev/snd` missing) can still open the window; a DAW is the reliable way to hear it. 独立程序需要在菜单里选择音频设备。没有声卡时窗口仍可打开，听声音以宿主为准。
- 8× oversampling on tape (two waveshapers) and on the analog clip costs CPU. The compander stays at the base rate. 磁带的两个波形整形和模拟削波使用 8 倍过采样，会占用 CPU。压缩扩展器仍在原采样率运行。
- Tap follows the Time knob, 50 ms–1 s. Host sync can still reach 4 s. Tap turns sync off and will not set those longer divisions. 点拍只覆盖时间旋钮的 50 ms–1 秒。宿主同步最长仍可达 4 秒。点拍会关掉同步，也不能设出那些更长的时值。
- Analog trails carry a hint of hiss. It is part of the brigade, and on a very long feedback setting it is the floor the repeats decay into. 模拟尾音里有一点嘶声，这是斗链的一部分。反馈开得很长时，重复会衰减到这个底噪。
- The oversamplers add about 1.7 ms of wet latency per waveshaper. Tape pays it twice, so tape echoes land a few milliseconds late. Dry is not delayed, and the plugin reports no latency for PDC. At guitar delays of 50 ms and up, that offset sits inside the echo rather than flamming against the pick. 过采样会给湿声带来大约每个波形整形 1.7 ms 的延迟。磁带有两级，回声会晚几毫秒。干声不延迟，插件也不向宿主报告延迟补偿。在 50 ms 以上的吉他延迟里，这点偏移落在回声内部，不会和拨弦抢在一起。

## License / 许可

The Sunset Delay source in this repository is [MIT](LICENSE), copyright White Tea. JUCE is not in the tree and is not MIT. It is fetched at configure time and is available under the [AGPLv3](https://www.gnu.org/licenses/agpl-3.0.html) or a [commercial JUCE licence](https://juce.com/juce-7-licence/).

A binary built with the AGPL edition of JUCE is a combined work. Distributing that binary means complying with the AGPL, including offering the corresponding source to the people who receive it. A commercial JUCE licence is what allows distributing the binary on terms other than the AGPL. The MIT grant covers the plugin source here. It does not grant rights in JUCE.

本仓库里的 Sunset Delay 源代码以 [MIT](LICENSE) 许可发布，版权归 White Tea。JUCE 不在仓库中，也不适用 MIT。构建时会单独获取 JUCE，其许可是 [AGPLv3](https://www.gnu.org/licenses/agpl-3.0.html) 或 [JUCE 商业许可](https://juce.com/juce-7-licence/)。

用 AGPL 版 JUCE 编出来的程序，和 JUCE 链接在一起，分发时就要遵守 AGPL，包括向收到程序的人提供对应源码。若要按 AGPL 以外的条款分发，需要 JUCE 商业许可。这里的 MIT 只覆盖本仓库的插件源码，不包含对 JUCE 的任何授权。
