# Guitar Synth

Monophonic **guitar-to-synth** audio plugin for macOS. Pitch-tracks your guitar input in real time and drives a subtractive synthesizer with minimal latency.

## Features

- YIN pitch tracking optimized for monophonic guitar (80 Hz – 1200 Hz)
- Frequency-direct oscillator control (no MIDI note quantization delay)
- Input gate / envelope follower for natural note articulation
- Subtractive synth: sine, saw, or square oscillator + resonant low-pass filter + ADSR
- Live pitch display (Hz + note name), confidence meter, voiced indicator, latency readout
- Formats: **VST3**, **AU**, and **Standalone**

## Latency

Typical end-to-end latency at 48 kHz with a 128-sample buffer:

| Stage | Approx. |
|-------|---------|
| Input buffer | ~2.7 ms |
| Pitch analysis (512-sample window) | ~5–10 ms |
| Output buffer | ~2.7 ms |
| **Total** | **~10–15 ms** |

### Tips for lowest latency

1. Use the **Standalone** app for live playing (no DAW overhead).
2. Set audio buffer size to **64** or **128** samples in the Standalone audio settings.
3. On first launch, macOS will ask for **Microphone** access — click **Allow**. If you previously denied it: **System Settings → Privacy & Security → Microphone → enable Guitar Synth**, or run `tccutil reset Microphone com.guitarsynth.plugin` and relaunch.
4. Enable the correct **input channels** for your guitar in Audio Settings (usually input 1 on your interface).
4. Use **headphones** when monitoring through speakers to avoid feedback (Standalone enables live input by default).
5. Disable direct monitoring on your audio interface if you hear a dry double.
6. Use a relatively clean guitar signal; heavy distortion makes tracking harder.
7. Adjust **Gate** and **Tracking** knobs if notes fail to trigger or flicker.

Run the automated checks after building:

```bash
cmake --build build --target GuitarSynthTests
ctest --test-dir build --output-on-failure
```

The plugin reports latency to the host via `getLatencySamples()` so DAWs can compensate.

## Requirements

- macOS 11+
- Xcode Command Line Tools
- CMake 3.22+

Install tools if needed:

```bash
xcode-select --install
brew install cmake
```

## Build

```bash
cd "/Users/thomas/Documents/Data science/guitar_vst"
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

### Outputs

- Standalone: `build/GuitarSynth_artefacts/Release/Standalone/Guitar Synth.app`
- VST3: `~/Library/Audio/Plug-Ins/VST3/Guitar Synth.vst3`
- AU: `~/Library/Audio/Plug-Ins/Components/Guitar Synth.component`

## Live setup

1. Connect guitar → audio interface input.
2. Open **Guitar Synth.app** (Standalone).
3. Select your interface as input and output in **Audio/MIDI Settings**.
4. Set buffer size to 128 samples (or 64 if your interface supports it).
5. Play single-note lines for best tracking; bends and hammer-ons are supported with glide enabled.

## Parameters

| Parameter | Description |
|-----------|-------------|
| Waveform | Sine, saw, or square oscillator |
| Cutoff / Resonance | Low-pass filter tone shaping |
| Attack / Decay / Sustain / Release | Amplitude envelope |
| Glide | Portamento between detected pitches |
| Master | Output level |
| Tracking | Pitch confidence threshold (higher = stricter) |
| Gate | Input level threshold in dB |

## License

Built with [JUCE](https://juce.com). JUCE is free for personal and open-source projects; commercial distribution requires a JUCE license.
