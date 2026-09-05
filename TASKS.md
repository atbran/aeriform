# AERIFORM - Task Tracker

Living document. Updated as work progresses.

## Completed
- Environment: no compiler was present; installed portable WinLibs GCC 14.2 (POSIX/UCRT) + Ninja 1.12 + CMake 3.31 to `D:\dev\tools\mingw64` (user scope, no admin)
- JUCE 7.0.12 vendored in `external/JUCE` (last JUCE line with MinGW support; JUCE 8 needs MSVC/clang-cl)
- Phase 1 foundation: CMake project (VST3 + Standalone, AU on macOS), presets file, build scripts, stable parameter IDs (122 parameters), APVTS state with version field, MIDI-learn/editor-scale/preset-name in state, malformed-state tolerance
- Phase 2 voice: exciter (white/pink noise, turbulence, pluck bursts, tongue transient, release puff, key-tracked LP/HP, sidechain input), waveguide resonator (4-point Lagrange fractional delay, end-reflection shelf, damping LP, dispersion allpass chain, DC blocker, bounded saturator with pressure bias, reed junction, position comb, body SVF), phase-delay-compensated tuning, click-free ADSR + output fader
- Phase 3 polyphony: 16-voice pool (1..16 selectable, 8 default), oldest-releasing/oldest-playing stealing with 3 ms fade-then-retrigger, MPEInstrument-based MIDI (legacy + MPE lower zone), sustain/sostenuto, velocity, per-note pressure/slide/bend, bend range without note drops, mono/legato with note stack, glide, unison 1-4 with detune/spread
- Phase 4 modulation/effects: 3 LFOs (7 shapes, sync, retrigger/free, fade, phase), mod envelope, 8-slot matrix (15 sources, 23 destinations), ensemble chorus, ping-pong tempo delay, 8-line FDN reverb, output HP + soft limiter, NaN safety net
- Phase 5 GUI: custom look-and-feel, five regions (BREATH / RESONATOR / MOTION / SPACE / MASTER), airflow-tube visualizer, knobs with value readout, double-click reset, shift fine-adjust, right-click MIDI learn, tooltips, teal modulation rings with live value, preset bar (prev/next/menu/save/save-as/init/import/export), uniform scaling 60-200 %
- Phase 6: 20 factory presets, 33 unit tests + 5 smoke/stress tests + 2 sidechain tests, VST3 host-load checker (scan, instantiate x2, 3 sample rates x 3 block sizes, state round trip, editor open/close x3 with audio running), CPU measurement, pluginval installed to `D:\dev\tools\pluginval`
- Sidechain: "Sidechain" input bus declared as VST3 aux input (`getPluginHasMainInput() = false`), audio is injected into every playing note's exciter and rings the tuned tubes (note-gated)

## Current
- Done. Remaining items are optional follow-ups.

## Remaining
- Optional: macOS/Linux/AU build verification (untested here: no such machine), MSVC build verification, a 'sidechain always on' (non-gated) input mode, oversampling of the reed junction

## Known issues / limitations
- Build directory must not contain `^` (cmd.exe escape) because of JUCE's post-build steps under Ninja; the scripts automatically build under `D:\dev\build\aeriform` when the source path contains `^`
- MinGW build has no DirectWrite: JUCE falls back to GDI text rendering (slightly softer fonts than an MSVC build)
- Resonator tuning is exact for the fundamental; the autocorrelation-measured pitch is up to ~5 cents flat at C2 and below because the in-loop damping filter stretches upper partials slightly (physically plausible, like a real pipe)
- Notes above ~5 kHz with maximum dispersion cannot be tuned exactly (allpass chain delay exceeds the loop length); this is clamped safely
- Linear loop gain is capped at exactly 1.0; sustained self-oscillation is produced by the reed junction (Reed + Pressure), not by over-unity feedback
- MPEInstrument allocates a small note array on first use (JUCE behaviour); no allocation after the first few notes
- Sidechain audio only sounds while notes are held (the resonators are note-gated by design)
- AU target is declared for macOS but has not been built or tested on a Mac

## Build and test status (Windows, GCC 14.2 / MinGW-w64, Release)
- `cmake --preset mingw-release -B D:/dev/build/aeriform/mingw-release && cmake --build D:/dev/build/aeriform/mingw-release --parallel`: OK
- `AeriformTests`: 33 tests, 0 failures
- `AeriformTests --smoke`: 5 tests, 0 failures; 8 voices + all effects @ 48 kHz / 256 = 4.0 % of real time; 16 voices (unison 2) = 6.7 %
- `AeriformHostCheck AERIFORM.vst3 --editor`: PASSED
- `pluginval --strictness-level 5`: SUCCESS (scan, open cold/warm, info, programs, editor, editor whilst processing, audio processing at 44.1/48/96 kHz x 64..1024, state, automation, editor automation, automatable parameters, buses)
- `pluginval --strictness-level 10`: SUCCESS (adds non-releasing processing, state restoration, parameters, background-thread state, parameter thread safety, parameter fuzzing) - 25 test groups, 0 failures
- Steinberg's own `validator` was not run: it is not bundled with JUCE's VST3 SDK subset and pluginval skips it unless given a path
