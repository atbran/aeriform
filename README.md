# AERIFORM

**An oscillator-free, breath-driven physical-modelling synthesizer.**
VST3 + Standalone, 8-voice polyphonic (up to 16), MPE, sidechain excitation.
C++20 / JUCE 7.0.12 / CMake.

![AERIFORM interface](docs/screenshot.png)

AERIFORM has no oscillators. Every sound starts as air: noise, steady breath
pressure, a pluck impulse, or audio you feed into the sidechain. That energy is
pushed into a tuned waveguide tube whose length follows the keyboard, and the
tube's own reflections, losses, dispersion and non-linear reed junction shape
the tone. Three LFOs, a modulation envelope and an 8-slot matrix animate it;
chorus, tempo-synced delay and an algorithmic reverb put it in a space.

It is inspired by the broad concept of exciter -> resonator -> space
instruments such as the Erica Synths Steam Pipe. It is an original design and
contains no code, presets, artwork or branding from any other product.

---

## Contents

1. [Features](#features)
2. [Building on Windows](#building-on-windows)
3. [macOS and Linux](#macos-and-linux)
4. [Installing and running](#installing-and-running)
5. [Tests and validation](#tests-and-validation)
6. [Playing it: sidechain, MPE, MIDI learn](#playing-it)
7. [Architecture](#architecture)
8. [DSP: how the tube works](#dsp-how-the-tube-works)
9. [Parameters](#parameters)
10. [Presets](#presets)
11. [Performance](#performance)
12. [Known limitations](#known-limitations)
13. [Licensing](#licensing)

---

## Features

- **Exciter**: white / pink breath noise with turbulence and slow drift, steady
  pressure, pluck / strike bursts, tongue transient, release puff, key-tracked
  low-pass and high-pass filters, velocity sensitivity, **sidechain audio in**.
- **Resonator**: fractional-delay waveguide with coarse / fine / length / key
  tracking, feedback, damping, brightness, dispersion (inharmonicity), bore
  shape (excitation position), end reflection, non-linear saturation with
  pressure bias, three topologies (open pipe / closed pipe / string), body
  (formant) filter with key tracking, sympathetic coupling between voices.
- **Reed junction**: a pressure-driven reed non-linearity at the mouth makes the
  pipe speak on its own (clarinet, sax, brass, whistle) without any oscillator.
- **Articulation**: pressure-dependent brightness, flow-to-pitch, pitch
  instability, per-voice component variation.
- **Breath envelope**: click-free ADSR with velocity-to-pressure.
- **Motion**: 3 LFOs (sine, triangle, saw up / down, square, S&H, smooth random;
  Hz or tempo-synced; free-running or retriggered; fade-in; start phase), a
  modulation ADSR, and an 8-slot matrix with 15 sources (LFOs, envelopes,
  velocity, mod wheel, aftertouch, pitch bend, MPE slide, key track, per-note
  random, breath CC2, expression CC11) and 23 destinations.
- **Voices**: 1-16 voices (8 default), poly / mono / legato, glide (always or
  legato-only), unison 1-4 with detune and stereo spread, pitch-bend range,
  sustain and sostenuto pedals, channel and polyphonic aftertouch, **MPE**.
- **Space**: three-tap ensemble chorus, ping-pong / stereo tempo delay with tone
  control, 8-line FDN reverb with pre-delay, damping, width and internal motion,
  final high-pass and a soft peak limiter.
- **Presets**: 20 factory presets, user presets as portable XML files, save /
  save as / load / next / previous / init / import / export, dirty indicator.
- **GUI**: original vector interface, five regions, animated airflow-tube
  visualizer, value readouts while editing, tooltips on every control,
  double-click reset, shift-drag fine adjustment, right-click MIDI learn,
  modulation rings showing matrix depth and live modulation, 60-200 % scaling.
- **Engineering**: all 122 parameters automatable and serialised with a state
  version; no allocation / locks / I/O on the audio thread after `prepare`;
  denormal protection; NaN / Inf safety net; sample-rate and block-size
  independent; host bypass releases notes; automated tests, offline smoke tests,
  a VST3 host-load checker and pluginval.

---

## Building on Windows

### Prerequisites

| Tool | Version used | Notes |
|---|---|---|
| CMake | 3.22+ (3.31 bundled with the toolchain below) | |
| Ninja | 1.12 | bundled with the toolchain below |
| C++20 compiler | **GCC 14.2 MinGW-w64 (WinLibs, POSIX threads, UCRT)** or Visual Studio 2022 | |
| Git | any | to fetch JUCE if `external/JUCE` is absent |

No admin rights are required for the MinGW path. JUCE 7.0.12 is vendored in
`external/JUCE` (shallow clone). If the folder is missing, CMake fetches it
automatically with `FetchContent`.

### Toolchain setup (portable, no installer)

1. Download `winlibs-x86_64-posix-seh-gcc-14.2.0-mingw-w64ucrt-12.0.0-r3.zip`
   from <https://github.com/brechtsanders/winlibs_mingw/releases>.
2. Extract it so that `g++.exe`, `ninja.exe` and `cmake.exe` are in
   `D:\dev\tools\mingw64\bin` (any folder works: set the environment variable
   `AERIFORM_TOOLCHAIN` to that `bin` directory).

### Build (exact commands)

From the repository root, in PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1            # Release build
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Test      # build + unit + smoke tests
powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Config Debug
```

or from Git Bash:

```bash
scripts/build.sh                 # Release
scripts/build.sh Release --test  # build + tests
```

The scripts put the toolchain on `PATH`, choose the `mingw-release` /
`mingw-debug` CMake preset and build with Ninja. Equivalent manual commands:

```powershell
$env:PATH = "D:\dev\tools\mingw64\bin;" + $env:PATH
cmake --preset mingw-release -B D:\dev\build\aeriform\mingw-release
cmake --build D:\dev\build\aeriform\mingw-release --parallel
```

**Build directory location.** CMake's Ninja generator wraps JUCE's post-build
steps in `cmd.exe /C "cd /D <dir> && ..."`, which breaks when the path contains
`^` (cmd's escape character). The scripts therefore build under
`D:\dev\build\aeriform` when the source path contains `^`, and under `build\`
inside the repository otherwise. Override with `AERIFORM_BUILD_ROOT`.

### Visual Studio 2022

```powershell
cmake --preset msvc
cmake --build --preset msvc-release
```

(The MSVC path is configured but was not exercised on the development machine,
which had no Visual Studio installed.)

### Build outputs

| Artefact | Path (Release, MinGW preset) |
|---|---|
| VST3 bundle | `<build>/Aeriform_artefacts/Release/VST3/AERIFORM.vst3/` |
| Standalone | `<build>/Aeriform_artefacts/Release/Standalone/AERIFORM.exe` |
| Tests | `<build>/AeriformTests.exe` |
| Host checker | `<build>/AeriformHostCheck_artefacts/Release/AeriformHostCheck.exe` |

The Windows binaries are statically linked against the compiler runtime
(`-static -static-libgcc -static-libstdc++`), so the VST3 depends only on
system DLLs and the Universal CRT that ships with Windows 10 / 11.

---

## macOS and Linux

The project is generator-agnostic. Untested on those platforms here, but the
intended commands are:

```bash
cmake --preset unix-release          # Ninja, Release
cmake --build --preset unix-release
```

- macOS: Xcode command-line tools; the `AU` format is added automatically.
  Bundles land in `build/unix-release/Aeriform_artefacts/Release/{VST3,AU,Standalone}`.
- Linux: install the usual JUCE dependencies (`libasound2-dev libjack-jackd2-dev
  libfreetype6-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev
  libcurl4-openssl-dev libgl1-mesa-dev`), then the commands above.

---

## Installing and running

**VST3 (Windows):** copy the whole `AERIFORM.vst3` folder to
`C:\Program Files\Common Files\VST3\` (or configure `-DAERIFORM_COPY_PLUGIN=ON`
to copy after every build). Rescan plug-ins in your DAW; AERIFORM appears as an
instrument by "Aeriform Audio".

**Standalone:** run `AERIFORM.exe`. Use *Options -> Audio/MIDI Settings* to pick
the audio device, sample rate, buffer size and MIDI input. The standalone mutes
the audio input by default (feedback protection): untick *Mute audio input* to
blow into a microphone and let it drive the resonators (see Sidechain below).

---

## Tests and validation

```powershell
D:\dev\build\aeriform\mingw-release\AeriformTests.exe            # 33 unit tests
D:\dev\build\aeriform\mingw-release\AeriformTests.exe --smoke    # offline DSP smoke / stress tests
D:\dev\build\aeriform\mingw-release\AeriformTests.exe --all      # both
D:\dev\build\aeriform\mingw-release\AeriformTests.exe --params > docs\PARAMETERS.md   # regenerate the parameter reference
ctest --test-dir D:\dev\build\aeriform\mingw-release             # same via CTest
```

What the unit tests cover: MIDI-note-to-frequency, fractional-delay
interpolation accuracy, resonator tuning at 44.1 / 48 / 96 kHz across the range
(open pipe, closed pipe, string with dispersion), resonator boundedness under
extreme settings, ADSR shape, silence after release, effects finiteness,
parameter IDs / defaults / units / skews, choice lists vs enums, parameters
reaching the DSP, state round trip (parameters, MIDI learn, editor scale,
preset name), garbage and partial state, preset XML and file round trips,
every factory preset loading and rendering, polyphony and stealing, voice-count
limit, mono / legato note stack, glide, unison, sustain pedal, pitch bend and
bend-range changes, velocity and aftertouch, mod matrix routing, MIDI learn,
MPE per-note expression, sample-rate / block-size changes, bypass, and the
sidechain input exciting tuned resonators.

The smoke tests instantiate the processor at 44.1, 48 and 96 kHz with block
sizes 32, 256 and 1024, play an 8-note chord, render several seconds and verify
finite, bounded (limiter) and non-trivial output; drive every parameter to its
extremes (and all to minimum / maximum) with 8 notes across MIDI 0-127; change
presets and sweep parameters every block while playing; cycle
prepare / release; and measure CPU.

**VST3 host check** (loads the built bundle through JUCE's VST3 hosting like a
DAW would; stand-in for hosts you may not have):

```powershell
cd D:\dev\build\aeriform\mingw-release
AeriformHostCheck_artefacts\Release\AeriformHostCheck.exe Aeriform_artefacts\Release\VST3\AERIFORM.vst3 --editor
```

**pluginval** (Tracktion's validator, installed to `D:\dev\tools\pluginval`):

```powershell
D:\dev\tools\pluginval\pluginval.exe --strictness-level 5 --validate D:\dev\build\aeriform\mingw-release\Aeriform_artefacts\Release\VST3\AERIFORM.vst3
```

Last results (this build): strictness 5 **SUCCESS**, strictness 10 **SUCCESS**
(25 test groups including editor-while-processing, non-releasing processing,
state restoration, background-thread state, parameter thread safety and
parameter fuzzing). Steinberg's standalone `validator` is not bundled with
JUCE's VST3 SDK subset; pass `--vst3validator <path>` to pluginval if you have
the full VST3 SDK built.

---

## Playing it

**Sidechain / external audio.** The plug-in declares a stereo *Sidechain* input
bus (a VST3 aux input, so DAWs list it in their sidechain routing). Turn up
**Sidechain** in the BREATH section (parameter `exc_ext_in`): the input is
summed to mono, sent through the exciter filters and injected into the tube of
every note you hold, so the audio is forced to resonate at the notes you play.
Turn **Noise**, **Pluck** and **Pressure** down for a pure "vocoder-like"
resonator effect, or leave breath noise in for a hybrid. The resonators are
note-gated: with no note held nothing passes through.

- Ableton Live: add AERIFORM on a MIDI track, open the device's sidechain
  section and pick the audio track. Bitwig / Reaper / Cubase / Studio One:
  route an audio track's send or output into the instrument's sidechain input.
- Standalone: un-mute the audio input in the audio settings and blow into a mic.

**MPE.** Enable **MPE** in MASTER. The lower zone uses channels 2-16 with a
48-semitone per-note bend range; per-note pressure maps to *Aftertouch*, slide
(CC74) to *MPE Slide*, per-note pitch bend is applied directly. Route them in
the matrix.

**MIDI learn.** Right-click any knob -> *MIDI Learn*, move a controller.
Mappings are saved with the session / preset state. Right-click again to
clear. CC1 (mod wheel), CC2 (breath) and CC11 (expression) are always available
as matrix sources unless a learned mapping claims them.

**Fine control.** Shift-drag = fine; Ctrl-drag = velocity mode; double-click =
default; mouse wheel works on every knob.

---

## Architecture

```
Source/
  Plugin/        PluginProcessor (buses, state, bypass), PluginEditor (layout, scaling, timers)
  Params/        ParamIDs.h (stable IDs), ParameterLayout (APVTS layout, units, skews, tooltips, enums)
  DSP/
    DspUtils.h        one-pole, SVF, DC blocker, allpass, noise, tanh, ramps
    FractionalDelay.h 4-point Lagrange delay line
    Envelope.h        click-free ADSR
    LFO.h             multi-shape LFO with fade-in
    Exciter           breath / pluck / transient / sidechain excitation
    Resonator         phase-compensated waveguide tube, reed junction, body filter
    ModMatrix.h       routing evaluation
    Voice             exciter -> tube -> body -> fader -> pan, per-voice LFOs / envelopes / modulation
    SynthEngine       voice allocation, MPE / MIDI, mono / legato / unison, global modulation, effects
    Effects/          Chorus, Delay, Reverb (FDN), OutputStage (HP + limiter)
  Presets/       PresetManager (files, XML, dirty state), FactoryPresets (20 patches)
  MIDI/          MidiLearn (lock-free CC -> parameter mapping)
  Visualization/ VisualizerModel (atomics + ring buffer bridge to the GUI)
  GUI/           Theme, LookAndFeel, Knob, ParamControls, Section/PanelBase, Visualizer,
                 PresetBar, ModMatrixPanel, Panels/{Breath,Resonator,Motion,Space,Master}
Tests/           TestFramework (tiny harness), TestHelpers (host driver), Dsp/Parameter/State/Voice/Sidechain/Smoke tests
Tests/HostCheck/ AeriformHostCheck (JUCE VST3 host that loads the built bundle)
```

**Threading.** The audio thread reads parameters through `std::atomic<float>`
pointers once per block, copies them into a plain `VoiceParams` struct and never
touches the message thread. All buffers, delay lines and voices are allocated in
`prepare`. The GUI polls a `VisualizerModel` of relaxed atomics and a
single-producer ring buffer at 30 Hz. MIDI learn uses atomics; the only
"lock" on the audio thread is the uncontended `CriticalSection` inside JUCE's
`MPEInstrument`, which is touched from the audio thread only.

**Sample-accurate MIDI.** Each block is rendered in segments between MIDI
events; voices run a 32-sample control rate (modulation, coefficients, pitch)
inside sample-accurate audio processing with per-sample ramps for gains and
delay length.

**Voice stealing.** A stolen voice is faded out over 3 ms and the new note is
queued; it starts the moment the fade completes (checked at every segment
boundary), so stealing never produces a discontinuity.

---

## DSP: how the tube works

### Exciter

`Exciter` generates, per sample: white noise (xorshift) crossfaded with pink
(Kellet filter), multiplied by a slow chaotic turbulence (two band-limited
random sources), a slow breath drift and the per-sample breath envelope; plus
three one-shot components: a pluck burst (white noise with exponential decay
over *Pluck Length*), a tongue transient (short bright click at note-on) and a
release puff (pink noise burst at note-off). The sidechain sample is added,
scaled by *Sidechain* and the envelope. Everything passes through a key-tracked
SVF high-pass then low-pass; *Press > Bright* opens the low-pass with pressure.

### Resonator (waveguide)

Per sample, with `L` the loop length in samples:

```
d   = delay.read(L)                       4-point Lagrange fractional read
d   = reflection(d)                       end reflection: blend of d and a one-pole low-pass (HF radiated from the open end)
d   = dampingLP(d)                        frequency-dependent loss (key-tracked cutoff)
d   = ksAverage(d)          (string only) two-point average, extra HF loss
d   = allpass x4 (d)        (dispersion)  first-order allpasses: inharmonic partials
d   = dcBlock(d)                          1.5 Hz high-pass
sat = (tanh(d*drive + bias) - tanh(bias)) / drive      bounded saturation, pressure bias => even harmonics
refl= sat * g * polarity                  g in [0.70, 1.00], polarity -1 for the closed pipe
in  = tilt(excitation)                    brightness tilt (+/- 9 dB above ~1 kHz, key-tracked)
in -= 0.85 * exciteDelay.read(pos * L)    excitation-position comb (Shape)
in  = 2*tanh(in/2)                        the mouth cannot inject unbounded pressure
x   = lerp(refl + in,  refl + (1.2*pressure + in - refl) * r,  reed)      reed junction, r = clamp(0.7 - 0.3*dp)
delay.write(x)
out = body(x)                             SVF band-pass formant mixed in
```

### Tuning and stabilisation

**Tuning.** The loop length is `L = period - tau`, where `period = fs / f0`
(open pipe, string) or `fs / (2 f0)` (closed pipe, inverted feedback, odd
harmonics), and `tau` is the *phase delay at f0* of every in-loop filter,
computed analytically each control block: the damping one-pole, the
reflection blend, the two-point average, the four allpasses and the DC blocker.
The Lagrange interpolator's phase delay equals its fractional delay to within
0.01 samples at musical frequencies. The result is tested at 44.1 / 48 / 96 kHz
across C2-C8: the fundamental is exact to better than 1 cent from C3 upwards;
an autocorrelation pitch estimate reads up to about 5 cents flat at C2 because
the damping filter stretches the upper partials slightly (the same effect a
real pipe or string shows). Length changes (glide, modulation, flow-to-pitch,
instability) are smoothed per sample with a 2.5 ms one-pole so pitch moves
without zipper noise; a new (non-legato) note snaps the length and clears the
tube.

**Stability.** The linear loop gain never exceeds 1.0 and every in-loop filter
is passive, so the loop cannot grow on its own; the saturator bounds the
circulating signal; the injected excitation is soft-limited; the reed junction
is bounded by construction (`|r| <= 1`). Blown self-oscillation comes from
*Reed* + *Pressure* (the reed table turns steady pressure into a sustained
oscillation exactly like a clarinet model) rather than from over-unity
feedback, which is what makes it controllable and bounded. On top of that,
every voice checks its resonator state for NaN / Inf after each block and
flushes itself if needed, the engine scans the final mix and resets everything
if a non-finite sample slips through, the output stage clamps to +/- 4 even
with the limiter off, and `ScopedNoDenormals` is active for the whole block.
The extreme-settings tests drive feedback, saturation, reed, pressure,
dispersion, body resonance and coupling to their limits with 16 notes from MIDI
0 to 127 at 96 kHz and verify bounded, finite output.

### Articulation

- *Flow > Pitch*: pitch offset proportional to (envelope - sustain), so the
  pitch scoops into the note on the attack and sags on release, staying in tune
  while sustained.
- *Instability*: 0.7 Hz band-limited random pitch wander.
- *Variation*: each of the 16 voices has fixed random tuning / damping /
  brightness / shape offsets scaled by this amount.
- *Coupling*: a fraction of the previous segment's summed voice output is fed
  into every tube's excitation (sympathetic resonance).
- Reed pressure and the saturator bias follow the breath envelope per sample.

### Effects

Chorus: three modulated Lagrange taps per channel with quadrature LFOs (width
sets the L/R phase offset). Delay: two 2.6 s lines, one-pole tone + high-pass in
the feedback path, tanh-bounded feedback up to 95 %, ping-pong cross-feed, slow
(1.5 Hz) time smoothing for tape-like changes, tempo sync from the host
`PositionInfo`. Reverb: 4 Schroeder allpass diffusers into an 8-line Householder
FDN with per-line damping, T60-derived gains (0.4 s-24 s), slow modulation of
four lines, pre-delay and mid/side width. Output: 2-pole high-pass, smoothed
gain, peak follower limiter (instant attack, 120 ms release) with a gentle
saturation stage.

---

## Parameters

The complete reference (ID, range, default, description for all 122
parameters) is generated from the layout: [docs/PARAMETERS.md](docs/PARAMETERS.md).
IDs are stable; never rename them.

Display units are derived from DSP values (Hz, ms, %, dB, semitones, cents,
degrees) and the same formatting is used by the host's automation lane and the
plug-in's own knobs.

---

## Presets

- Factory presets are compiled in (`Source/Presets/FactoryPresets.cpp`) as
  sparse parameter lists: Init, Airy Flute, Warm Wooden Pipe, Reed Song,
  Cathedral Organ, Plucked Tube, Glass Resonator, Brass Horn, Bass Pipe,
  Evolving Drone, Dark Cinematic Pad, Metallic Ambience, Unstable Feedback,
  Soft Breath Pad, Percussive Click, Noise Machine, Whistle Lead, Sub Drone
  Engine, Ceramic Bells, Steam Vent.
- User presets: `Documents\Aeriform\Presets\*.aerpreset` (portable XML with a
  `version` attribute, name and category). Unknown parameters are ignored and
  missing ones take their defaults, so files stay forward and backward
  compatible.
- The preset menu (click the name) lists presets by category and offers
  load / export file, opening the folder and deleting user presets.

---

## Performance

Measured by `AeriformTests --smoke` on an Intel i5-11600K, Release build,
GCC 14.2, 48 kHz, 256-sample blocks:

| Load | Fraction of real time |
|---|---|
| 8 voices, chorus + delay + reverb | 4.0 % |
| 16 voices (unison 2), effects | 6.7 % |

The editor repaints at 30 Hz; the visualizer caches its static artwork and
does no DSP on the message thread.

---

## Known limitations

- Notes above roughly 5 kHz with maximum dispersion cannot be tuned exactly
  (the allpass chain's delay exceeds the loop length); the loop length is
  clamped safely instead.
- The autocorrelation-measured pitch reads a few cents flat below C2 (partial
  stretch from the loop filters); the fundamental itself is exact.
- Sidechain audio only sounds while notes are held.
- The MinGW build renders text with GDI (no DirectWrite); an MSVC build looks
  marginally crisper.
- macOS / Linux / AU builds are configured but untested on the development
  machine.
- `MPEInstrument` (JUCE) may allocate its small note array on the very first
  notes; this is JUCE's own behaviour and settles immediately.

---

## Licensing

AERIFORM's code is MIT licensed. Binaries include JUCE (GPLv3 / commercial) and
the Steinberg VST3 SDK (GPLv3 / proprietary), so **distributed binaries are
GPLv3 unless you hold a JUCE commercial licence and the Steinberg VST3
licence**. See [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md) for details,
including the splash-screen rule for JUCE Personal.
