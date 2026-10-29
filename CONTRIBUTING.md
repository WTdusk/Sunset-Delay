# Contributing to Sunset Delay

Sunset Delay is a guitar delay by WTdusk. Issues and small, focused changes are welcome. The product name stays **Sunset Delay**. The manufacturer stays **WTdusk**. CMake targets stay `GuitarDelay` so existing build scripts keep working.

Sunset Delay 是 WTdusk 的吉他延迟。欢迎针对具体问题的修改。产品名保持 **Sunset Delay**，厂商保持 **WTdusk**。CMake 目标名保持 `GuitarDelay`，现有构建脚本不用改。

## Build and test

Follow [BUILD.md](BUILD.md). Before sending a change that touches audio, the editor, or plugin metadata, run:

```bash
./build/GuitarDelayRenderTest_artefacts/Release/GuitarDelayRenderTest
./build/GuitarDelayHostCheck_artefacts/GuitarDelayHostCheck
```

Both must pass. The host check is the DAW insert contract: stereo in, dry plus wet out, no silence at the default mix, full dry at mix 0, bypass trails intact.

## The insert must keep its input

This plugin is an effect on a track. A few rules keep it that way:

- The VST3 category is `Fx|Delay`. Do not add `Instrument`. Hosts treat `Fx|Instrument` as a plugin that may run with the audio input disabled, and the track then goes silent.
- `isBusesLayoutSupported` accepts mono and stereo only, with the input no wider than the output. A disabled or empty input is not a supported layout.
- `processBlock` must not clear channels that already hold the incoming audio. Spare outputs (mono in, stereo out) may start empty; the track itself may not.
- The default preset leaves the dry pick audible. Mix 0 is full dry. Bypass returns the dry signal and lets repeats already in the delay line ring out.

Instrument-only hosts, including YBK Instrument Player, are outside the supported setup. Do not special-case them by turning the effect into an instrument.

## Source layout

DSP lives in `Source/DSP/`. The processor in `Source/PluginProcessor.cpp` owns mix, sync, bypass trails, presets, and the mode crossfade. The faceplate is `Source/PluginEditor.cpp`. Factory programs are `Source/Presets.h`. The Windows installer is `installer/windows/`.

Match the style of the file you are editing. Do not reformat unrelated code. Do not restyle the faceplate unless the change is specifically about the look.

## License

Plugin source in this repository is under the MIT license in [LICENSE](LICENSE). JUCE is fetched at build time and is not MIT. Do not vendor JUCE, and do not commit build directories, VST3 bundles, or the installer executable.
