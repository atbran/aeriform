## v3 EXP features implemented (2026-09-06)

Contact pickup-stop audibility, contact route feedback, Economy control disabling, dedicated spectral freeze UI, and three-band saturation DSP/parameters/UI are implemented. Version 3.0.0 / 509 parameters. Release EXE/VST3 build, 12 focused tests, and instrument VST3 host check passed. Packaging the feature checkpoint for the separate testing agent. Latest user instruction: prioritize feature completion; run only compilation and cursory smoke checks here. The separate testing agent owns comprehensive acceptance and will report findings. No v3 release certification is claimed. See docs/V3_TESTING_HANDOFF.md for changed paths and precise contracts.

## Current user priorities (supersedes older scope lists, 2026-09-06)

- Finish contact/collision first, with demonstrably meaningful audio/timbre changes and clear routing/activity feedback.
- Then finish the spectral-freeze UI/behavior and implement low/mid/high multiband saturation with crossovers, per-band drive/model/mix/output, global mix and oversampling quality.
- The separate FX application/plugin is owned by somebody else. Do not implement it here or merge its branch as feature work.
- The user's cheaper autonomous agents own broad testing. docs/V3_TESTING_HANDOFF.md specifies requested evidence; the user will supply the overnight report. Reuse that work, run focused implementation checks, and fix confirmed issues without duplicating the entire testing campaign.
- Additional presets are low priority. Preserve existing preset compatibility.
- User listening feedback: the revised modules are much clearer, and Physical stereo has a substantial effect. Contact/collision remains too subtle. Economy intentionally uses the original single network.

# AERIFORM - Task Tracker

## Next implementation priority: audible bank and room

Read [the detailed audibility implementation plan](docs/AUDIBILITY_IMPLEMENTATION_PLAN.md) before the next feature pass. The user reports that Sympathetic Bank barely changes timbre and Coupled Room is inaudible. Prioritize excitation/gain correction, return audition, stronger independently bounded room coupling, and measured/listened acceptance before adding more unrelated features. The plan covers shared shimmer dependencies, compatibility, realtime constraints, tests and implementation order. The first audibility implementation is now in the experimental source: decay-independent bank excitation, source-funded room return, early reflections, isolated-return audition, and actual input/output meters. Read docs/AUDIBILITY_RESULTS.md for validation and remaining listening work. The development pause has expired. Preserve the separate validation task and its no-push work.


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

## Collision checkpoint

- 406 stable parameters. Bounded oversampled contact route connected to both the source reaction and destination injection; CONTACT page with force curve and atomic activity meter.
- Five focused tests, 486787 checks, zero failures (build/contact-tests.log). Full regression at previous filter checkpoint: 91 tests passing; filter pluginval strictness 10 SUCCESS.
- Further shared physical models, true stereo, effects/FX and final performance/allocation validation remain in progress.

## Physical stereo checkpoint

- 415 stable parameters. Independent left/right resonators, coupling, excitation/pickup/damping differences, rotation, width and mono bass connected; contextual PHYSICAL page.
- Five stereo tests: 220685 checks, zero failures. Seven filter regressions: 266281 checks, zero failures. Original source status remains clean.
- Second network stops in Economy. Comb buffers now reserve oversampled storage only where needed, and unused comb buffers are not cleared on note restart.

## Sympathetic bank and six-tab checkpoint

- 443 stable parameters. Twelve shared modes with nine tuning modes, damper/decay/brightness/detune, normalized voice send, hold/clear, MIDI chord capture and persistent chord memory. Controls and atomic activity display are connected.
- Six main tabs now match User-Test-Issues.txt; contact/stereo/sympathetic are under NETWORK, filters under SPACE, morph tools under ADVANCED. Saved section navigation and early experimental tab migration tested.
- Full regression: 107 tests, 2245948 checks, zero failures (build/sympathetic-all-tests.log).
- User-test reports still to resolve: verify sustained resonator pitch edits with Repipe; improve measurable contact audibility; add explicit/automatic direct resonator bypass; right-click modulation assignment and depth dragging.
- Original requested room coupling, new effects, FX target, final realtime/performance audit and release verification remain unfinished.

## User testing checkpoint

- Explicit and automatic resonator bypass added (444 parameters). Contact response strengthened and measured against bypass; direct-network bounds and alias suppression still pass.
- Right-click modulation assignment/removal and teal-ring/Alt depth dragging implemented for existing matrix destinations; preserves occupied slots and groups each gesture for undo.
- Repipe pitch-reset report not reproduced at parameter or DSP-target level; independent slot/exciter pitch can affect the perceived fundamental. Test and limits documented.
- Full regression: 111 tests, 2266122 checks, zero failures (build/user-feedback-all-tests.log).
- Remaining original work: shared room feedback, resonant delay/shimmer/spectral freeze/multiband saturation, FX target, demonstrator presets, realtime allocation/lock fixes, final performance matrix/validators/docs.


## Coupled room checkpoint

- 459 stable parameters; shared eight-line room with bounded, delayed resonator return, live NETWORK controls, freeze/clear and an appended demonstration preset. Original factory definitions retained.
- Full regression: 116 tests, 4119984 checks, zero failures (`build/room-all-tests.log`). Room screenshot rendered and inspected; state and saved section restoration pass.
- Room-to-network return is deliberately conservative and currently subtle; coefficient and measurement details are in docs/RESONATOR_NETWORK.md.
- Additional effects, FX target, final realtime/performance audit and validators remain unfinished.


## Resonant delay checkpoint

- 473 stable parameters; normalized six-mode stereo feedback delay with sync, four colours, smooth tuning/dispersion, last-note tracking, saturation, offsets and mix. SPACE controls and Modal Echo Pluck preset connected.
- Six focused tests: 1616330 checks, zero failures (`build/resdelay-tests.log`). Screenshot rendered and inspected. A float conversion causing fractional error at exact integer delay times was corrected.
- User compiled an independent test build and cancelled the requested package; continue feature implementation. Shimmer source is in progress and not included in this checkpoint.


## Shimmer checkpoint

- 483 stable parameters; independent pitch-shifted reverb feedback and all SPACE controls, interval shortcuts and fractional semitone display connected.
- Four focused tests: 1881052 checks, zero failures (`build/shimmer-tests.log`). Reference pitch, interval transitions, maximum-feedback bounds, actual shifted-tail output, host state and GUI tested; screenshot inspected.
- Spectral freeze, multiband saturation, FX target and the final complete realtime/performance/host validation remain unfinished.


## GitHub test-build checkpoint

- User requests the full experimental source, standalone EXE and VST3 on GitHub's codex/experimental-aeriform branch. Keep the original checkout and main branch unchanged.
- 492 stable parameters. Spectral-freeze DSP and host parameters are connected; four focused tests pass (1339488 checks), including zero intercepted C++ allocations/frees on repeated captures. Dedicated spectral GUI remains pending after an earlier usage-limit rejection; no placeholder page is shipped.
- Full regression before spectral integration: 126 tests, 7618053 checks, zero failures (`build/shimmer-all-tests.log`). Release-checkpoint validation is recorded with the binary artifacts.
- Remaining feature work: dedicated spectral GUI, multiband saturation, FX target, additional demonstration presets, final realtime allocation/lock fixes and all-feature performance validation.

## Audibility revision — validated source checkpoint (2026-09-06)

- Bank onset/sustained excitation, stereo feed, total-energy bound, useful damping and count normalization implemented. Room early/late output and source-funded physical return implemented. Temporary return audition and real input/output meters are available on both pages.
- 139 tests / 17,142,923 checks pass. Forty original presets have bit-identical audio samples against the verified frozen baseline. VST3 host checker and pluginval strictness 10 pass.
- See docs/AUDIBILITY_RESULTS.md for measurements, CPU sample, comparison setup and limits. Human listening approval remains pending. New test packaging goes under artifacts/windows-x64-audibility; older artifacts/windows-x64 remains the previous published reference.
- Preserve Tests/status-report-testing.txt and the independent validation branches/work. They were updated by the separate validation task and are outside this production checkpoint. Do not include its no-push history in a publication.
- Remaining original feature work: dedicated spectral GUI, multiband saturation, normal-input FX target (inspect github/effect-version first), final combined-feature realtime/performance audit and demonstration presets.

## Publication history boundary — audibility checkpoint

The main experimental checkout retains the separate local validation commit d40798c. Production fixes and the validated package were copied onto the previous public checkpoint in build/audibility-publication, branch codex/audibility-publication, for a clean fast-forward of github/codex/experimental-aeriform. The validation ancestor, harness/fixtures, status report and private feedback were explicitly verified absent from that publication tree.

Local production source commit 55aa009 maps to public production commit c06d1bb; local package/log commits 393f46a and bcdce70 map to f5e16fa and 55a245a. The complete Source tree is identical: 2d891c762f15c66e3c5b19d5c6972227e39ce309. Artifact bytes are also identical. Keep this intentional history separation when resuming: inspect file differences before merging remote commits, and do not publish the local validation ancestor.
