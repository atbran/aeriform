# Aeriform: modular effects rack and breath overhaul

## Assignment and agreed scope

This handoff covers requests 1, 5, and 6-10. It is intended for an advanced DSP implementation agent, followed by a different AI verification agent. The user will delegate the work.

Build four serial, reorderable effect slots using existing algorithms other than the permanent Chorus/Delay/Reverb set. Repeated effect types are allowed. Preserve that permanent set as available on every patch; do not force nonzero wet amounts. Replace the default breath character with audible human exhalation, expose meaningful shaping controls, and provide alternate breath characters.

The user explicitly permits existing breath patches to change and does not require a legacy breath-sound mode. Preserve the ability to load patches and unrelated parameter meanings. Macros already exist. New effect algorithms, parallel rack routing, Coupled Room engine expansion, and the other unselected feature suggestions are outside scope.

FEATURE-REQUESTS-CONTROLS-AND-WORKFLOW.md owns continuous modulation coverage, undo/A/B, expression, and control ergonomics. New rack and breath parameters must integrate with those systems.

## Initial investigation and shared interfaces

1. Read current repository/build instructions and inspect the actual implementation branch. Start with DSP/SynthEngine.cpp, DSP/Voice.*, DSP/Exciter.*, DSP/Exciters/ExciterSlot.*, Params/ParamTable.inc, State/PatchStateManager.*, EffectsWorkspace, and current effects implementations. Documentation may describe older UI checkpoints; code and measurements establish current behavior.
2. Inventory existing effect algorithms, their processing location, parameter/state ownership, prepare/reset requirements, latency, feedback bounds, and real-time costs. Inventory breath filtering from source through pre-shaper and resonator so a filter change is not accidentally bypassed or applied twice.
3. Coordinate stable IDs, per-slot parameter ownership, modulation registration, complete patch serialization, and undo/A/B transactions with the companion implementer before changing shared interfaces.
4. Render deterministic baseline breath examples through both the isolated source and full synth. Measure output level, spectrum, attack, and sustain. These are comparisons, not a requirement to preserve the old sound.

Completion: record the architecture decisions, eligible rack inventory, baseline artifacts, and shared interface contract.

## 1. Four-slot modular effects rack

### Theory and required behavior

A serial rack processes x through four ordered slot instances. Each instance owns its parameters and runtime state. Two copies of an effect must behave independently. Changing order changes signal processing order, not merely labels.

The permanent Chorus/Delay/Reverb algorithms are excluded from the slot selector. Distinct existing algorithms such as Resonant Delay, Shimmer, Spectral Freeze, and Multiband Saturation are initial candidates; their family names do not make them the excluded permanent effects. Verify availability in the current branch. Acoustic processors that depend on per-voice or resonator internals must retain their current functionality; do not force them into a post-sum insert or silently change their semantics. Document their eligibility explicitly.

### Concrete work

1. Use four persistent slot identities and store processing order separately. Automating an instance must continue to target that instance after reorder. Provide Empty, enable/bypass, type selection, and the selected algorithm's complete controls. Reuse existing DSP code and shared control components.
2. Give each instance independent state and parameter storage. Define type-switch parameter retention and reset behavior. Use stable, documented host parameter identities and ranges; avoid changing a parameter's host-visible meaning unpredictably when switching type.
3. Choose and document one fixed insertion position relative to the permanent trio, filters, and master stage, based on the current graph. Keep the final output protection in its intended position. No selectable placement or parallel routing is required. Explain migration if existing optional effects currently occupy multiple positions.
4. Migrate existing eligible optional-effect settings into corresponding slots on load where possible, preserving all controls. Keep incompatible acoustic processing in its existing place. If legacy active effects exceed four slots, preserve their configuration and provide an explicit deterministic migration strategy; silently discarding effects is unacceptable.
5. Implement reorder, type changes, and bypass with bounded transitions that avoid abrupt buffer-state discontinuities. Retain instance identity when moving a slot. Define whether removed effects' tails fade or terminate; use a short controlled fade rather than a hard discontinuity.
6. Allocate and prepare runtime resources outside the audio callback. Safely publish graph changes without audio-thread locks, allocation, or destruction. Account for worst-case duplicate expensive effects and temporary crossfade processing. Handle sample-rate/block-size changes and effect reset.
7. Expose the rack on Effects using the established visual style. Repeated instances must be easy to distinguish. Preserve the permanent FX controls on Main and Effects through existing shared bindings.
8. Serialize slot types, order, enable states, and complete parameter values. Integrate structural edits with undo/A/B and continuous controls with modulation. Preserve existing automation identities when migrating old parameters, or implement explicit tested translation.

### Acceptance and independent checks

- Four slots, Empty, bypass, reorder, duplicate instances, and all eligible effect controls work. Permanent trio types do not appear in the selector and remain separately available.
- Use deterministic probes and a known manual chain to verify actual processing order. Use a noncommuting effect pair to demonstrate a measurable order difference.
- Change one of two identical effect instances; only that instance's configuration and state should change. Confirm automation still targets the same instance after reorder.
- Test empty/bypassed transparency, impulse response, silence, tails, rapid type/reorder/bypass changes, maximum feedback, and repeated heavy effects at 44.1/48/96 kHz with small, large, and irregular blocks.
- Check finite output, processing time, and callback allocation/lock behavior. State the instrumentation's coverage and report worst-case measurements; do not claim universal real-time safety from a narrow allocator probe.
- Round-trip sessions/presets, migrate representative existing optional-effect patches, and test undo/A/B after companion integration. Confirm permanent FX amounts are retained.

## 2. Breath engine and default overhaul

### Theory and target sound

The target is an audible soft human exhalation with shape and movement, suitable for exciting resonators. Broad noise alone can sound like static; simply reducing its level can make the instrument harder to play. Combine a shaped noise spectrum, subtle time variation, and an exhalation envelope so breath has a recognizable onset and sustained body.

The current source blends white/pink noise with turbulence and transient noise. Reuse useful components, but evaluate the complete path: the slot can delegate filtering to the pre-shaper, and resonators strongly change the perceived source. Proposed filter shapes and control ranges are hypotheses to tune through rendering and listening, not fixed physical-model claims.

### Concrete work

1. Create a smooth Air component with broad spectral shaping that evokes an open mouth. Use broad, low-resonance peaks or shelves rather than narrow ringing formants. A Mouth control should move naturally between rounded and more open/bright exhalation without requiring speech synthesis.
2. Separate Air amount from Turbulence/roughness. Turbulence should progressively add irregular texture while the low/default setting remains smooth. Use bounded, correlated variation in amplitude and/or spectral shape; avoid obvious periodic tremolo or uncontrolled spikes.
3. Add an exhalation contour: a soft initial h-like air swell that settles during sustain. Expose detailed timing/amount on the dedicated Exciters page. Keep this separate from the main amplitude envelope so changing breath character does not unexpectedly shorten the entire note.
4. Make pressure affect brightness and texture as well as level. Coordinate with existing Pressure > Bright, velocity, and breath controls to avoid double application. Higher pressure should yield a stronger, more open/rough exhalation, with useful low-pressure audibility.
5. Tune an audible default through normal resonator routing. Compare level-matched versions to distinguish tonal improvement from loudness. Reduce harsh attack/release noise if it undermines the target; retain intentional control over these components.
6. Provide Soft Exhale (default), Focused Jet, Whisper, and Rough Air as coherent starting characters using the same engine. Prefer explicit character presets that set visible parameter values over hidden multipliers. Character selection should be one undo transaction; edits afterward remain possible. Clearly indicate a modified character.
7. Expose a compact set of meaningful controls, including Air, Mouth, Turbulence, and contour details. Reconcile existing Noise/Noise Color/Pressure controls with the new design without silently deleting their capabilities. Describe any renaming or parameter migration. Add continuous controls to modulation and integrate macros through the existing engine.
8. Review factory patches using the default breath source. Retune those adversely affected so the new default is useful across typical pitches and velocities. Existing breath sound compatibility is explicitly not required; successful state loading and unrelated sound behavior remain required.
9. Document normalization/headroom, smoothing, sample-rate scaling, and deterministic seed behavior. Avoid allocations in sample processing and keep filter transitions stable.

### Acceptance and independent checks

- Soft Exhale is clearly audible in a normal initialized patch, at ordinary velocity, without requiring an extreme gain boost. Agree and record the exact reference patch/output settings used.
- Render isolated breath and full resonator output for all four characters, low/mid/high notes, soft/medium/hard velocity, short/repeated notes, long holds, and release. Include default and extreme control values.
- Demonstrate independent audible effects of Mouth, Air, Turbulence, contour, and pressure shaping. Test zero and maximum settings without unstable filters, NaNs, accidental DC buildup, or pathological output spikes.
- Compare sample rates and block partitions using reproducible seeds. Account for expected stochastic differences; test bounded statistical behavior where waveform equality is inappropriate.
- Produce before/after WAVs with both raw levels and level-matched listening versions. Record peak/RMS levels, spectral summaries, contour plots, and normalization method. Preserve headroom and avoid clipping in comparison files.
- Verify existing patch files load, non-breath patches retain their intended behavior, and factory breath examples remain playable. Verify modulation and undo/A/B integration after merge.
- The independent agent must distinguish technical verification from subjective sound quality. If it cannot listen, mark listening unverified. Present a small representative listening set to the user for final approval; metrics cannot prove that a sound is convincingly breathy.

## Independent verification and completion

Use a different AI agent than the implementer. The verifier starts from the submitted commit, reads this specification first, derives its own cases, then inspects the diff and implementation report. Build cleanly using current repository instructions and run relevant existing regressions plus independently designed rack, breath, state, and host tests.

The implementer hands off commit ID, architecture notes, parameter/migration tables, reproduction commands, baseline/new audio, tests, timing measurements, and known limitations. The verifier reports each acceptance item as pass/fail/unverified with artifact paths and environment details. Return concrete failing cases for repair and re-test the repaired commit.

Technical completion requires a passing independent report and integrated checks for continuous modulation, undo/A/B, session restore, and permanent effects. Final breath acceptance additionally requires the user's listening approval. Never substitute a successful build or spectrum plot for that approval.
