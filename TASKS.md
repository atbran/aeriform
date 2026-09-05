# AERIFORM - Task Tracker

Living document. Updated as work progresses.

## v0.2 overhaul: dual exciters, wavefolder, resonator network - DONE (2026-09-05)

### Baseline (recorded 2026-09-04 before any v0.2 change)
- Build: OK (GCC 14.2 MinGW-w64, Release, `D:\dev\build\aeriform\mingw-release`)
- `AeriformTests --all`: 38 tests, 3690 checks, 0 failures
- `AeriformHostCheck ... AERIFORM.vst3`: PASSED; pluginval strictness 10: SUCCESS
- CPU: 8 voices + effects @ 48 kHz / 256 = 3.7 % real time; 16 voices (unison 2) = 6.3 %
- 120 parameters, state version 1, 20 factory presets
- Git: repository initialised at this baseline (commit "Checkpoint: AERIFORM v0.1.0 baseline")

### Final status (v0.2.0)
- Build: OK, VST3 + Standalone + tests + host checker
- `AeriformTests`: 64 unit tests, 0 failures; `--smoke`: 8 tests, 0 failures (fuzz 60-93 s over 16-25 random configurations)
- Host check (VST3 load, prepare, notes, render at 3 rates, state, editor): PASSED
- pluginval strictness 10 (`--validate-in-process`): SUCCESS
- 353 parameters (120 v0.1 IDs preserved), state version 2, 40 factory presets, five-page GUI
- CPU: see docs/PERFORMANCE.md (default patch 5.6 / 7.9 / 10.7 % Eco / Normal / High, 8 voices)
- Git: three commits on top of the baseline (parameter table, DSP, GUI + tests + docs)

### Architectural plan (as built)
```
Exciter A --+                                                          +-- Resonator A --+
            +-- Interaction -- Pre-shaper -- Wavefolder -- Dynamics -- >|   Resonator B   |-- Body -- Fader -- Pan
Exciter B --+   (oversampled 1x / 2x / 4x, decimated before the network) +-- Resonator C --+
                     ^                                                                     |
                     +-------------------- Energy loop (optional, bounded) ----------------+
```
- Exciter chain at the oversampled rate (Eco 1x / 2x with folder, Normal 2x, High 4x), polyphase IIR halfband decimation (elliptic design, ~70 dB).
- Exciter models: Off, Breath (v0.1 exciter), Wave (PolyBLEP morph, PW, sub, PD, sync), Complex ("orbit" oscillator with bounded chaos), 12 seeded noise models, 8 physical exciters, Sidechain (LP/HP, follower, transients, 250 ms freeze; no recording / sample storage).
- Interaction: 13 modes; balance, depth, B->A, A->B, DC block, normaliser, drive.
- Pre-shaper: v0.1 exciter LP/HP/key-track IDs, band-pass, resonance, drive, bias, slew, transient, envelope amount, order.
- Wavefolder: 7 original fold modes, 1-4 stages, symmetry, bias, shape, mix, compensation, post LP, DC-free, bounded; transfer curve display.
- Network: 3 slots x 9 models, Single / Serial / Parallel / Hybrid, coupling normalisation (routes scaled by the target slot's loss), 6 cross routes with shared delay / filter / drive / polarity / damping, inject point, output tap (constant-power normalised), width, mix, Repipe macro (shared curve helper used by DSP and GUI), energy loop, governor (peak follower limiter on the feedback paths), 2 ms fade on model switches, NaN flush.
- Matrix: 16 slots, 29 sources, 52 destinations.
- Parameters: all 120 v0.1 IDs preserved with identical meaning; state version 2; v0.1 sessions / presets load unchanged (tested).

### Parameter inventory
- Preserved as-is (120): all `exc_*`, `env_*`, `art_*`, `res_*`, `lfo*`, `menv_*`, `mod1..8_*`, `chorus_*`, `delay_*`, `rev_*`, `voice_*`, `glide_*`, `unison_*`, `bend_range`, `mpe_enable`, `out_*`, `limiter_on`.
- Re-homed in the GUI only (no ID change): `exc_lp`, `exc_hp`, `exc_keytrack` -> PRE-SHAPER filter; `exc_reed`, `exc_pressure` -> Resonator A + Breath model; `exc_pluck`, `exc_pluck_len`, `exc_attack_click`, `exc_release_noise` -> Breath model.
- New (233): `exa_*` / `exb_*` (51 each), `exb_sync`, `mix_*` (9), `pre_*` (8), `wf_*` (11), `dyn_amount`, `res_on/input/output/pan/width/pickup/inharm/size`, `rb_*` / `rc_*` (21 each), `net_*` (23), `loop_*` (8), `mod9..16_*` (24), `quality`. See docs/PARAMETERS.md.

## Completed
- (v0.1) Everything listed in the baseline above
- v0.2 step A: table-driven parameter layout (`scripts/gen_params.py`), state version 2
- v0.2 step B: DSP (exciter slots, interaction, pre-shaper, folder + oversampling, modal resonators, network, loop, matrix)
- v0.2 step C: stability work (modal normalisation, coupling normalisation, governor, click-free model switches), 20 presets, GUI overhaul (5 pages, scopes, transfer curve, interactive diagram), editor / migration / network / folder / exciter / profile / preset-level tests, docs

## Remaining / follow-ups (not required for v0.2)
- macOS / Linux / AU verification; MSVC build verification
- Per-slot sidechain freeze is momentary by design; a longer hold would need a user-facing buffer length
- CPU: the Wave / Complex exciters dominate at 4x; a SIMD pass over the exciter chain would help 16-voice High-quality patches
- Diagram: route dragging could also edit the serial sends

## Known issues / limitations
- Build directory must not contain `^` (cmd.exe escape) because of JUCE's post-build steps under Ninja; the scripts build under `D:\dev\build\aeriform` when the source path contains `^`
- MinGW build has no DirectWrite: JUCE falls back to GDI text rendering
- Waveguide tuning is exact for the fundamental; autocorrelation-measured pitch is up to ~5 cents flat at C2 (partial stretch)
- Waveguide notes above ~5 kHz with maximum dispersion cannot be tuned exactly (clamped safely)
- Sidechain audio only sounds while notes are held (note-gated by design)
- Default patch costs ~2x v0.1 in Normal quality because of the oversampled exciter chain (Eco restores the old cost with the folder off)
- AU target declared for macOS but not built here

## Build and test status
- 2026-09-05: build OK; 64 unit / 8 smoke tests pass; host check PASSED; pluginval strictness 10 SUCCESS
