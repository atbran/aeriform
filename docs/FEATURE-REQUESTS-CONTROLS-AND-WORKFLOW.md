# Aeriform: controls, expression, and patch workflow

## Assignment and agreed scope

This is an implementation handoff for requests 2, 4, 12, and 16, plus the reported missing modulation coverage. A separate AI agent must verify the implementation. The user will delegate the work; this document does not dispatch agents.

Deliver individual resonator dry/wet controls, complete ordinary-keyboard expression support, complete patch undo/redo and A/B comparison, improved control ergonomics, and modulation support for all continuous sound controls. Macros already exist: integrate with the latest implementation rather than rebuilding them. Additional MPE development is deferred; preserve existing behavior.

The companion document, FEATURE-REQUESTS-ADVANCED-DSP.md, owns the modular rack and breath overhaul. Coordinate shared parameter, state, modulation, and UI changes with its implementer. Randomization expansion, additional network-diagram features, a new feedback-protection feature, and Coupled Room engine expansion are outside this assignment.

## Start with an implementation inventory

1. Read the current repository instructions and build/testing documentation. Inspect the actual branch supplied for implementation; this handoff's file pointers are starting points, not proof that another agent's recent work is absent.
2. Inspect Source/Params/ParamTable.inc, ParameterLayout.h, DSP/ResonatorNetwork.*, DSP/SynthEngine.cpp, DSP/Voice.*, State/PatchStateManager.*, GUI controls, and Tests. Existing code contains global Network Mix, MPE/controller handling, modulation destinations, an UndoManager, and A/B-related snapshot machinery. Extend these systems rather than adding competing state owners.
3. Produce a short inventory of existing behavior, missing behavior, and relevant tests. Explicitly distinguish Body Mix from resonator dry/wet and snapshot morphing from editable A/B comparison.
4. Before coding shared interfaces, agree with the advanced-DSP implementer on stable parameter identifiers, modulation registration, full patch snapshots, and rack state serialization. Record the agreement in the implementation report.

Completion: each requested behavior has an identified implementation path and an existing-or-new test location.

## 1. Individual resonator dry/wet

### Theory and intended behavior

Each resonator acts like an insert: its incoming signal is blended with its processed output. The blended signal is what subsequent serial stages and cross-feedback connections receive. This is distinct from overall Network Mix, which remains available.

For input x, processed output r, and wet amount w, use y = (1-w)x + wr as the initial blend law. A linear law preserves unity for identical correlated signals; equal-power blending can boost correlated paths. Document any necessary departure with measurements. At 0% the stage passes its input; at 100% it matches the existing processed stage.

### Concrete work

1. Add stable A/B/C wet parameters with 100% defaults so existing patches retain prior routing behavior. Add equivalent knobs to their corresponding Main tabs and dedicated resonator controls using shared bindings.
2. Identify each stage's complete input, including injection, serial sends, and cross-feedback. Apply the blend at the stage output before fan-out. Preserve intentional feedback delays; blending must not introduce a same-sample algebraic loop.
3. Define interaction with disabled resonators, bypass, output taps, and internal tails. Preserve existing enable semantics and keep internal state advancement predictable so moving wet upward does not unexpectedly resurrect stale state.
4. Smooth changes at a suitable DSP rate. Register all three controls as modulation destinations and include them in preset/session persistence, undo, A/B, tooltips, and host automation.

### Acceptance and independent checks

- Verify dry and wet endpoints numerically with deterministic input; compare 100% to baseline renders.
- Test A, B, and C independently and in every routing mode, with all output taps and representative cross-feedback paths. Confirm downstream stages receive the blend, not an unblended side path.
- Sweep and modulate wet at 44.1/48/96 kHz with small and irregular block sizes. Check finite output, discontinuities, and high-feedback stability.
- Save/reload, undo/redo, and A/B-switch distinct wet settings. Both UI locations must display the same base value.

## 2. Ordinary-keyboard expression

### Theory and intended behavior

Performance MIDI is live input to a patch, not a stream of patch edits. Support velocity, pitch bend, mod wheel, channel aftertouch, polyphonic aftertouch where the controller supplies it, sustain CC64, expression CC11, and breath CC2. Existing per-note and channel semantics must remain distinct. MPE expansion is deferred.

### Concrete work

1. Trace every listed message from MIDI ingestion through voice assignment and modulation evaluation to audible output. Reuse existing paths and repair gaps.
2. Make appropriate expression sources available in the modulation UI with clear names and ranges. Keep sustain as note-lifecycle behavior, while pressure/wheel/expression/breath can drive assignable sound parameters.
3. Establish predictable initialization and reset behavior. Expression should not mute an untouched keyboard session; a zero-valued unassigned controller must not suppress sound. Honor MIDI channel and note targeting, note-off, sustain release, all-notes-off, and voice stealing.
4. Preserve MIDI Learn through control context menus. Keep the removed MIDI Control panel removed. Provide concise tooltips and an ordinary-keyboard example patch or documented setup.

### Acceptance and independent checks

- Inject MIDI sequences for each source and verify both source values and an audible assigned destination response.
- Hold two pitches and send polyphonic aftertouch to one: only its intended voices should respond. Verify channel pressure affects the intended channel.
- Test pedal down/up around note releases, repeated notes, stealing, controller reset, and all-notes-off. No stuck voices.
- Verify normal keyboard operation without expression messages and regression-test existing MPE paths without expanding them.
- Confirm performance messages do not populate patch undo history.

## 3. Complete patch undo/redo and editable A/B

### Theory and intended behavior

An undo transaction represents a user's action, not each intermediate numeric update. A/B represents two independent editable sound configurations. Audio buffers and live held notes are runtime state, not patch configuration.

### Concrete work

1. Audit the existing PatchStateManager and wire every patch-editing path into one transaction system: knob drags, numeric entry, resets, modulation edits, expression assignments, and applicable structural changes such as rack edits.
2. One drag is one undo step. Group multi-parameter actions into one transaction. New edits after undo invalidate redo. Restoration must not recursively record new actions.
3. Exclude incoming performance MIDI and host automation playback from edit history. Preserve correct host gesture notifications for actual UI edits and restorations.
4. Extend existing A/B machinery to capture the complete sound configuration, including modulation assignments and rack structure when available. Editing A must not overwrite B. Provide explicit copy A-to-B/B-to-A behavior and an unambiguous active-side indicator.
5. Reconcile comparison with existing morph features rather than replacing them silently. Persist both comparison states in sessions; document preset-file behavior. Switching must restore configuration safely without pretending to rewind live delay buffers or held notes.
6. Bound history memory and keep serialization, allocation, and transaction bookkeeping off the audio callback.

### Acceptance and independent checks

- Exercise drag, text entry, reset, assignment edit, and grouped action, then undo/redo and compare complete serialized configuration.
- Verify divergent edits clear redo and continuous host automation creates no undo storm.
- Make intentionally different A/B patches, edit both repeatedly, copy each direction, reload the session, and compare all stored sound fields.
- Include rack order/type/parameters once the companion work lands. Test switching while notes and effect tails are active for crashes and abrupt transition defects.

## 4. Continuous modulation coverage and ergonomics

### Theory and intended behavior

Every continuous sound control must be usable as a modulation destination. A visible assignment alone is insufficient: its result must reach DSP. UI base values and transient modulated values should remain separate so modulation does not rewrite presets or host automation.

### Concrete work

1. Generate a parameter coverage table from the current parameter registry: identifier, continuous/discrete classification, sound/administrative role, destination availability, DSP consumer, global/per-voice scope, and test evidence. Review every continuous sound parameter, including advanced pages and effects. Administrative controls such as UI scale are outside the requirement; discrete selectors and switches are outside this request.
2. Replace or extend the limited destination mapping without renumbering existing saved assignments. Prefer shared metadata to separate lists that drift. Preserve current matrix slot count and behavior.
3. Define appropriate modulation ranges and domains, especially logarithmic frequency/time controls. Clamp safely, smooth where needed, and define global versus per-voice evaluation explicitly. Address modulation of modulation-related continuous controls with bounded, deterministic dependency handling rather than recursion.
4. Make every eligible destination discoverable through existing assignment mechanisms. This request does not require the separately deferred drag-and-drop modulation workflow.
5. Integrate new resonator wet controls and, after merge, continuous rack and breath controls. Preserve stable automation IDs and avoid writing modulated values back into base parameters.
6. Standardize knob ordering, sizing, labels, fine adjustment, reset gestures, and numeric entry using shared control components. Preserve units and fractional precision. Keep interaction-driven readouts and concise tooltips describing audible effects.
7. Preserve every original parameter and Main-page priorities, the six-page navigation, and the logical 1180 x 820 layout where practical. Verify supported scaling rather than hiding controls to make screenshots fit.

### Acceptance and independent checks

- Independently regenerate the continuous-control inventory and reconcile every row. No unexplained missing continuous sound destinations.
- Probe each destination with a known modulation source under an appropriate active DSP configuration. Verify the effective value at its consumer; use representative audio comparisons across every subsystem to catch disconnected consumers.
- Test negative/positive depths, zero depth, extrema, multiple assignments, preset reload, and legacy destination migration. Test global effects with polyphony to catch accidental per-voice multiplication.
- Inspect every page and sub-tab at supported scales. Verify drag, fine drag, numeric entry, reset, tooltip, and keyboard focus behavior. Record screenshots of affected layouts.

## Independent verification protocol and handoff

The verifier must be a different AI agent from the implementer. Start from the submitted commit and a clean build. Read this specification before the implementation report, derive a requirement checklist, then inspect the diff and run existing and independently designed tests. Passing the implementer's tests alone is insufficient.

Use the repository's current build instructions. Exercise supported plugin/standalone targets and host save/reload. Record sample rates, block sizes, seeds, commands, commit IDs, and logs. Reproduce each failure minimally and return it to the implementer; re-test fixes on their new commit. Separate baseline failures from introduced failures.

Deliver a verification report mapping every acceptance item to evidence and pass/fail/unverified. Include the modulation coverage table, MIDI sequences, state round-trip results, screenshots, and unresolved limitations. Never label an unperformed host or listening check as passed. Completion requires all requested behavior to pass independent verification, including integration checks after the companion changes merge.
