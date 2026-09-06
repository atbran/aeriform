# EXP_Aeriform Windows x64 test build

This directory contains the standalone EXE and complete VST3 bundle built from the source in this experimental checkpoint. The plugin version remains 2.1.0; use this commit and the hashes to identify the exact test build.

## Download and run

Download EXP_Aeriform-Windows-x64.zip and extract it. Run EXP_Aeriform.exe for standalone testing. For Ableton, copy the entire EXP_Aeriform.vst3 directory to your VST3 plug-in folder and rescan plug-ins. Preserve its Contents directory; the inner file alone is not the complete bundle.

EXP_Aeriform has its own name and VST3 identity, separate from the original AERIFORM. Earlier EXP_Aeriform builds use this same experimental identity. This repository does not install or replace your plug-ins automatically.

## Included

- 492 host parameters, A/B parameter/deep morph, seeded randomize/mutate/locks, undo and favourites.
- Three movable filters; collision routing; independent physical stereo; shared sympathetic bank; bounded coupled room.
- Resonant delay and shimmer, with dedicated controls under SPACE.
- Spectral-freeze DSP through host parameters. Its dedicated UI is unfinished. Enable Spectral Freeze Enabled, then use Spectral Hold or Capture Spectrum; Release Spectrum returns to live audio. Blur, shift, random phase, decay and mix are exposed. Captured sound is not saved with sessions.
- Original factory presets retained, with Reed in a Small Room and Modal Echo Pluck added.

## Validation

All targets built. Complete suite: 130 tests, 8957623 checks, zero failures. VST3 host check PASSED at 44.1/48/96 kHz with 32/256/1024-sample buffers. Pluginval strictness 10 SUCCESS. Full logs are in validation/.

Repeated spectral captures caused zero intercepted C++ allocations or frees in the focused test. This is not a whole-plugin allocation guarantee: the remaining MPE/controller realtime audit and final all-feature performance measurements are pending.

## Still in progress

Dedicated spectral-freeze page, multiband saturation, separate normal-input FX target and final realtime/performance audit. Room-to-resonator feedback is intentionally conservative and can be subtle. Deep morph runs two engines and costs more CPU. The Repipe pitch-reset report was not reproduced at parameter/DSP-target level; coupled resonators can have a different dominant audible peak.

SHA256SUMS.txt verifies the files inside the test package. ZIP-SHA256.txt verifies the downloadable archive. See the repository's docs/ and TASKS.md for implementation details and limitations.
