# PIPE P1 listening prototype

Experimental, standalone, unmerged. This folder is not part of the plugin build.
It reads the existing `Source/DSP/FractionalDelay.h` directly; it changes no plugin
DSP, model enum, parameters, presets, UI, allocator or Tests/ files.

## Build and reproduce

Requires C++20, CMake, and Python with NumPy (no SciPy/JUCE required).

```powershell
cmake -S Repo/prototypes/pipe -B Repo/prototypes/pipe/build -G Ninja `
  -DCMAKE_CXX_COMPILER=D:/dev/tools/mingw64/bin/g++.exe `
  -DCMAKE_MAKE_PROGRAM=D:/dev/tools/mingw64/bin/ninja.exe `
  -DCMAKE_BUILD_TYPE=Release
cmake --build Repo/prototypes/pipe/build
Repo/prototypes/pipe/build/pipe-verify.exe
Repo/prototypes/pipe/build/pipe-render.exe Repo/prototypes/pipe/output
python Repo/prototypes/pipe/analyze.py Repo/prototypes/pipe/output
```

Use the configured Python runtime if `python` is only a Windows Store alias.
The numeric verifier emits errors/magnitudes, not an acceptance verdict. All raw
renders are preserved as little-endian float32; analysis refuses silent clipping.
The JSON hash manifest supports deterministic rerender comparison.

After the acceptance report's tuning failure, generate revised evidence without
overwriting the original `output/` directory:

```powershell
Repo/prototypes/pipe/build/pipe-render.exe Repo/prototypes/pipe/output-retuned
Repo/prototypes/pipe/build/pipe-verify.exe Repo/prototypes/pipe/output-retuned/extreme-grid.csv
python Repo/prototypes/pipe/analyze.py Repo/prototypes/pipe/output-retuned --baseline Repo/prototypes/pipe/output
```

The new `PeakTuning.h` aligns a small-signal response maximum rather than only its
phase. `renders.csv` now includes the old phase-only length and `peak_tuned` flag.
`tuning-before-after.csv` compares the same 72 configurations. `source-sha256.json`
identifies the actual uncommitted source used; the repository HEAD alone does not.
The optional verifier CSV records the 972 extreme analytical configurations.

## Audition

`pressure-audition-matched.wav`: six 3-second notes with 0.5-second gaps. First
short RT (0.15 s), then long RT (3 s); each group uses pressure 0.08, 0.30, 0.80.
Notes start at 0, 3.5, 7, 10.5, 14 and 17.5 seconds. All six have the same steady
RMS. This is RMS matching, not a perceptual loudness standard.

`sweep-audition-raw.wav`: 8-second pressure-only and macro sweeps at short RT,
then the same pair at long RT, separated by 0.5 seconds. The macro also changes
DC/noise and exciter cutoff. These sweeps are deliberately unnormalized.

`control-*.wav` sweeps each continuous provisional control separately at C5,
velocity 0.8. Endpoint-B sweeps activate morph=1. `bore-*.wav` compares both
polarities as separate notes, not an unimplemented live mode switch.
`tracking-*.wav` repeats each tracking control at notes 48/72 and velocities 0.2/0.9.

## Limits

- Output is a proposed topology, not a verified realistic wind instrument. Pure
  DC is not promised to sustain a tone. P1 needs a human listening verdict.
- Render fixture has its own simple envelope. Production will use the existing
  shared envelope and fader, whose integration is not exercised here.
- Controls move slowly at 64-sample boundaries. Production automation smoothing,
  switching, nonfinite parameter handling and network coupling are not implemented.
- Tuning CSV reports a nearby impulse-response spectral peak and whether an
  interior peak exists; this is not a nonlinear oscillator pitch detector.
- A bounded peak search can fail at very lossy settings. The phase-based fallback
  is explicitly flagged, not reported as tuned. This and solver cost must be
  resolved before any production-wide tuning claim.
- Decay is a -5 to -25 dB energy-decay extrapolation, not exact full-engine T60.
- No conclusions about aliases, full-plugin CPU, DAW automation or release safety
  follow from this standalone probe.
