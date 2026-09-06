# Sympathetic Bank and Coupled Room: audibility implementation plan

Status: first implementation completed; final validation and listening sign-off tracked in AUDIBILITY_RESULTS.md. Original plan written 2026-09-06 at the user's request.
Reference checkout: `D:\dev\build\gpt-aeriform-test`, branch `codex/experimental-aeriform`, HEAD `babb0664fb572a8c1745da4f6e2d8a25153af8db` when inspected.

## 1. What the user reported and what success means

The user could hear only a small Sympathetic Bank change with Held notes tuning and essentially no useful response from its other controls. Coupled Room was inaudible to them. Requiring maximum sends, one mode and carefully matched notes is not an adequate normal workflow.

The next implementation priority is to make these existing modules musically useful before adding more unrelated features. One sounding voice and one main resonator must be sufficient. Useful defaults should produce an obvious but controllable contribution; upper settings should let the module become a prominent part of the sound. The existing tuning, damping, brightness, decay, room geometry and coupling controls need demonstrable effects beyond a small level change.

Historical planning context: the initial turn only wrote this plan and linked it from TASKS.md. Implementation has since begun; see AUDIBILITY_RESULTS.md for measured results and remaining work. Preserve the previously requested development pause until 2026-09-06 07:51 UTC unless a later user instruction changes it. At the next authorized development pass, start with this plan and current repository state.

## 2. Repository and collaboration boundaries

- Edit and build only in the experimental checkout. Do not edit, build, clean or push to the original `C:\Users\The Nerd^2\Documents\Synth claude code` checkout. The `origin` remote points to that local original; GitHub is the separate `github` remote.
- Inspect HEAD, branch, status and relevant diffs again before implementation. The user or another tool may have changed the project since this note was written.
- Existing unrelated WIP was present: `.gitignore`, `Tests/AudioRegression/`, `VALIDATION_PLAN.md`, and `docs/FEATURE_INVENTORY.md`. Preserve it. Do not overwrite, stage or publish another validation task's work as part of this change.
- Read the validation plan to coordinate fixtures, baselines and shared build use. Its separate validation workflow currently says not to push its work. Do not silently change that workflow or start its stages from this implementation note.
- Keep `User-Test-Issues.txt` local and unchanged. Earlier automatic approval review rejected publishing that private feedback file. This plan contains the engineering response rather than reproducing that file.
- Preserve stable parameter IDs, existing enum order, the EXP_Aeriform identity and the original factory presets. Keep new modules disabled in original presets.
- Current test binaries in `artifacts/windows-x64` are an immutable reference checkpoint until deliberately replaced after the revised source is validated. Do not describe them as containing this fix.

## 3. Diagnosis confirmed by source inspection

### Sympathetic Bank

`Source/DSP/SympatheticBank.cpp::next` currently takes the mono voice sum, applies `tanh(input * send) / voiceCount`, injects each mode through `(1 - radius)`, and scales the output by `4 * returnLevel / (sumWeights * brightnessCompensation)`. The factor of four is already present: another blanket gain multiplier is not a complete fix.

Longer decay puts radius nearer one, reducing the initial excitation and making buildup slow. IMPORTANT: `(1 - radius)` is not a fixed attenuation of the eventual on-resonance sine response; coherent accumulation offsets it. The defect is weak short excitation, slow buildup and the interaction with very narrow modes, voice division and mode averaging.

The current brightness compensation can attenuate low modes to allow for brighter upper modes that are not even active. Adding more modes can dilute a small number of modes that actually ring. Dividing the already saturated voice sum by voice count can make denser playing particularly weak. Abrupt changes in active voice count also need checking for gain steps during releases.

`SynthEngine::renderVoiceSegment` supplies the bank with `(L + R) / 2` after voice/resonator processing. That signal can be narrow-band, and strongly antiphase stereo content can cancel. A linear, narrow resonator cannot generate an arbitrary scale pitch absent from a steady input sine. Gain alone cannot guarantee that every unrelated tuning becomes prominent; useful source-derived onset excitation may also be required.

### Coupled Room

`CoupledRoom::next` divides the send by active voices, then scales excitation by `1 - g`, where `g = 0.2 + 0.78 * feedback`. Thus more feedback also reduces fresh input. Output level is applied only to the audible room output.

The separate return path is much smaller: `makeReturn` applies `0.02 * networkReturn / voiceCount`, and `Voice.cpp` applies another `0.8 * minimumResonatorLoss`. With the current minimum loss floor of 0.002, the combined coefficient is only 0.000032 per voice before considering the room signal itself. That example describes a coefficient, not a measured overall loop gain. Changing Return delay/filter affects this very weak feedback path, not the audible room output.

The existing host test accepted squared audio differences as small as `1e-10`. Such a test proves connectivity, not useful audibility. The isolated room tests do not establish stability of a substantially stronger complete voice-room-voice loop.

### Shared dependency to protect

`Source/DSP/Effects/ShimmerReverb.cpp` owns a `CoupledRoom` and sends its output through pitch shifting back to its input. Changing the shared room's injection or output gain changes shimmer's loop too. Keep the new room voicing outside that shared core where practical, or use an explicit caller-selected configuration that leaves shimmer's existing behaviour intact. Never globally boost the shared core and assume shimmer is unaffected.

## 4. First pass: establish meaningful measurements

Before changing gains, freeze deterministic reference patches/stimuli and record the current output at the actual current commit. Reuse the existing AudioRegression work if it is ready; do not create a competing harness or modify another task's frozen reference fixtures. Use generated stimuli rather than external/copyrighted recordings.

Collect separate signals for: ordinary dry voice sum, bank return, room audible return, actual room feedback sent to voices, and final output before/after the master limiter. Meter actual audio at these boundaries, not just internal state estimates. Store peak, RMS, tail energy, spectrum, and controller/guard activity with patch, seed, sample rate, block size and commit identity.

Use three principal musical probes: a short pluck/impact, a sustained harmonic exciter, and a noisy/broadband exciter. Include matched and deliberately unmatched notes for scale/held/custom bank tuning, ordinary chords and coherent repeated pitches. Test one main resonator first, then multiple resonators and Repipe. Turn unrelated effects and morphing off for diagnosis, then restore them for integration checks.

For every audible comparison retain both the unmodified gain comparison and an RMS-matched comparison. The former exposes insufficient wet level; the latter determines whether timbre and decay genuinely change. Do not peak-normalize every file and conceal the original gain problem.

## 5. Audition controls without changing the excitation graph

Add a clearly labelled momentary or mutually exclusive `Audition return` control to each module page. Only one audition target should be active at once. Audition should be temporary monitoring state, not a preset parameter, randomizer target or morph endpoint value. Reset it on preset/session load and when the editor is closed so a reopened project is not unexpectedly soloed.

Implement an atomic processor-owned monitor selection and preallocated audio tap buffers. Capture the actual stereo return during normal DSP processing. Continue computing the complete ordinary voice/send/feedback graph; select the monitor signal afterward with a 15–30 ms fade. Muting voices before computing their send would starve the module and change the feedback behaviour, defeating the purpose.

Choose and label the exact audition boundary: the raw module return before unrelated global effects is the preferred diagnostic signal. Continue normal internal processing in the background and pass the monitored signal through the final output/DC protection. Audition must not amplify the signal independently or disguise its actual level. Verify routing in Deep morph, oversized-block subdivision, mono output and release tails. No buffer allocation or UI calls from the audio thread when toggled.

The energy displays should distinguish input/send activity from returned audio level. If Hold/Freeze is enabled on an empty module, indicate that it has no stored energy; do not imply that Hold creates sound.

## 6. Sympathetic Bank redesign

### 6.1 Decouple useful excitation from decay

Retain the complex modal rotation and radius-derived decay. Prototype the update as `zNext = radius * rotation(z) + calibratedInput * availableHeadroom`, with excitation calibration independent of the requested decay time. Do NOT simply remove `(1 - radius)` and leave an unbounded resonant amplifier. Likewise, replacing it with `sqrt(1 - radius^2)` is a noise-power normalization, not a solution that preserves transient strength at every decay.

Separate sustained excitation from a short, bounded source-derived attack contribution. Calibrate the sustained path at a reference response/energy level and make its gain sample-rate aware. Derive attacks from changes in the actual source envelope or existing exciter transients; make their energy comparable across sample rates and long/short decay settings. No always-running noise, oscillator, autonomous drone, or attack on silence. Decay should primarily determine how long captured energy rings.

Use a smooth total modal-energy controller to reduce incoming energy only near a documented capacity, plus a rare independent finite-state/radial bound as a backstop. Radial control scales a mode's complex state without independently clipping real and imaginary parts and disturbing phase. Keep the ordinary playing range away from the guard; record gain-reduction and guard counts. At Hold, ramp new excitation to zero and radius to one; retain the existing stored energy without pumping it.

### 6.2 Provide enough spectral material where needed

First measure the calibrated current post-network feed. If scale/custom tuning still fails on representative harmonic and percussive patches, introduce a modest, documented contribution from the existing pre-resonator exciter/transient signal. Keep it driven by played audio, not arbitrary added noise.

If that requires a new tap, write a real sample stream from the existing Voice render point into a preallocated engine send buffer. Apply appropriate voice envelope/level scaling, sum at the correct base sample rate, and preserve sample-accurate note boundaries. Do not treat the single `getLastFolded()` value as a whole block of excitation. Start with a fixed calibrated blend under the existing Voice send control; add another user-facing control only if comparison shows a necessary musical choice.

Check antiphase stereo. If mono cancellation is responsible for missing excitation, retain both channels at the send boundary and evaluate an energy-preserving two-port mapping (for example mid/side into the mode's real/imaginary excitation), calibrated against mono. Do not rectify or otherwise distort ordinary audio merely to avoid cancellation. Test the chosen mapping with mono, stereo, antiphase and side-only stimuli.

### 6.3 Replace excessive count normalization

Replace unconditional `1 / activeVoiceCount` attenuation with bounded, slowly varying input/energy control that responds to the signal, not just note bookkeeping. Loudness should not collapse when another note is pressed or jump when a released voice finally expires. Use a fast independent peak bound for exceptional coherent input; a slow controller alone cannot catch every instantaneous peak.

Evaluate normalization from the sum of squared participating output weights rather than a simple average across mode count. Include only active/audible modes in brightness compensation. Distinct modes, duplicate custom intervals, nearly coincident detuned modes and modes clamped near Nyquist require separate tests: a square-root normalization can still add coherent modes excessively. Smooth mode-count and normalization changes together, and set final constants from those tests.

Keep tuning rules explicit. Held notes follows the played chord; scales/custom intervals impose their selected pitches when the excitation contains transient/broadband energy. An unmatched steady sine is a useful negative control, not a promise of arbitrary pitch generation. Make brightness visibly/audibly tilt the bank, damping shorten/darken modes, and damper reduce ringing without the input controller cancelling that change.

## 7. Coupled Room redesign

### 7.1 Make the audible path useful first, with feedback return zero

Temporarily set network return to zero and tune the room output in isolation. Calibrate the audible return independently of the coefficient that stabilizes the FDN. Retain a contracting internal network and deliberate input/state limits.

Add or strengthen a feedforward set of early reflections from the source using preallocated taps. Size and shape should move those delays; diffusion should move the sound between recognizable reflections/comb colour and a diffuse tail. Keep the early-reflection/output gains outside the path used to feed voices or shimmer. Avoid a zero-delay copy that merely raises the dry level.

Prefer a separate output/tap voicing layer over raising internal stored energy just to make the room audible. Calibrate early and late return levels so the default-enabled module is noticeable and the upper Output level range can become prominent. Preserve internal decay/damping control rather than compensating away its timbral effect. Revise stale labels/tooltips that describe all room behaviour as a small return.

### 7.2 Strengthen network coupling separately

After the audible room is validated, set audible Output level to zero and measure the full return-only path. Trace room send, room return conditioning, voice injection, each resonator/network mode, voice gain/envelopes, body processing, modular filters and the resummed room input. The existing `minimumResonatorLoss` is a heuristic, not a proof that every complete path is passive.

Derive a conservative but useful coupling budget from the actual transfer/state behaviour. Keep the delayed return normalized across contributors, use documented bounded conditioning and smooth energy-dependent reduction as needed, and recalibrate the user range instead of stacking arbitrary tiny constants. Do not select a larger constant merely because a short smoke test remains finite. No claim of exact mechanical energy conservation is warranted by this musical DSP model.

Preserve the explicit causal return delay and its 32-sample minimum independent of host block size. Retain local energy/finite-state guards independently of the final limiter. Coupling should audibly alter attack, resonance colour, beating or tail duration with room Output level at zero. It must not produce uncontrolled self-oscillation in ordinary use or change voice count/quality automatically.

Output level should remain independent from network-return strength. Test that changing audible output or enabling audition does not alter internal room/network energy. If a shared-core change is unavoidable, give shimmer an explicit compatible configuration and verify its previous sound and stability.

## 8. Initial acceptance targets and test matrix

These are proposed calibration targets, not achieved results or universal psychoacoustic guarantees. Establish reference windows and measurements before tuning. Do not reduce a target to a tiny nonzero value just to make a test pass. If a stimulus is inappropriate, document the physical reason and replace that fixture deliberately.

| Behaviour | Initial target / evidence |
|---|---|
| Useful normal level | On the named matched/harmonic/noise probes, an enabled module at its revised default should typically provide an isolated return around -18 to -6 dB RMS relative to the dry reference in the relevant active window. Preserve a zero contribution at minimum send/return. |
| Prominent upper range | Maximum musically intended settings should allow return level roughly comparable to dry (-6 to 0 dB on those references), without continuous emergency guarding or master-limiter flattening. |
| Short-note response | A pluck/impact produces a clearly observable and audible tail; long decay does not make the initial response disappear. Compare early wet energy separately from measured decay. |
| Control usefulness | Use level-matched spectra, spectral centroid, reflection timing and decay curves plus actual listening. Require meaningful differences on appropriate probes; a small numerical null residual is insufficient. |
| Return-only room | With audible room output zero, normal-to-strong Network return must produce a repeatable audible change in resonator attack/tail or spectrum. Quantify it relative to the ordinary output and retain a matched-level audition pair. |
| Polyphony and count | No unexplained collapse or jump with voices 1/4/8/16 or modes 1/3/6/12. Test distinct notes, coherent duplicates, short releases and voice stealing. |
| Silence and hold | No output from an unexcited module; no source-independent attack generator. Freeze preserves existing energy and rejects new input. Clear empties it. Bypass fades cleanly. |
| Safety | Finite bounded internal state without the master limiter, with no emergency state clipping in representative or sustained maximum-control probes. Explicitly measure any normal controller gain reduction. |

Run at 44.1/48/96 kHz, relevant 32/64/256/512/1024 host blocks, all quality modes, Economy/Physical stereo, normal and Deep morph, and combinations with Repipe, Energy Loop, contact, feedback-position filters, sympathetic/room together, and shimmer. Include parameter extremes and rapid automation, silence recovery, repeated prepare/reset and state/preset restoration. Use long enough sustained/decaying runs to expose slow accumulation; short finite-output tests alone do not qualify.

Retain isolated mathematical/stability tests and add full processor tests. Replace or supplement the old arbitrary tiny-difference assertions and the bank test that explicitly expects `1 / voices` attenuation. Do not discard valid bounds, tuning or timing tests merely because normalization changes. Extend the existing C++ allocation probe around processing, note/count transitions, capture/clear and audition; distinguish its coverage from all malloc calls and from the remaining whole-synth lock audit.

Create labelled WAV A/B examples and metrics JSON with source commit and hashes. Listen to them where supported and record what was actually heard; otherwise mark listening as pending and provide them to the user. Never claim a listening pass from graphs or a nonzero difference test alone.

## 9. Files and implementation sequence

1. **Reference measurements and taps:** coordinate `Tests/AudioRegression/`; extend `Tests/SympatheticTests.cpp`, `Tests/RoomTests.cpp`, `SynthEngine.cpp`, `Voice.cpp/.h`, and `VisualizerModel.h` only as needed for real audio taps. Capture current behaviour before changing it.
2. **Temporary audition:** processor/editor monitoring state, `SympatheticPage.cpp/.h` and `RoomPage.cpp/.h`; verify normal graph unchanged by monitor selection.
3. **Bank excitation/gain:** `SympatheticBank.cpp/.h` and the engine/voice send boundary. Validate transient, sustained, scale/custom and count behaviour before combining it with stronger room feedback.
4. **Room audible output:** `CoupledRoom.cpp/.h` plus a separate voicing/tap layer if needed. Verify shimmer compatibility before the next stage.
5. **Room network return:** `CoupledRoom::makeReturn`, `SynthEngine::renderSegment`, `Voice::render` and loss handling. Validate the complete loop with output muted and all relevant internal modes.
6. **Control/default polish and demonstrations:** change generator definitions in `scripts/network_params.py` if necessary, regenerate metadata, and append demonstration presets. Preserve IDs and existing ranges where possible; document any deliberate change in enabled experimental-preset sound. No silent compatibility claim for retuned enabled modules.
7. **Regression and delivery:** focused tests, full suite, host check and pluginval strictness 10; appropriate CPU/peak-time/memory/allocation measurements. Rebuild both VST3 and standalone. Update `docs/RESONATOR_NETWORK.md`, performance/testing notes and feature inventory with measured results, then create a coherent checkpoint and update binaries only after validation and the applicable publishing instructions permit it.

Use coherent, small checkpoints so the bank correction, room output voicing and stronger feedback can be reviewed/reverted independently. Keep the older validated artifacts available while evaluating the new sound. The separate validation baseline at `fe0cb07` and its fixtures must remain unchanged; original-new-modules-off comparisons must still pass within their documented timing rules.

## 10. Completion checklist

- [ ] Reproduce and measure the user's weak-bank/inaudible-room report on named patches.
- [ ] Provide a useful isolated-return audition without starving excitation or altering feedback.
- [ ] Bank responds strongly to ordinary attacks and has useful scale/custom settings on appropriate source material.
- [ ] Bank decay, brightness, damping, damper, tuning and mode count have clear, documented effects.
- [ ] Room has an audible early/late contribution at sensible defaults.
- [ ] Room size, shape, diffusion and damping reshape that contribution.
- [ ] Return-only coupling changes the physical instrument, with an independently verified bound.
- [ ] Shimmer remains compatible/stable after shared-room work.
- [ ] Original presets with both modules off remain compatible; enabled experimental changes are documented.
- [ ] Quantitative tests, real listening evidence, realtime checks and both binary validators are recorded honestly.
- [ ] The user can hear a useful difference without special one-mode/max-gain troubleshooting settings.

Until these points are satisfied, call the work an audibility redesign in progress rather than marking the two features musically complete.
