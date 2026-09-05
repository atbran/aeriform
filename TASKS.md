# AERIFORM - Task Tracker

Living document. Updated as work progresses.

## v2.1 overhaul: dual exciters, wavefolder, resonator network - DONE (2026-09-05)

### Baseline (recorded 2026-09-04 before any v2.1 change)
- Build: OK (GCC 14.2 MinGW-w64, Release, `D:\dev\build\aeriform\mingw-release`)
- `AeriformTests --all`: 38 tests, 3690 checks, 0 failures
- `AeriformHostCheck ... AERIFORM.vst3`: PASSED; pluginval strictness 10: SUCCESS
- CPU: 8 voices + effects @ 48 kHz / 256 = 3.7 % real time; 16 voices (unison 2) = 6.3 %
- 120 parameters, state version 1, 20 factory presets
- Git: repository initialised at this baseline (commit "Checkpoint: AERIFORM v0.1.0 baseline")

### Final status (v2.1.0)
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
- v2.1 step A: table-driven parameter layout (`scripts/gen_params.py`), state version 2
- v2.1 step B: DSP (exciter slots, interaction, pre-shaper, folder + oversampling, modal resonators, network, loop, matrix)
- v2.1 step C: stability work (modal normalisation, coupling normalisation, governor, click-free model switches), 20 presets, GUI overhaul (5 pages, scopes, transfer curve, interactive diagram), editor / migration / network / folder / exciter / profile / preset-level tests, docs

## Remaining / follow-ups (not required for v2.1)
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

## EXP_Aeriform implementation plan (Codex, 2026-09-05)

All edits and builds occur only in D:\dev\build\gpt-aeriform-test. Confirmed original: C:\Users\The Nerd^2\Documents\Synth claude code. Preserve original factory presets. Experimental synth name EXP_Aeriform with distinct plugin identity; separate shared-DSP FX target. A/B morph is approved; optional XY is deferred. Three modular movable filter blocks provide surgical, character and comb types. Open-source hobby project; no extra proposed ideas are approved.

- [ ] Phase 0: independent clone, baseline build/unit/smoke/fuzz/host/pluginval, parameter/preset/limiter audit, CODEX_BASELINE.md and baseline commit.
- [ ] Phase 1: stable appended parameters, migration, immutable metadata, GUI undo, A/B effective layer and deep morph, seeded randomizer/locks, persistent favorites. Focused tests and commit.
- [ ] Phase 2: movable filters, collision, sympathetic bank, true stereo, bounded room return; routing/tuning/aliasing/stability tests and commit.
- [ ] Phase 3: resonant delay, shimmer, spectral freeze, multiband saturation, normal-input FX controller and target. Effects/FX tests and commit.
- [ ] Phase 4: functional PLAY/FILTERS/contextual pages, undo shortcuts, favorites/search, demonstrations and GUI rendering. Tested commit.
- [ ] Phase 5: class-aware preset metering, full configuration tests, allocation/performance/latency measurements, validators for both targets, docs/screenshots/release artifacts and final tested commit.

Each successfully tested phase receives a focused commit. Document partial or experimental behavior honestly. No silent quality or voice reduction. Ordinary controls edit the selected snapshot endpoint while morphing. Preserve original sounds by defaulting new DSP off; add corrected preset variants where warranted.

## EXP_Aeriform state/performance phase - verified

- Separate EXP_Aeriform VST3 identity (Exaf / com.aeriformaudio.exp-aeriform) and standalone built.
- 358 stable parameters; state version 3, old state loading tests pass.
- A/B snapshot capture/load/serialization, explicit endpoint edits, Parameter and Deep effective layers, seeded scoped randomization/mutation and locks, GUI undo/redo, persistent stable-ID favorites and combined search/filter are connected.
- New PLAY page and global undo/redo; docs/experimental/play.png rendered and visually inspected.
- Complete regression run: 84 tests, 53468 checks, zero failures. Host check PASSED. Pluginval strictness 10 SUCCESS for EXP_Aeriform.
- Logs: build/state-verified-tests.log, state-host.log, state-pluginval.log.
- Known limits: interior Deep blends cannot flatten to one structure (commit disabled); activating Deep on held notes primes notes but cannot reconstruct past physical energy. Existing JUCE MPE locks/allocation still require the real-time audit. Additional modules and final performance matrix remain unimplemented.

## Movable filters checkpoint

- 394 stable parameters; three movable filters, eleven models and twenty insertion positions, FILTERS page.
- Complete regression: 91 tests, 320079 checks, zero failures (`build/filter-all-tests.log`).
- Disabled routing equality, all-placement audio differences, transition/reset tests, 18 audible reference tuning cases and 126 analytic-versus-measured phase cases passed. Exact tuning limitations are documented in docs/FILTER_ROUTING.md.
- Original presets preserved; new DSP defaults off. Additional physical modules and effects remain in progress.
