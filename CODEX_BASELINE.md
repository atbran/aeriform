# Verified copied baseline - 2026-09-05

## Provenance and isolation

Authoritative source confirmed by user: C:\Users\The Nerd^2\Documents\Synth claude code.
Source commit: cd13ed0db3e57a48d0457ebba2963cf304e00537 (v2.1 rename), following GUI preset-browser fix 91707d7 and completed network work e8a9819.
Original Git status was clean before and after cloning. No edits or builds occurred in the original.
Independent local clone (--no-hardlinks): D:\dev\build\gpt-aeriform-test, branch codex/experimental-aeriform. Destination existed but was empty. JUCE source copied separately because external/JUCE is ignored. No disposable original build artifacts copied.
Build directory: D:\dev\build\gpt-aeriform-test\build\mingw-release.

## Verified build and tests

- C++20, JUCE 7.0.12, GCC 14.2 MinGW-w64, Ninja, Release.
- Every baseline CMake target built successfully; subsequent build exits 0 with no work to do.
- VST3 and standalone, AeriformTests, AeriformHostCheck and VST3 manifest helper produced.
- Complete --all run: 72 tests (64 unit plus 8 smoke), 9809 checks, zero failures.
- Random fuzz: 60 seconds rendered, 16 random configurations; worst reported block 6.41 ms / 278.0 percent of its real-time budget. This is an observed outlier, not a claim that all worst-case blocks meet real time.
- Host check: PASSED, including 44.1/48/96 kHz at 32/256/1024 samples and state restoration.
- Pluginval strictness 10, in-process validation: SUCCESS, process exit 0. Explicit Start-Process waiting and stdout capture used because direct PowerShell invocation produced an empty log.
- Parameter table: 353 stable parameters, 120 inherited v0.1 IDs. Factory bank: 40 presets; full bank load/ID checks pass. State format 2.
- Logs: build/baseline-build.log, baseline-build-verify.log, baseline-tests.log, baseline-host.log, baseline-pluginval-stdout.log and baseline-pluginval-stderr.log.

## Measured CPU

48 kHz / 256 samples, percentage of real-time rendering budget, Eco / Normal / High:

| Configuration | Eco | Normal | High |
|---|---:|---:|---:|
| Default, 8 voices | 5.8% | 8.2% | 11.2% |
| All existing features, 8 voices | 23.4% | 24.5% | 39.1% |
| All existing features, 16 voices | 45.3% | 48.9% | 77.7% |

Reported plugin latency is zero samples. Additional latency and memory measurements will be added for new DSP.

## Limiter and preset audit

Actual output order: global effects -> high-pass/DC removal -> smoothed out_gain -> peak-follower gain reduction (threshold .92) -> safety tanh. Contrary to an assumption in the takeover text, out_gain is BEFORE this final limiter. Disabling the limiter still clamps output to +/-4.

Existing preset test uses max(RMS, peak/4) compared with a shared median, measured from note-on over 1.2 seconds with effects off. It still lacks explicit preset classes and true pre-limiter telemetry and must be replaced. Repeated measured peak ~.77 in Orbit Crossmod, Mallet Triad, Metallic Steam, Granular Wind, Serial Bodies and Resonator Cloud is consistent with the final transfer ceiling. Peak alone does not establish continuous saturation. Exact limiter occupancy and pre-limiter energy are the next diagnostic checkpoint before any preset adjustments. Original factory presets will remain intact; any corrections become new variants.

## Baseline technical debt found by inspection

- PresetManager parameter callback queues allocating callAsync GUI work and captures raw this; dirty/applying flags are not atomic.
- Parameter metadata is a shared vector rebuilt whenever another plugin instance is constructed; GUI retains pointers into it. Knob display-name overrides mutate this shared metadata.
- Oversized process blocks allocate a temporary MIDI buffer and lose external input in recursive processing.
- JUCE MPEInstrument uses an internal CriticalSection even though calls are audio-thread-owned. Absolute no-lock claims are therefore not currently justified.
- Existing development helpers: scripts/gen_params.py, build scripts, AERIFORM_PAGE screenshot hook and profile environment controls. No unexplained patch scripts found in tracked files.
- JUCE/MinGW warnings: GDI text fallback (no DirectWrite), deprecated tzname and large translation-unit indentation analysis. No baseline build failures.

This is a tested source checkpoint, not a claim that the requested expansion or the new diagnostic matrix is finished.

## Diagnostic checkpoint after baseline

Fresh-instance class-aware measurements now include pre/post-limiter RMS/peak/DC, attack, limiter occupancy, ceiling occupancy, release observation (capped at six seconds) and CPU. Normal effects are preserved. This differs from the old shared-instance, effects-disabled test; their level values are not directly comparable. Orbit Crossmod, Metallic Steam, Mallet Triad and Serial Bodies had zero limiter occupancy in this two-second chord probe. Resonator Cloud had 8.1% gain-reduction occupancy and .06% near-ceiling occupancy, pre peak 1.227. No evidence here justifies changing the original factory presets. More parameter extremes and listening remain necessary.

Reliability checkpoint: 75 tests, 51896 checks, zero failures. Immutable metadata and per-control labels, atomic preset dirty polling, allocation-free oversized-block subdivision with preserved input/MIDI timing, and output telemetry regression tests pass. No saturation correction applied. See build/reliability-tests.log.
