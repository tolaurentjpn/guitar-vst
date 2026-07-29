# Guitar Synth

Monophonic **guitar-to-synth** audio plugin for macOS and PC. Pitch-tracks your guitar input in real time and drives a dual-oscillator subtractive synthesizer with unison, PWM, sub/noise, filter envelopes, a pro-style arpeggiator, a dedicated chorus, a reorderable FX rack, and factory presets (including cinematic Zimmer and Jupiter-8 characters).

AU is macOS-only; VST3 + Standalone are built for both macOS and PC.

## Features

- Hybrid pitch tracking: YIN period discovery + a tiny neural net for octave / voicing decisions (80 Hz – 1200 Hz)
- Frequency-direct oscillator control (no MIDI note quantization delay)
- Input gate / envelope follower with ADSR release on note-off (no hard mute cut)
- **Pick-attack retrigger**: successive plucks of the same fretted note re-attack the synth (eighths / sixteenths) without waiting for the gate to close
- Dual oscillators (sine / saw / square / triangle) with pulse-width on square, per-osc low-pass filter, and amp ADSR
- Sub oscillator (−1 octave) and noise bed mixed into Osc 1 before its filter
- Vital-style **unison** per oscillator: Voices (1–8), Detune, Stereo Spread, Blend, Phase Random
- Per-filter **ADSR envelopes** with Amount, edited as draggable plots on the Filter Env tab (attacks up to 5 s for cinematic blooms)
- Dual LFOs (rate, shape, cutoff / resonance / pitch / amp / PWM modulation)
- Gate-driven **arpeggiator** (chord from tracked root, octaves, modes, rate/sync, gate %, latch)
- Dedicated **chorus** (ensemble) stage before the FX rack
- Reorderable **FX rack** (Distortion, Compressor, Delay, Reverb) on the synth output — not on the guitar input
- Factory **presets** including basses, leads, SuperSaw, pads, pluck, keys, organ, filter sweep, plus **Zimmer Brass**, **Zimmer Pad**, **Jupiter Brass**, and **Jupiter Strings**
- Live pitch display (Hz + note name), confidence meter, voiced indicator, latency readout
- Formats: **VST3**, **AU**, and **Standalone**

## Latency

Typical end-to-end latency at 48 kHz with a 128-sample buffer:

| Stage | Approx. |
|-------|---------|
| Input buffer | ~2.7 ms |
| Pitch analysis (2048-sample window) | ~15–20 ms |
| Output buffer | ~2.7 ms |
| **Total** | **~10–15 ms** |

### Tips for lowest latency

1. Use the **Standalone** app for live playing (no DAW overhead).
2. Set audio buffer size to **64** or **128** samples in the Standalone audio settings.
3. On first launch (macOS), macOS will ask for **Microphone** access — click **Allow**. If you previously denied it: **System Settings → Privacy & Security → Microphone → enable Guitar Synth**, or run `tccutil reset Microphone com.guitarsynth.plugin` and relaunch.
4. Enable the correct **input channels** for your guitar in Audio Settings (usually input 1 on your interface).
5. Use **headphones** when monitoring through speakers to avoid feedback (Standalone enables live input by default).
6. Disable direct monitoring on your audio interface if you hear a dry double.
7. Use a relatively clean guitar signal; heavy distortion makes tracking harder.
8. Adjust **Gate** (default -48 dB) and **Tracking** knobs if notes fail to trigger, or if you hear synth output while idle. Raise Gate toward -40 dB if the gate opens on interface noise.
9. Use **Retrigger** (default mid) so repeated picks of the same note re-attack the synth. Raise it if soft re-picks are missed; lower it (or set to 0 to disable) if you get double triggers from one pluck.

Run the automated checks after building (macOS build profile):

```bash
cmake --build build-mac --target GuitarSynthTests GuitarSynthProcessorTests GuitarSynthPitchTrackerTests
ctest --test-dir build-mac --output-on-failure
```

The plugin reports latency to the host via `getLatencySamples()` so DAWs can compensate.

## Requirements

- CMake 3.22+
- macOS 11+ (for AU + macOS Standalone)
- PC toolchain (Windows: MSVC + Visual Studio Build Tools) for VST3 + Standalone

Install tools if needed:

```bash
xcode-select --install
brew install cmake
```

## Build on macOS

```bash
cd "/Users/thomas/Documents/Data science/guitar_vst"
cmake -B build-mac -DCMAKE_BUILD_TYPE=Release
cmake --build build-mac --config Release -j
```

### Outputs (macOS)

- Standalone: `build-mac/GuitarSynth_artefacts/Release/Standalone/Guitar Synth.app`
- VST3: `~/Library/Audio/Plug-Ins/VST3/Guitar Synth.vst3`
- AU: `~/Library/Audio/Plug-Ins/Components/Guitar Synth.component`

## Build for PC (Windows/Linux)

```bash
cd "/Users/thomas/Documents/Data science/guitar_vst"
# Windows (example, x64):
cmake -B build-pc -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build-pc --config Release -j

# Linux: use your normal CMake generator (e.g. Ninja/Make) without -G/-A
```

### Outputs (PC)

- VST3 + Standalone: produced by the build and copied after build (see `COPY_PLUGIN_AFTER_BUILD TRUE` in `CMakeLists.txt`)
- AU: not built on PC (AU is macOS-only)

### Verification checklist

- macOS:
  - build `GuitarSynthTests`, `GuitarSynthProcessorTests`, and `GuitarSynthPitchTrackerTests`, then run `ctest --test-dir build-mac --output-on-failure`
  - confirm artifacts exist for `AU`, `VST3`, and `Standalone`
- PC: build `GuitarSynth` / `GuitarSynth_Standalone`
  - confirm `VST3` + `Standalone` artifacts exist
  - confirm no `AU` artifact is produced/installed

## Live setup

1. Connect guitar → audio interface input.
2. Open **Guitar Synth.app** (Standalone).
3. Select your interface as input and output in **Audio/MIDI Settings**.
4. Set buffer size to 128 samples (or 64 if your interface supports it).
5. Play single-note lines for best tracking; bends and hammer-ons are supported with glide enabled.
6. Pick a preset from the header menu, or open the **Filter Env** tab to shape filter sweeps by dragging the ADSR plots.

## Pitch tracking (YIN + OctaveNet)

Period candidates come from classic YIN. A small MLP (`OctaveNet`, weights baked into `Source/OctaveNetWeights.h`) scores those candidates using CMNDF depths, harmonic clarity, and previous pitch — reducing classic A3↔A2 octave errors without an ONNX runtime.

To retrain on synthetic (or later, your own) data:

```bash
cd "/Users/thomas/Documents/Data science/guitar_vst"
python3 -m venv ml/.venv
ml/.venv/bin/pip install -r ml/requirements.txt
ml/.venv/bin/python ml/train_octave_net.py
```

Rebuild the plugin after regenerating weights.

## Parameters

| Parameter | Description |
|-----------|-------------|
| Waveform (Osc 1 / 2) | Sine, saw, square, or triangle |
| Pulse Width (Osc 1 / 2) | Square duty cycle (PWM); ignored for other waves |
| Sub / Noise | −1 octave sub level and noise bed into Osc 1 |
| Osc 2 Mix / Octave / Detune | Second oscillator balance and pitch offset |
| Voices / Unison Detune / Spread / Blend | Per-oscillator unison stack (1–8 voices) |
| Cutoff / Resonance | Per-osc low-pass filter |
| Amp ADSR | Per-osc amplitude envelope (attack up to 5 s) |
| Filter Env ADSR + Amount | Per-filter envelope (plot editor); Amount opens/closes cutoff |
| Glide | Portamento between detected pitches |
| Master | Output level |
| Tracking | Pitch confidence threshold (higher = stricter) |
| Gate | Input level threshold in dB |
| LFO 1 / 2 | Rate, shape, and destinations (cutoff, resonance, pitch, amp, PWM) |
| Arpeggiator | On/Sync/Latch, rate or host division, gate %, mode, octaves, chord type |
| Chorus | Pre-rack ensemble (enable, rate, depth, mix) |
| Distortion / Compressor / Delay / Reverb | Post-synth FX rack (enable, mix, and effect-specific controls); drag cards or use arrow buttons to reorder |
| FX Order | Chain order of the four effects (defaults Dist → Comp → Delay → Reverb) |

FX process the **synth stereo output** only. Keep guitar input relatively clean for reliable pitch tracking (do not rely on Distortion as an input drive).

## License

Built with [JUCE](https://juce.com). JUCE is free for personal and open-source projects; commercial distribution requires a JUCE license.
