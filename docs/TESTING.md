# Tests and validation

```powershell
D:\dev\build\aeriform\mingw-release\AeriformTests.exe            # unit tests (~1 min)
D:\dev\build\aeriform\mingw-release\AeriformTests.exe --smoke    # offline smoke / fuzz / profile tests (~6 min)
D:\dev\build\aeriform\mingw-release\AeriformTests.exe --all      # both
D:\dev\build\aeriform\mingw-release\AeriformTests.exe --filter=<substring>   # a subset (unit or smoke)
D:\dev\build\aeriform\mingw-release\AeriformTests.exe --params > docs\PARAMETERS.md   # regenerate the parameter reference
ctest --test-dir D:\dev\build\aeriform\mingw-release             # same via CTest
```

Environment knobs: `AERIFORM_FUZZ_SECONDS` (default 60) and
`AERIFORM_PROFILE_SECONDS` (default 4) lengthen the fuzz and the CPU profile.

## Unit tests

Cover tuning at three sample rates, boundedness, envelopes, effects,
parameters, state, presets, voices, MPE, sidechain, every exciter model
(finite and audible in both slots), seeded noise determinism, wave-oscillator
tuning and alias level, the complex oscillator's boundedness across chaos /
feedback / instability, every interaction mode in every model family,
wavefolder boundedness / DC removal / harmonic generation, halfband
oversampler pass-band and image rejection, quality switching during playback,
every resonator model tuned and distinct, every routing mode, every
cross-feedback route at maximum with both polarities, topology changes while
notes play (no clicks, no level explosion), the energy loop bounded at maximum
for every source / destination / polarity, Repipe transitions, voice stealing
under complex routing, state round trips of every parameter, old session and
preset files loading with values restored and new parameters at their
old-equivalent defaults, every factory preset loading and sounding,
sample-rate / block-size changes, rapid automation, the editor opening,
painting every page, binding every control to an existing parameter, and
surviving open/close cycles while playing — plus (v3) the movable filters'
tuning and routing, collision/contact energy bounds, the true stereo network,
the sympathetic bank's tuning and decay, the coupled room's energy budget,
the new effects (resonant delay, shimmer, spectral freeze, multiband
saturation), A/B morph interpolation and Deep mode, the randomizer's
determinism and locks, and undo/redo.

## Smoke tests

Three sample rates x three block sizes; extreme settings; preset and
parameter sweeps while playing; prepare / release cycling; a randomised fuzz
(every parameter, voice count, notes, sample rate, block size and sidechain
randomised for `AERIFORM_FUZZ_SECONDS` of audio, verifying finite, bounded and
DC-free output and reporting the worst block time); the CPU profile matrix
(see [PERFORMANCE.md](PERFORMANCE.md)); and the factory-preset level check.

## VST3 host check

Loads the built bundle through JUCE's VST3 hosting like a DAW would:

```powershell
cd D:\dev\build\aeriform\mingw-release
AeriformHostCheck_artefacts\Release\AeriformHostCheck.exe Aeriform_artefacts\Release\VST3\AERIFORM.vst3 --editor
```

## pluginval

Tracktion's validator, installed to `D:\dev\tools\pluginval`:

```powershell
D:\dev\tools\pluginval\pluginval.exe --strictness-level 10 --validate-in-process --validate D:\dev\build\aeriform\mingw-release\Aeriform_artefacts\Release\VST3\AERIFORM.vst3
```

## Last recorded results (v3.0.0)

| Check | Result |
|---|---|
| Unit tests | 134 tests, 17.1M checks, 0 failures |
| Smoke tests | 8 tests, 43K checks, 0 failures |
| VST3 host check | PASSED |
| pluginval strictness 10 | SUCCESS |

See [../TASKS.md](../TASKS.md) for the full development log and
[AUDIBILITY_RESULTS.md](AUDIBILITY_RESULTS.md) for the audio-comparison
methodology used to confirm the 40 original presets are unchanged.

---
Back to [README](../README.md).
