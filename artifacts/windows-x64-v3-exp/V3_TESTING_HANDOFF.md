# AERIFORM v3 — testing handoff

Updated 2026-09-06. This is a request for evidence from the user's autonomous testing agents, not permission to modify production DSP or overwrite another agent's work.

## Feature build update: v3 EXP (2026-09-06)

The new feature build supersedes the reference below for contact, spectral UI and saturation. Use the source commit and SHA-256 manifest shipped with artifacts/windows-x64-v3-exp. This is an experimental handoff with compilation and focused smoke checks, not a full release certification.

The user's testing task has completed its earlier campaign. Reuse those results for unchanged code; do not restart its entire eight-hour run. Prioritize the changed paths below and report findings against the exact new build. The implementation task is concentrating on completing features; broad acceptance, performance and fuzz testing belong to the separate testing task.

- Contact now has an oversampled nonlinear pickup stop in addition to the original conservative physical injection. The stop can affect identical, in-phase resonators and the mono mix. Its Amount response is strongest through the middle of the control range; physical coupling retains its original linear Amount response. Its gap uses a squared response for quiet signals. Physical injection retains its original gap and destination-loss scaling. The pickup map is instantaneously contractive before the oversampling filters; this is not a proof that the complete delayed network is passive. Check actual timbre, feedback combinations, transitions and contact audibility in your patches.
- Contact activity measures pickup correction, and the interface explains inactive routes and below-gap signals. Physical stereo controls are disabled in Economy mode.
- Spectral freeze now has its dedicated SPACE page, one-click capture/release, hold, blur, shift, random phase, decay, mix and a captured-spectrum display. Capture/Release host booleans intentionally remain change-triggered in either direction, preserving their documented automation contract. A GUI click changes the command once, without changing it again on mouse release. A release now cancels an analysis-frame capture that has not happened yet. Do not classify both-edge host automation as a failure of the documented contract; flag any unintended GUI retrigger separately.
- Three-band saturation is now implemented and must no longer be marked NOT IMPLEMENTED. There are 17 appended IDs (509 total), all existing IDs retain their positions. Models: Soft, Warm, Clip, Fold. Zero drive is undistorted. LR4 bands have low-band allpass phase compensation; their neutral sum has flat magnitude, not sample-exact phase. Per-band and global mixes use phase-aligned oversampled references. Disabled bypass is sample-exact after its fade. Quality switches briefly fade to raw bypass. Check crossover reconstruction against that contract, especially near the split frequencies, and check model/drive/DC/aliasing behavior and state restore.

## Current scope and reference

Implementation priority: (1) finish contact/collision so it has meaningful musical effects; (2) finish the spectral-freeze interface and behavior; (3) implement three-band saturation. The separate normal-input FX application/plugin is assigned to somebody else and is outside this implementation/testing request. Additional presets are low priority.

The user listened to the latest experimental package and reported much clearer sound from the revised modules and a substantial effect from Physical stereo. Contact/collision remained difficult to hear. Economy's lack of physical-stereo processing is intentional. Do not report contact as accepted based on the other modules' successful listening test.

Reference public build: 6f3bb664f837674113307b8544da7a37d7843803. Its production Source tree is 2d891c762f15c66e3c5b19d5c6972227e39ce309, identical to local build source 55aa009. It passed 139 tests and 40 bit-identical original-preset comparisons, plus the host checker and pluginval strictness 10. These results are a baseline, not certification of future changes.

Record the exact source commit AND binary hash in every new report. There are intentionally separate local validation and public production histories; read docs/audibility/PUBLICATION.md. The existing staged validation report covers an older feature snapshot. Send that report as-is; identify which findings still reproduce on the revised source rather than silently reclassifying them.

## Boundaries and efficient execution

- Follow the existing validation task's branch/worktree, extension-point and no-push rules. Preserve its harness, reports and immutable fixtures. Verify fixture hashes before comparing them. Never regenerate a reference to make a failure disappear.
- Do not edit the original synth checkout, production DSP, parameter IDs, tolerances or another agent's files. Report suspected fixes to the implementation task.
- Reuse existing tests/results where the tested source is unchanged. First test changed features and their integration points; run the complete release matrix after implementation stabilizes.
- Saturation DSP and its dedicated UI are now present. Spectral freeze has its dedicated UI; validate both against the new package, not the earlier source snapshot.
- No FX-target tests or new preset-bank project are requested here. Existing preset compatibility and patch-level audibility examples are still required.

## Priority 1: contact/collision audibility

Use deterministic generated sources and frozen patch/settings definitions: a pluck/short impulse-like note, sustained harmonic excitation, and broadband/noisy excitation. Include a sustained single sine as a deliberately limited spectral probe. Initially disable bank, room, unrelated effects and master limiter. Keep topology unchanged while comparing contact off/on.

Test A-to-B with both resonators running, matched and different tunings, then other source/destination pairs. Also characterize source equals destination, disabled slots, every resonator off, and resonator bypass. An invalid or inactive configuration must be explained by the interface; do not silently enable additional resonators or accept a misleading active indicator. Source-reaction behavior when a destination is inactive needs an explicit documented decision.

For every contact comparison retain raw stereo WAVs, RMS-matched WAVs, dry/output RMS and peaks, output-difference RMS relative to contact-off, spectral/harmonic changes, attack and tail windows, and real contact activity/guard data when available. A nonzero residual alone is not evidence of useful sound.

Provisional calibration gates for the designated active-contact fixtures: normal settings should produce a clearly measurable contribution (at least -24 dB output-difference RMS relative to off); strong settings should reach at least -12 dB on suitable material. Retain the level-matched residual and spectra to distinguish timbre from gain. These are fixture-specific engineering targets, not universal hearing thresholds. Report failures or physically unsuitable stimuli; do not lower the gates to accommodate a weak implementation. Human listening must confirm useful rattles, bridge buzz, chatter or altered attacks.

Sweep gap, stiffness, hardness, damping, friction, asymmetry, amount and polarity separately. Verify a genuinely inactive region below the gap, smooth transitions into contact, and distinct musical effects above it. Threshold/hardness/amount interactions must not leave most of the useful knob range effectively dead. Check that indicators track actual contact rather than enable state alone.

## Priority 2: contact stability and aliasing

- Test resonant/coherent and dissimilar pitches, transient and sustained input, maximum controls and parameter automation. Include 1/4/8/16 voices, repeated notes, voice stealing and releases.
- Measure internal states and guard activity without relying on the master limiter. Check the intended energy budget/bound for the implemented algorithm; do not claim that the complete nonlinear instrument is mechanically passive from a single scattering equation.
- Include at least 30-second sustained probes and 60-second source-free decay/hold probes where applicable. Distinguish intentional held resonance from growing, unbounded state or a return that supplies its own excitation.
- Compare collision quality settings at equal output level and with a demonstrably excited nonlinear contact. Measure folded harmonics/alias energy using an appropriate high-rate reference. The prior testing report's zero measured alias suppression needs investigation in the actual production route, not dismissal based on an isolated mathematical test.
- Test changes of source/destination, quality, polarity, enable and amount during held notes. Record discontinuities, crashes, nonfinite samples and unexpected resets.

## Priority 3: spectral-freeze completion

Verify both the UI and host parameters: enable, Capture, Release, Hold/Freeze, blur, shift, random phase, decay and mix. Every UI control must reach the intended parameter and display its actual state.

Test capture after silence, a short note and a sustained note; hold before any capture; repeated captures; capture/release/capture; parameter automation and restored sessions. A button's press/release edges must not accidentally create extra captures. Recheck the earlier report that toggling the capture parameter re-arms it on both edges. Count actual captures where instrumentation exists.

Check that captured spectra respond to shift/blur/phase/decay, that dry and wet timing are understood, and that release returns smoothly to live audio. Measure live and frozen latency separately. Check mono/stereo, empty-buffer behavior, state/preset load, bypass/reset, sample-rate changes and Deep morph. Captured audio is temporary DSP memory under the current design; a restored project must not misleadingly claim to contain a capture that was not saved.

Intercept processing allocations during capture, recapture, release and held processing after prepare. Check finite output and long-term behavior at extreme settings. Do not infer allocation-free behavior solely from preallocated FFT arrays.

## Priority 4: three-band saturation, once implemented

Required controls: two crossover frequencies; low/mid/high drive, selectable model, mix and output; global mix; oversampling quality.

- Measure crossover ordering and range behavior, frequency response, phase/group delay and mono compatibility. With nonlinear processing disabled, verify reconstruction against the declared phase/latency behavior. A phase-coherent crossover may have an all-pass phase response; require a documented reference, not a misleading raw zero-lag null. Full effect bypass/global mix zero must preserve the existing dry path under its stated latency contract.
- Sweep each band's drive, model, mix and output while exciting that band. Confirm that other bands remain appropriately separated, and that models produce distinct transfer/spectral behavior.
- Check low/mid/high isolated signals, impulses, noise and full-band mixtures. Measure DC, peaks, internal energy and intermodulation. Test output compensation without concealing continual clipping behind a master limiter.
- Test crossover automation and limits without band inversion, gaps, clicks or stale filter state. Check oversampling changes and wet/dry alignment; quantify alias suppression at comparable drive and level.
- Check defaults/bypass in legacy states, stable appended IDs, state round trips, host automation, UI synchronization, randomizer safety and A/B Parameter/Deep morph behavior.

## Integration and release gates

Prioritize contact with each of Repipe, Energy Loop, feedback-position filters, Physical stereo, bank and room. Then test the completed effects with resonant delay, shimmer, spectral freeze, saturation and both morph modes. Do not assume standalone unit tests establish complete-loop stability.

Use 44.1/48/96 kHz, 32/64/256/512/1024-sample blocks, mono/stereo, all quality settings, and representative 1/8/16-voice loads. Include host blocks larger than prepare's requested maximum. Maintain existing sample-accurate MIDI timing and the room's causal return delay.

Compare CPU and worst block time on the same machine and configuration with warm-up, no competing builds, and separately reported preparation time. Report inactive-feature overhead, steady-state cost, transition spikes and Deep morph cost. Do not automatically reduce voices or quality to obtain a pass. Distinguish instrumented C++ allocation coverage from all allocator calls; source-audit audio-thread locks/host notifications rather than inferring their absence from a quiet timing trace.

Recheck original preset/state neutrality on the final source, repeated preset loads, note/pedal/voice lifecycle, editor reopen/scale/navigation, automation restore and Ableton save/reload. Preserve and investigate previously reported carryover/reset findings with feature-off controls; attribute them to a new module only when the evidence supports that attribution. Existing presets matter for compatibility; designing new presets remains low priority.

Run final instrument VST3 host and pluginval checks after code changes settle. Standalone synth smoke testing is relevant; the separately owned FX product is excluded. Binary name/version/source hash must identify exactly what was tested.

## Report format

Provide a concise summary plus per-case evidence:

1. Exact source commit, binary SHA-256, toolchain, OS/CPU, test-command/config and fixture hashes.
2. PASS / FAIL / PARTIAL / NOT IMPLEMENTED / NOT TESTABLE, with actual metrics and the threshold/reference used. Keep older-source findings labelled as such.
3. Minimal reproduction: patch/parameter values, MIDI/input, sample rate, block size, quality, voice count and timing of changes.
4. Raw and matched WAV paths, relevant spectra/metrics JSON, internal controller/guard results, and screenshots for UI failures.
5. Listening observations only when someone actually listened, including which files/settings were compared. Otherwise mark listening pending.
6. Severity, likely affected component, and the smallest suggested follow-up. Do not change implementation or tolerances to force a pass.

Send the existing overnight report first. New runs should fill evidence gaps and validate the revised feature builds, rather than repeat every completed stage by default.
