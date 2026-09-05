# AERIFORM - Task Tracker

Living document. Updated as work progresses.

## v0.2 overhaul: dual exciters, wavefolder, resonator network

### Baseline (recorded 2026-09-04 before any v0.2 change)
- Build: OK (GCC 14.2 MinGW-w64, Release, `D:\dev\build\aeriform\mingw-release`)
- `AeriformTests --all`: 38 tests, 3690 checks, 0 failures
- `AeriformHostCheck ... AERIFORM.vst3`: PASSED; pluginval strictness 10: SUCCESS
- CPU: 8 voices + effects @ 48 kHz / 256 = 3.7 % real time; 16 voices (unison 2) = 6.3 %
- 122 parameters, state version 1, 20 factory presets
- Git: repository initialised at this baseline (commit "Checkpoint: AERIFORM v0.1.0 baseline")

### Architectural plan
Per-voice chain becomes:
```
Exciter A --+                                                        +-- Resonator A --+
            +-- Interaction -- Pre-shaper -- Wavefolder(2x/4x) -- Dyn --+-- Resonator B --+-- Body -- Fader -- Pan
Exciter B --+        ^                                                  +-- Resonator C --+
                     +---------------- Energy loop (optional) -------------------+
```
- Exciter chain runs at the oversampled rate (2x Normal/Eco, 4x High) and is decimated with a polyphase halfband before the network, so oscillators, PM/FM interaction and the folder are all band-limited by the same filter.
- Exciter slot models: Off, Breath (the v0.1 exciter, unchanged behaviour), Wave (PolyBLEP morph, PW, sub, phase distortion, sync), Complex (original coupled phase-feedback "orbit" oscillator with bounded chaotic map), 12 noise models, 8 physical exciters (reed, lip, bow, jet, mallet, pluck, scrape, impact), Sidechain.
- Interaction: 13 modes with a central INTERACTION control, balance, depth, B->A / A->B, DC block, normaliser, drive.
- Pre-shaper: the existing exciter LP/HP/key-track parameters plus band-pass, resonance, drive, bias, slew, transient emphasis, envelope amount, before/after folder order.
- Wavefolder: 7 original fold modes, drive, symmetry, bias, 1-4 stages, shape, mix, compensation, post LP; polyphase IIR halfband oversampling.
- Resonator network: three slots (A keeps every existing `res_*` ID), 9 resonator types (waveguide family + comb + modal family + formant), Single / Serial / Parallel / Hybrid routing, 6 cross-feedback routes with shared delay / filter / drive / polarity, injection point, output tap, wet/dry, Repipe macro, energy governor.
- Energy loop: off by default, tanh-bounded, filtered, delayed, governed.
- Matrix: 16 slots, +15 sources, +29 destinations.
- Parameters: all 122 existing IDs preserved with identical meaning (`exc_lp` / `exc_hp` / `exc_keytrack` keep their place as the pre-shaper filter; `exc_reed` / `exc_pressure` stay Resonator A's reed junction; `res_mode` list is only appended). State version 2 with a migration hook; old presets / sessions load unchanged because every new parameter defaults to the v0.1 behaviour (Exciter A = Breath, B = Off, folder off, Single routing, loop off).

### Parameter inventory
- Preserved as-is (122): all `exc_*`, `env_*`, `art_*`, `res_*`, `lfo*`, `menv_*`, `mod1..8_*`, `chorus_*`, `delay_*`, `rev_*`, `voice_*`, `glide_*`, `unison_*`, `bend_range`, `mpe_enable`, `out_*`, `limiter_on`.
- Aliased / re-homed in the GUI only (no ID change): `exc_lp`, `exc_hp`, `exc_keytrack` -> PRE-SHAPER filter; `exc_reed`, `exc_pressure` -> Resonator A reed junction + Breath model; `exc_pluck`, `exc_pluck_len`, `exc_attack_click`, `exc_release_noise` -> Breath model one-shots.
- New (see docs/PARAMETERS.md after generation): `exa_*` / `exb_*` exciter slots, `mix_*` interaction, `pre_*` shaper, `wf_*` folder, `dyn_*`, `res_on/input/output/pan/width/pickup/inharm/size`, `rb_*`, `rc_*`, `net_*`, `loop_*`, `mod9..16_*`, `quality`.

## Completed
- (v0.1) Everything listed in the baseline above
- v0.2 plan, inventory and git checkpoint

## Current
- v0.2 implementation: parameter table refactor -> DSP -> tests -> GUI -> presets -> profiling -> docs

## Remaining
- See "Current"; optional follow-ups after v0.2: macOS/Linux/AU verification, MSVC verification

## Known issues / limitations
- Build directory must not contain `^` (cmd.exe escape) because of JUCE's post-build steps under Ninja; the scripts build under `D:\dev\build\aeriform` when the source path contains `^`
- MinGW build has no DirectWrite: JUCE falls back to GDI text rendering
- Resonator tuning is exact for the fundamental; autocorrelation-measured pitch is up to ~5 cents flat at C2 (partial stretch)
- Notes above ~5 kHz with maximum dispersion cannot be tuned exactly (clamped safely)
- Sidechain audio only sounds while notes are held (note-gated by design)
- AU target declared for macOS but not built here

## Build and test status
- Baseline above; v0.2 status is appended as phases complete
