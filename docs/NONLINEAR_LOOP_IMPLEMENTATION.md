# In-loop resonator nonlinearity: implementation plan

Intended location: `docs/NONLINEAR_LOOP_IMPLEMENTATION.md`
Owner: implementation agent (Astra)
Acceptance owner: separate testing agent. See `docs/NONLINEAR_LOOP_TESTING.md`.
Status: Phases 0–5 implemented on user instruction; separate acceptance remains pending.

## Phase 5: finishing (2026-09-14)

The user requested continuation after the Phase 4 implementation. Added four
Nonlinear-category demonstrators: Loop Saturate, Loop Hysteresis, Loop Tension,
and Loop Friction. The repository already contained 42 presets, including two
effects demonstrators beyond the original forty. The new four follow all 42,
for 46 total; a source comparison confirmed the preceding entries are unchanged.
Their ordinals and favorite IDs remain stable.

Each demonstration uses slot A alone, dry original effects, and an enabled
nonlinear model at Amount 100. Friction uses A Input 0.08 to place its sine
excitation within the force curve's useful range. The initial high-input version
had only 0.005395 relative RMS difference and was revised, without relaxing the
smoke threshold. See docs/NONLINEAR_LOOP.md for playing/comparison guidance.

Added the user guide, updated parameter notes and an Unreleased changelog entry.
docs/PERFORMANCE.md explicitly leaves feature-specific CPU measurements pending
with the user's testing agent; historical measurements are not reused as evidence.

### Final cursory checks

- AeriformTests and Aeriform_Standalone Release builds succeeded.
- Nonlinear filter: 11 tests, 248985 checks, zero failures.
- New-preset check: valid parameter references/ranges, finite note/release renders,
  non-silence, and on/off signal differences at A3, velocity 110, 48 kHz/256 samples.
- Relative RMS differences (difference RMS / disabled RMS): Saturate 0.981733,
  Hysteresis 0.067549, Tension 0.989075, Friction 0.063643. These are signal
  comparisons, not listening or loudness-matched acceptance results.
- All 766 original parameter definitions remain unchanged; 19 appended = 785.
- Final panel captures render the four model states; the X/Y layout and Tension
  meter were inspected with the expected enabled/disabled controls.

Logs: ../Build/nl-phase5-build.log, ../Build/nl-phase5-tests.log and
../Build/nl-phase5-presets.log. Eight on/off WAVs and four panel PNGs are in
artifacts/nonlinear-phase5/; its README identifies the comparison settings.

Implementation through Phase 5 is delivered. Full T1–T11 acceptance, sustained
maximum-route stability, CPU measurements, and subjective preset listening remain
with the separately assigned testing agent, per the user's testing instruction.
Earlier phase status statements below are historical delivery notes.

## Phase 4: Friction (2026-09-14)

Implemented on the user's instruction to continue. The sections below this handoff
record earlier phases chronologically; their "not started" statements describe
those earlier deliveries. Phase 5 has not started.

### Budget and force

Each element accumulates double-precision squared input, requested output, and
limited output over local 32-sample DSP blocks, independent of host buffer size.
The slot closes the block after all lanes/taps; the standalone processSample API
also closes it. getFrictionEnergy() exposes the last completed block and
getFrictionScale() exposes the current force scale. These are audio-thread
inspection APIs, not atomics for live cross-thread UI polling.

blockBoundary compares the requested energy against 1.02 times the incoming
energy. An overshoot sets the next scale to 0.98*scale*sqrt(input/requested),
ramped over the following 32 samples. Otherwise it recovers toward unity with a
500 ms time constant. Requested energy is measured before the immediate guard,
so limiting cannot hide excessive force from the adaptive budget.

The first block is protected too: waveform output magnitude cannot exceed
sqrt(1.02)*input magnitude (within float rounding). This pointwise restriction
implies the same squared-energy bound for every block, lane, and tap, without
borrowing stored energy or allowing silence to seed oscillation. This is a LOCAL
element contract, not proof of stability for the entire routed network.

Force opposes x[n]-x[n-1]. Its smooth sign uses v/(abs(v)+0.0001), with static
coefficient 1, dynamic coefficient 0.2, velocity scale 0.01, and the rational
falloff 1/(1+z+0.48*z^2+0.235*z^3), z=min(100,abs(v)/0.01). No per-sample exp.
Normal force is amount*fade*(0.002+0.018*gain/32), using the existing smoothed,
keytracked Drive mapping. Contact scales by abs(x)/(abs(x)+0.02). Energy-adding
force tapers to zero at magnitude 0.5; there is no added constant excitation.
The immediate guard applies after the wet/dry strength and budget scale.

The whole-network governor cannot supply paired local input/output energy and
only regulates cross routes/returns, not intra-slot feedback. Consequently the
implementation uses exact local sums instead of reusing that governor's peak
follower. It adds no second envelope follower, heap allocation, or lock.

### Modal reconciliation and controls

Waveguide pre/post and all pickup paths use signed force, which can locally add
energy. Modal pre/post derives a nonnegative radial gain capped at unity from the
same force and applies it to both recurrence coordinates through the existing
modal hook. Those paths are deliberately dissipative. Injecting force into only
one modal coordinate, as a literal scalar adaptation would do, does not preserve
pole stability. The scalar energy diagnostic for modal paths describes the
observed coordinate; the shared gain <=1 also contracts the other coordinate.
The supplied proposal did not specify this necessary topology distinction.

All four models are now selectable and active. Drive and Amount control Friction;
Bias is disabled, and ADAA is ignored. The live X/Y view labels waveform energy
limiting or modal radial loss. Parameters, destination order, state version, and
all forty factory presets remain unchanged from the previous phase.

### Cursory verification

Release builds of AeriformTests and Aeriform_Standalone succeeded. The nonlinear
filter ran 10 tests, 248464 checks, zero failures. Added checks cover pointwise and
block energy, actual local injection, silence with retained history, budget attack
and recovery, reset, and finite output across nine resonator modes/three positions.
The rendered phase4 panel was inspected: Drive/Amount available, Bias disabled,
real X/Y trace and energy-limited label visible. Logs: ../Build/nl-phase4-build.log
and ../Build/nl-phase4-tests.log. Capture: artifacts/nonlinear-phase4/nonlinear-phase4-network.png.
The 766 original parameter definitions still match, with 785 total parameters.
All existing factory preset source was unchanged at this checkpoint.

### Acceptance handoff

Only cursory checks are owned here. The separate testing agent retains T1, T3,
T6, T7, T9, T10 and C5, particularly maximum routes plus energy loop, held chords,
all rates/qualities, release recovery, aliasing, DC, pitch, CPU, and audibility.
The pointwise budget is intentionally stricter than the proposed block-only
budget; useful sound and full-network recovery still need measurement. No claim
of physical friction accuracy or self-oscillation acceptance is made.

## Phase 3: Tension (2026-09-14)

Implemented on the user's instruction to continue after Phase 2. Separate
acceptance remains assigned to the testing agent; Friction has not been started.

### Energy and topology

The existing per-slot 15 ms RMS probe drives `1 + 0.03*amount*fade*clamp(RMS,0,1)`.
There is no second RMS follower. The maximum ratio 1.03 is approximately 51.17
cents, consistent with the proposal's 0.030 cap and the testing document's 55-cent
measurement ceiling. `NonlinearElement::tensionRatio` and `delayScale` expose the
requested ratio and reciprocal; waveform processing is identity for this model.
Pre and post share this pitch modulation. Pickup is intentionally inactive, with
an explanatory GUI message: changing loop tuning at pickup would violate that
position's output-only meaning. Drive, Bias, and ADAA do not alter Tension.

### Waveguide implementation

`Resonator::next` maintains a bounded fractional shortening independently of the
existing nominal delay/length smoother. Its per-sample step is limited to
`0.02 / nominalDelay`, and the fractional delay read uses
`nominalDelay * (1-shortening)`, subject to the existing minimum delay of two
samples. For a fixed nominal note, Tension can move the read length at most 0.02
samples per sample. Legacy pitch/glide changes are not artificially rate-limited;
the fractional representation prevents old offsets from exceeding the 1.03 bound
when nominal tuning changes. Disabling Tension slews the shortening back to zero.
The original disabled arithmetic path is retained when no shortening is present.

The existing four-point Lagrange interpolator is reused. No allpass interpolation
coefficient exists to smooth. Dispersive Tube changes only nominal delay length;
its dispersion allpass coefficients remain untouched. The small difference between
delay-ratio and measured pitch-ratio in a filter-compensated loop must be measured
in acceptance; this implementation follows the proposal's `L0 / ratio` mapping.

### Modal implementation

Each mode caches its nominal frequency, radius, and coefficients during existing
control updates. While Tension runs, current frequency-dependent coefficients are
preserved across those updates. Every 32 samples, target frequency is
`min(nominalFrequency*ratio, sampleRate*0.45)` and target a1/input gain are evaluated.
They interpolate over the following 32 samples. The a2/decay coefficient is not
changed by Tension. On return to unity, exact nominal coefficients are restored.
The engine does not recompute trigonometric coefficients per sample. All four
modal families, including the size-based Formant Body frequencies, use this path.

### GUI, telemetry and testing boundary

The existing display switches to a cents readout and horizontal meter. Audio
publishes the applied left-slot ratio through fixed atomics; conversion to cents
and rendering occur on the message thread. It is a control-state readout, not a
spectral estimator. Drive and Bias are disabled for Tension. Global ADAA remains
available for other slots using Saturate. Friction is the only reserved model.
Parameter count (785), destination count (242), and state format (4) stay unchanged.

Exact-tuning tests now carry an explicit `nominal` eligibility flag: enabled,
nonzero Tension at pre/post is exempt from the nominal-frequency assertion and
has separate bounded-envelope checks. Existing numerical tolerances and disabled
baseline tuning cases are unchanged. The measurement helper uses the slot path
so future Tension cases exercise actual integration. Focused smoke coverage also
includes the delay slew and ratio bounds, modal control updates and settling,
Repipe plus energy-loop routing, and the GUI meter. Full T5b and T10 acceptance,
including measured pitch monotonicity and live-playing interactions, remains with
the testing agent.

### Phase 3 cursory verification

MinGW Release builds of `AeriformTests` and `Aeriform_Standalone` passed (exit 0).
The eight `nonlinear_` smoke tests passed: 182,930 checks, zero failures. The
existing exact-tuning test also passed with its original thresholds: one test,
46 checks, zero failures. Checks cover the requested ratio ceiling and monotonic
energy mapping, fixed-note delay slew, nominal-pitch changes without exceeding
the ratio ceiling, modal control updates/settling, all nine resonator types,
Repipe/energy-loop finite output, and GUI meter/control availability. Full acoustic
monotonicity and decay measurements remain acceptance work, not claimed here.

Logs: `Build/nl-phase3-build.log`, `Build/nl-phase3-tests.log`,
`Build/nl-phase3-tuning.log`, and `Build/nl-phase3-layout.log` in the workspace
parent. The parameter comparison confirms the original 766 rows and factory
presets are unchanged. `artifacts/nonlinear-phase3/nonlinear-phase3-network.png`
was rendered and visually inspected: cents readout and meter fit, with Drive/Bias
disabled and Amount available. Full Phase 3 acceptance remains with the separate
testing agent. Phase 4 has not started.

## Phase 2: Hysteresis (2026-09-14)

Implemented on the user's instruction to continue. Full acceptance remains with
the separately assigned testing agent; no Phase 3 processing has been started.

Waveguide pre/post and both pickup paths use a four-operator play stack:
`state[i] = clamp(state[i], x-r[i], x+r[i])`. Thresholds are geometrically spaced
at 0.001, 0.004, 0.016, and 0.064 signal units, scaled by effective drive gain/32.
Gain mapping and upper-note key tracking are shared with Saturate. Thresholds
operate on raw input units: automation never rescales stored history. Amount
interpolates positive weights from (0.55, 0.25, 0.15, 0.05) to
(0.10, 0.20, 0.30, 0.40); the weighted sum is explicitly normalised. Amount also
controls the convex dry/wet blend, including the existing transition fade.

Each projection stays between the previous operator state and the new input.
With zero initial state and positive normalised weights, output running peak
cannot exceed input running peak. This remains true with changing thresholds
and weights. It is NOT pointwise contraction: an operator can retain a nonzero
output at zero input. Tests must include history since reset, including history
retained across model changes, when asserting this property.

There is no ADAA, static bias, extra DC blocker, or Saturate tuning compensation
on the hysteresis waveform path. A DC blocker could invalidate the advertised
peak bound; remembered offsets are allowed by the play model. Existing waveguide
and final-output DC filters remain untouched. Hysteresis has an amplitude/history
dependent response; a fixed half-sample compensation would be incorrect. Remaining
aliasing, DC, tuning and musical effects require acceptance measurements.

Modal pre/post retains the Phase 1 radial-loss architecture. A separate play stack
is driven by mode-state magnitude, and its output/input ratio is bounded to [0,1]
before attenuating both coordinates. A held value cannot amplify or reverse modal
state. At zero magnitude the history still updates, without division. This is a
memory-driven modal attenuation variant; the signed play-stack contract applies
directly to waveform/pickup processing. Waveform and modal-magnitude histories
are separate so switching resonator families cannot reinterpret signed memory.

Each position and lane owns fixed storage for its four operators. Saturate's
filter histories and Hysteresis histories are independent and retained across
model/position transitions; prepare/reset clears them. Topology switching reuses
the wet fade. Toggling ADAA while remaining in Hysteresis causes no fade or
processing change. Bias is disabled in the panel for Hysteresis; reserved-model
messaging applies only to Tension and Friction. The live plot uses the existing
paired-sample queue. Parameter count (785), destination count (242), and state
version (4) do not change. No factory presets are edited in this phase.

`docs/PARAMETERS.md` has been normalised to UTF-8, preserving the original
reference and the appended feature notes in a single consistent encoding.

### Phase 2 cursory verification

MinGW Release builds of `AeriformTests` and `Aeriform_Standalone` passed (exit 0).
`AeriformTests.exe --filter=nonlinear_` passed six tests, 104,954 checks, zero
failures. New checks cover retained output at zero input, independent lane
histories, reset, running-peak bounds during drive/amount changes, no dependence
on Saturate-only controls, and finite output while switching models across all
nine resonator types and three positions. The earlier nonlinear smoke checks
also pass. The GUI check verifies Hysteresis disables Bias.

Logs are `Build/nl-phase2-build.log`, `Build/nl-phase2-tests.log`, and
`Build/nl-phase2-layout.log` in the workspace parent. The comparison confirms
all 766 original parameter definitions and factory presets are unchanged.
`artifacts/nonlinear-phase2/nonlinear-phase2-network.png` was rendered and visually
inspected: Hysteresis is selectable, Bias is disabled, and controls/plot fit.
These are cursory implementation checks, not full acceptance. T1, T2b, T3, T6,
T7, T9, and T10 remain with the separate testing agent. Phase 3 is not started.

## Phase 1 source reconciliation (2026-09-14)

The user explicitly requested the next phase while assigning acceptance to another
agent. No separate acceptance result was present when Phase 1 started. These notes
supersede the original proposal where the actual topology requires changes.

### Saturate implementation

`NonlinearElement` implements the piecewise cubic and continuous even antiderivative.
Drive maps exponentially from 1 to 32 with approximately -3 dB/octave above 220 Hz
(minimum gain 1). Bias maps to +/-0.02 signal units and subtracts the static offset
analytically. The biased memoryless curve remains 1-Lipschitz: by the mean value
theorem, `|f(g(x+b))-f(gb)|/g <= |x|` because `0 <= f'(u) <= 1`. Bias does not
intrinsically require an extra 0.02 allowance. This is a static-transfer claim,
not a pointwise bound for the subsequent filters with memory.

First-order ADAA uses double-precision divided differences and the midpoint
fallback below transformed-input delta 1e-5. It caches the previous antiderivative.
When drive or bias moves, the previous raw sample is evaluated in the current
curve before subtraction, avoiding differences between unrelated curves.
Each of three positions has twelve independent lane histories. Drive, amount,
and bias smooth over 5 ms; topology changes fade the wet contribution to zero over
2 ms, change configuration, and fade back over 2 ms. Filter state is preserved;
only the previous ADAA interval is primed on re-entry. Unsupported models are wire
operations. No allocations, locks, or file I/O are added to processing.

The normalised 5 Hz DC blocker runs on the nonlinear branch at pre/post, and on
biased pickup processing. The dry branch remains unchanged so amount zero and
bypass are exact wires. The legacy waveguide DC blocker remains in the dry loop.
The new blocker has unity maximum frequency-response magnitude, unlike the
unnormalised blocker in the original scalar sketch.

### Phase compensation and modal correction

The original unconditional half-sample subtraction is only correct at full wet.
For partial wet, the transfer is `(1-m) + m*slope*Hdc*Hadaa`; its phase depends on
frequency and wet fraction. `NonlinearElement::loopPhaseDelay` evaluates that
small-signal transfer, including biased slope and the DC blocker's phase lead.
`Resonator::update` subtracts its phase delay from nominal delay length. At full
wet ADAA adds exactly 0.5 samples relative to ADAA off. The existing length smoother
handles control-rate changes. There is no compensation at pickup. This corrects
small-signal tuning; large-signal nonlinear behaviour must still be measured.

The Phase 0 scalar recurrence hooks cannot simply gain waveform ADAA/DC filtering:
that adds state to the modal recurrence and changes its poles. For the four modal
models, pre/post Saturate instead applies a nonnegative gain in [0,1] to BOTH
coordinates of the mode state. Pre attenuates the existing pair before advancing
and adding excitation; post attenuates the newly advanced pair before storing it.
The gain is the memoryless biased cubic output/input ratio blended by amount.
This scales the state energy without rotating the state vector; no new signed DC
term, waveform ADAA, DC filter, or modal frequency compensation is inserted.
Modal pickup uses ordinary waveform saturation/ADAA with independent tap histories.
This is an intentional modal variant. Its remaining aliasing and musical result
need separate modal acceptance measurements.

### Controls and display

Parameter count stays 785; state format stays 4. Six destinations append after
all existing destinations, taking the actual count from 236 to 242 (including None).
Modulation adds 100 times the modulation value to drive/amount, then clamps 0 to 100.
The original factory bank and original parameter definitions are unchanged.

NETWORK resonator panels now have Resonator/Nonlinearity tabs. Slot A exposes the
shared ADAA switch. Each panel has enable, model, position, drive, amount, bias,
and a live X/Y plot. A bounded SPSC queue publishes complete sample pairs with
acquire/release handoff and no overwriting of unread data. The message thread
drains the queue and draws the path. The newest voice's left network supplies the
trace; modal pre/post shows the first mode's attenuation. Reserved models are
labelled inactive. See `docs/PARAMETERS.md` for mappings.

### Cursory verification and handoff

MinGW Release builds of `AeriformTests` and `Aeriform_Standalone` passed (exit 0).
`AeriformTests.exe --filter=nonlinear_` passed four tests, 45,014 checks, zero
failures. These cover static saturation, constant-input ADAA, full-wet phase
difference, finite output through switching for all nine resonator models,
zero-amount identity, state defaults/round-trip, queue delivery, and GUI bounds.
The Phase 0 identity test now uses zero amount because Saturate is active in Phase 1.
A table comparison confirms the original 766 parameter rows are untouched; the
existing modulation destination ordering and factory preset source are unchanged.

Logs: `Build/nl-phase1-build.log`, `Build/nl-phase1-tests.log`, and
`Build/nl-phase1-layout.log` in the repository's workspace parent. Screenshot:
`artifacts/nonlinear-phase1/nonlinear-phase1-network.png`, rendered and visually
inspected; no control or plot clipping observed. Full golden audio, aliasing,
CPU budget, tuning-grid, host, and allocation acceptance remain assigned to the
separate testing agent. No Phase 2 processing has been implemented.

For element-only probes, `setParams` enables the standalone processing path and
`processSample` advances one sample of smoothing/transitions. For exact fixed
settings, use `configure(params, true)` before processing. Slot integration calls
`beginSample` once, then `at` or `modalGain` per lane without advancing smoothing
multiple times in a sample. `configuration()` returns the requested settings;
transitions may still be fading the previous topology.

## Phase 0 source reconciliation (2026-09-13)

Historical Phase 0 checkpoint: implemented, with acceptance assigned to the user's
separate testing agent. The user subsequently instructed proceeding to Phase 1. The proposal
below is retained for context; these source findings supersede its inferred
placement, parameter count, and fade-helper assumptions.

Baseline source: `6cc13ef054bef52399fa53a5b2928113b480d504` in this repository.
The pre-feature layout has 766 parameters. Nineteen appended IDs bring it to 785,
not 528. State version is now 4 (previously 3). Existing state loading resets
missing IDs to metadata defaults before restoring the tree, so v3 sessions load
with all three nonlinear enables off. Macro/snapshot restoration also defaults
missing IDs. The original factory presets and modulation destinations are unchanged.
The existing multiband saturation already ships and remains separate.

### Actual insertion sites

- **Open Pipe, Closed Pipe, String, Comb, Dispersive Tube:** `Resonator::next`
  closes the loop. Its order is delay read -> reflection/damping/string averaging
  -> dispersion -> DC blocker -> movable loop filter -> existing saturation/loss
  -> excitation/reed junction -> delay write. The main pickup uses the filtered
  delay return; the second pickup is a separate linear delay read. Consequently
  the existing loop filter is before the main pickup, but not the second tap.
  `pre` is immediately after the fractional delay read, before the filter chain;
  `post` is after the junction and immediately before the delay write. The junction
  is downstream of the filters in this source, so a literal sum -> filter -> delay
  arrangement would require reordering legacy DSP. Phase 0 preserves that order.
  Both positions are inside the existing loop. `pickup` acts on both returned taps
  after output compensation and does not modify internal delay state.
- **Modal Bank, Metallic Bar, Membrane, Formant Body:** `ModalBank::next` closes
  independent two-pole recurrences in each mode. There is no shared recirculating
  scalar input. The movable slot filter processes excitation in `ResonatorSlot::next`
  before entering the bank. `pre` processes each mode's weighted state return before
  excitation is added; `post` processes the resulting value before it is written
  to that mode's state. `pickup` acts on the two final bounded output sums, after
  the legacy AGC follower observes the main sum. These are modal recurrence hooks,
  not a new shared feedback route. There is no separate modal loop filter to straddle.
  One element object belongs to each slot, with an explicit lane argument so later
  stateful models can maintain independent mode/tap histories. Nonlinear stability
  contracts must be reassessed for these recurrences before enabling later models.

`FractionalDelay::readLagrange` uses four-point, third-order Lagrange interpolation.
Pickup and excitation-position reads are linear. Dispersion has its own first-order
allpass chain; that chain is not the fractional delay interpolator.

The network governor in `ResonatorNetwork::next` attenuates the six cross routes
and the energy-loop return. It does not regulate intra-slot recurrences. Its peak
measurement is network-wide and cannot measure a slot's before/after friction
energy budget. Later friction work needs its own slot-local energy accounting.

`ResonatorSlot::next`, `applyPending`, and `applyParams` implement resonator engine
switching using a roughly 2 ms fade-out, switch, then fade-in. There is no reusable
2 ms crossfade helper here. Phase 0 nonlinear model/position changes are identity
operations and require no audio fade or reset. Later stateful model switching must
supply an explicit transition implementation without assuming a helper exists.

### Identity element and observational probe

`Source/DSP/NonlinearElement.h` contains a statically owned identity element. It
stores all seven configuration values (including the shared ADAA toggle), returns
its input exactly for every model and position, and returns delay scale 1. No new
DC filtering, delay compensation, drive mapping, or nonlinear processing runs.
The element does not allocate; `prepare` currently needs no block storage.

Each `ResonatorSlot` owns one new 15 ms mean-square follower. It measures the
waveguide's actual delay-write sample squared, or the mean of squared internal
modal state-write samples across active modes. A constant unit sample (unit state
in every modal lane) converges to RMS 1. Values may exceed 1; this is an observation,
not a limiter. Double precision avoids overflow when squaring finite float samples.
The follower advances once per slot sample, resets with the slot, and is available
through `ResonatorSlot::getLoopEnergyRms()` and `ResonatorNetwork::loopEnergyRms(i)`.
Enabled elements receive the previous sample's smoothed RMS; disabled paths skip
that square root. Physical stereo owns independent slots/probes for each side.

Existing absolute-amplitude followers remain untouched: modal AGC and other legacy
behaviour depend on them, so replacing them with RMS would violate disabled audio
equality. The new follower never feeds legacy audio or governor calculations.

### Validation handoff

Build directory: `D:/dev/Aeriform-ORCH/Build` (existing MinGW Release configuration).
Cursory checks are named `nonlinear_phase0_identity_and_probe` and
`nonlinear_phase0_state_defaults_and_round_trip`. Run with
`AeriformTests.exe --filter=nonlinear_phase0`. They do not establish golden-audio
acceptance, host validation, or the full no-allocation contract. The separate agent
owns Phase 0 acceptance (T1, T7, T9) against the baseline commit above.

Cursory results: MinGW Release `AeriformTests` target built successfully (exit 0).
The two `nonlinear_phase0` tests passed: 7,017 checks, zero failures. A direct
comparison of the generated table against baseline confirmed all 766 existing
parameter definitions are unchanged, with exactly 19 appended; factory preset
source is unchanged. Logs are `Build/nl-phase0-build.log`,
`Build/nl-phase0-tests.log`, and `Build/nl-phase0-layout.log` in the workspace
parent of this repository. These results are not Phase 0 acceptance.

## 1. What this is

One bounded nonlinear element per resonator slot, inserted inside that slot's own
feedback loop rather than after the network. Four models ship in increasing order
of risk: material saturation, hysteresis, tension modulation, stick-slip friction.

The musical goal is amplitude-dependent response. A post-chain saturator applies
the same curve to every note at every level. An element inside the loop makes the
resonator behave differently when it is excited hard, which is the behaviour a
physical model is supposed to have and rarely does.

This is separate from the planned low/mid/high multiband saturation. That feature
remains a post-effect and is unaffected by this work.

## 2. Non-negotiable constraints

1. Every new parameter defaults to a state in which the DSP is inactive. With all
   `*_nl_on` false, rendered audio must be bit-identical to the current build for
   all forty factory presets.
2. Parameters are appended only. No existing parameter ID changes meaning, range
   or default. Bump state version; older sessions load with the feature off.
3. No allocation, no locking and no file I/O on the audio thread.
4. The forty original factory presets are not edited. New demonstrator presets are
   appended.
5. Documentation names the models by behaviour, not by physical claim. See
   section 12.

## 3. Signal path placement

The element sits inside a single resonator slot's feedback loop. Three insertion
positions are exposed, mirroring how the movable filters already address positions
in the network:

| Position | Meaning |
| --- | --- |
| `pre` | Between the loop input summing point and the loop filter |
| `post` | Between the loop filter and the delay line or modal bank input |
| `pickup` | On the output tap only, outside the recirculating path |

`post` is the default. The `pickup` position is not a feedback insertion at all
and is included because it is cheap, it is unconditionally stable, and it gives a
useful reference for listening comparisons against the two in-loop positions.

Cross-slot routes, the energy loop, the governor and coupling normalisation are
untouched by this work. The element is intra-slot only.

### Confirm against the source before coding

These are inferred from the docs and must be checked in the actual resonator class:

- The exact class and method that closes the loop for each of the nine models.
- Whether the loop filter runs before or after the pickup tap.
- Whether fractional delay uses Lagrange interpolation or an allpass. An allpass
  needs coefficient smoothing for section 7 and Lagrange does not.
- Whether the existing governor acts on intra-slot loops or only on the six cross
  routes. If only cross routes, section 9 stands on its own and cannot lean on it.
- Where the 2 ms model-switch fade lives, so model changes on the nonlinearity can
  reuse it rather than introducing a second fade mechanism.

Record the answers in this file before Phase 1.

## 4. Element interface

A single polymorphic element per slot, allocated at `prepareToPlay`, never
reallocated, never branching on model type inside the sample loop beyond a switch
on a cached enum.

```cpp
struct NonlinearElement
{
    void prepare (double sampleRate, int maxBlock) noexcept;   // allocates here only
    void reset() noexcept;                                     // clears all state
    void setModel (Model m) noexcept;                          // triggers 2 ms fade
    void setParams (float drive, float amount, float bias) noexcept;

    // Returns processed loop sample. energyRms is the slot's smoothed loop energy,
    // supplied by the caller so only one follower exists per slot.
    float processSample (float x, float energyRms) noexcept;

    // Tension model only. Returns the multiplier applied to the nominal delay
    // length this sample. Returns 1.0f for all other models.
    float delayScale() const noexcept;

    void blockBoundary (float energyIn, float energyOut) noexcept; // friction budget
};
```

Model changes crossfade over 2 ms using the existing fade helper. Do not reset
state on a model change while a note is sounding.

## 5. Model 1: material saturation

Memoryless, odd, bounded, unity small-signal gain.

Use a cubic soft clip rather than `tanh`. The anti-aliasing scheme in section 8
needs a closed-form antiderivative, and the cubic gives a cheap piecewise
polynomial one.

```
f(u) = u - u^3 / 3          for |u| <= 1
f(u) = sign(u) * 2/3        for |u| >  1

F(u) = u^2/2 - u^4/12       for |u| <= 1        (antiderivative, even)
F(u) = (2/3)|u| - 1/4       for |u| >  1
```

Applied with drive `g` in `[1, 32]` mapped from `_nl_drive`, and wet fraction `m`
from `_nl_amount`:

```
y = (1 - m) * x + m * f(g * x) / g
```

The `1/g` makeup keeps small-signal gain at unity and keeps the map contractive.
A convex blend of two contractive maps is contractive, so the `amount` control
cannot break the bound.

Bias `b` from `_nl_bias` is applied with the resulting offset removed
analytically, then a one-pole DC blocker at 5 Hz runs on the element output:

```
y = (f(g * (x + b)) - f(g * b)) / g
```

The DC blocker is mandatory whenever bias is nonzero and whenever the element is
in `pre` or `post`. DC inside a delay-line loop shifts the interpolation operating
point and detunes the resonator. This is the single most likely source of a
"tuning drifts when I turn up drive" report.

Bias relaxes strict contractivity slightly. The contract becomes
`|y| <= |x| + delta(b)` with `delta` measured and required to stay at or below
0.02 at maximum bias. Measure it, do not assume it.

## 6. Model 2: hysteresis

Do not implement Jiles-Atherton. It requires a per-sample Newton or Runge-Kutta
solve, it can diverge under fast large-signal input, and inside a feedback loop
that risk becomes permanent.

Use a Prandtl-Ishlinskii stack of play operators:

```
y_i[n] = clamp (y_i[n-1], x[n] - r_i, x[n] + r_i)
y[n]   = sum_i ( w_i * y_i[n] ),   sum_i w_i = 1
```

Four operators. Thresholds `r_i` spread geometrically over the working range and
scaled by `_nl_drive`. Weights from `_nl_amount`, normalised so they always sum to
one.

Stability property: each play operator's output at any sample lies within the
range of past input values, so the running peak of the output never exceeds the
running peak of the input. That is non-expansion in the peak sense, which is the
property the loop argument needs. It is not pointwise contractivity, and the test
doc asserts the correct one. Do not write a test that asserts `|y[n]| <= |x[n]|`
here, because that is false and will fail correctly.

Cost is roughly four compares and four multiplies per sample per slot.

## 7. Model 3: tension modulation

Loud excitation raises effective tension and sharpens pitch, settling as energy
decays.

Drive the slot's smoothed loop energy `E` through a one-pole RMS follower with a
15 ms time constant, normalised so a full-scale sustained loop reads 1.0. Then:

```
ratio = 1 + clamp (k * E, 0, 0.030)        // k from _nl_amount
```

`ratio` is the pitch multiplier. 0.030 corresponds to about 51 cents, which is the
hard ceiling. Do not expose a control that exceeds it.

**Waveguide and comb models.** Effective delay length becomes `L0 / ratio`. Two
requirements: apply the change through the existing fractional delay interpolator,
and rate-limit the change to 0.02 samples per sample. An unlimited delay-length
change produces audible pitch glitches and, when shortening, discards samples.
If the interpolator is an allpass, smooth its coefficient over the same ramp.

**Modal bank, membrane, bar, formant body.** Scale each mode frequency by `ratio`
and recompute biquad coefficients once per 32 samples, smoothing the coefficients
between updates. Per-sample coefficient recomputation is not required and is too
expensive.

**Dispersive tube.** Apply `ratio` to the nominal length only. Do not also scale
the dispersion allpass chain. The existing documented tuning limits above 5 kHz
with maximum dispersion continue to apply and are not made worse by this feature.

Existing waveguide tuning tests assert exact fundamental tuning. Those tests must
gain an explicit exemption when this model is active, not a loosened threshold.

## 8. Anti-aliasing

The exciter chain's polyphase halfband oversampling does not transfer here. You
cannot upsample inside a tuned delay loop without restructuring the resonator, and
that restructuring is out of scope.

Use first-order antiderivative anti-aliasing on model 1:

```
y[n] = (F(x[n]) - F(x[n-1])) / (x[n] - x[n-1])
```

with a fallback to `f((x[n] + x[n-1]) / 2)` when `|x[n] - x[n-1]| < 1e-5`. The
fallback branch matters. Without it the divide produces garbage on sustained tones
where consecutive samples are nearly equal, which is most of the time.

ADAA-1 introduces half a sample of group delay. Inside a tuned loop that detunes
the resonator by a fixed amount that grows with pitch. **Subtract 0.5 samples from
the nominal delay length whenever ADAA is active on an in-loop position.** Add a
comment at the subtraction site pointing at this section, and the test doc covers
it so a later refactor cannot silently remove it.

Models 2 and 4 are stateful and are not ADAA candidates. Keep their drive ranges
conservative and let the test doc measure what aliasing remains.

Global `nl_adaa` toggle, default on. The off position exists so the testing agent
can demonstrate that the ADAA path is doing something, and so users on tight CPU
budgets have an escape.

## 9. Model 4: stick-slip friction

This is the only model that injects energy by design, and the only one that can
self-oscillate. That is the intent: bow squeal, howl, edge-of-feedback behaviour.
It ships last and it is acceptable to cut it without cutting the feature.

Velocity proxy from the loop signal:

```
v[n]   = x[n] - x[n-1]
mu(v)  = mu_d + (mu_s - mu_d) * exp (-|v| / v_s)
force  = -sign(v) * mu(|v|) * N
y[n]   = x[n] + scale * force
```

`N` is bow force from `_nl_amount`. `mu_s`, `mu_d` and `v_s` are fixed internally
and not exposed. `exp` is a fast rational approximation, not `std::exp`.

**Energy budget.** At each block boundary the caller supplies loop energy before
and after the element. If the element added more than 2 percent of the incoming
block energy, `scale` is reduced over the following block until the budget holds,
and recovers with a slow ramp when it does. Reuse the governor's existing peak
follower for the measurement rather than adding a second follower.

The budget is the only thing standing between this model and a runaway. Implement
the budget before the friction curve, not after.

## 10. Parameters and state

Per slot, using the existing `res_` / `rb_` / `rc_` prefixes:

| Suffix | Type | Range | Default |
| --- | --- | --- | --- |
| `_nl_on` | bool | | false |
| `_nl_model` | choice | Saturate, Hysteresis, Tension, Friction | Saturate |
| `_nl_drive` | float | 0 to 100 | 0 |
| `_nl_amount` | float | 0 to 100 | 50 |
| `_nl_bias` | float | -100 to 100 | 0 |
| `_nl_pos` | choice | pre, post, pickup | post |

Eighteen parameters, plus one global `nl_adaa` bool defaulting true. Nineteen new
parameters, taking the total from 509 to 528. Add them to the table in
`scripts/gen_params.py` and regenerate; do not hand-edit generated output.

**Do not expose per-model sub-parameters.** Stribeck curvature, individual play
thresholds, DC blocker corner and the ADAA epsilon stay internal and are driven
from the `_nl_amount` macro curve. The plugin's parameter count is already a
usability problem and nineteen is the ceiling for this feature. Document the macro
mappings in `docs/PARAMETERS.md` instead of exposing them.

Key tracking is folded into the drive mapping rather than exposed. Loop energy per
cycle differs with pitch, so a fixed drive over-saturates high notes. Apply an
internal keytrack term of roughly -3 dB per octave above A3 to the drive before it
reaches the element, and state the figure in the parameter docs.

State version bumps. Sessions saved at the current version load with all
`*_nl_on` false and the rest at defaults.

## 11. Modulation matrix and GUI

Add six destinations: drive and amount for each of the three slots. That takes the
destination count from 52 to 58. Sources are unchanged.

GUI placement is the per-slot resonator section on the NETWORK page.

The wavefolder's static transfer curve widget will not represent three of the four
models. A hysteresis loop, an energy-dependent pitch shift and a velocity-dependent
friction characteristic are not static input-output maps. Instead, plot `y` against
`x` from a short ring of actual loop samples updated on the message thread from a
lock-free single-producer buffer. That draws the real hysteresis loop, shows the
friction model's negative-resistance region opening under sustained excitation, and
costs one small ring buffer per slot.

For the tension model the X-Y plot is uninformative. Show a cents readout and a
small meter of current pitch offset instead, in the same panel position.

## 12. Naming and documentation honesty

Call the models Saturate, Hysteresis, Tension and Friction.

Do not call them Steel, Tape, Gut, Rosin or any material name. Do not cite
Jiles-Atherton, Bouc-Wen or Prandtl-Ishlinskii in user-facing text for something
that is a four-operator play stack with hand-tuned weights. The internal docs may
name the structure accurately. The user-facing docs describe what it does.

The existing feature notes already flag the risk of labels implying unmeasured
physical accuracy. This feature is the most likely place to breach that, because
the behaviours genuinely resemble real materials and the temptation to say so is
strong.

## 13. Phases

Each phase ends in a focused commit and a handoff to the testing agent. Do not
start the next phase before the previous one is accepted.

**Phase 0: hook and probe.** Add the insertion point and the per-slot loop energy
follower with an identity element wired in at all three positions. Add the
parameters with the DSP inactive. Produce bit-identical audio on all forty presets.
No audible change ships in this phase, which is exactly why it will be tempting to
skip. Do not skip it. Everything after this depends on the hook being correct and
the identity case being provably free.

Acceptance: T1, T7, T9 in the testing doc.

**Phase 1: saturation.** Model 1, ADAA with the half-sample delay compensation, DC
blocker, drive and bias mapping, GUI curve, modulation destinations.

Acceptance: T1 through T5, T7 through T10.

**Phase 2: hysteresis.** Model 2, play stack, peak non-expansion contract.

Acceptance: T1, T2b, T3, T6, T7, T9, T10.

**Phase 3: tension.** Model 3, waveguide and modal variants, rate limiting, cents
readout, tuning test exemption.

Acceptance: T1, T3, T5b, T6, T7, T9, T10, plus the Repipe interaction case in T10.

**Phase 4: friction.** Energy budget first, then the friction curve. Highest risk.
Cut without prejudice if the budget cannot be made to hold under T3.

Acceptance: T1, T3, T6, T7, T9, T10.

**Phase 5: finishing.** Four demonstrator presets appended after the existing
forty, one per model, each chosen so the difference against the same patch with
the element off is obvious on a single listen. Docs: a new
`docs/NONLINEAR_LOOP.md` in the style of `docs/EFFECTS.md`, plus parameter table
entries and a changelog entry. Screenshots rendered and inspected.

Acceptance: full suite, T1 through T11.

## 14. CPU budget

Target for the default patch at Normal quality, 8 voices, with one element active
per slot: no more than a 10 percent relative increase over the same patch with the
feature off. Measure at all three quality settings and record in
`docs/PERFORMANCE.md` alongside the existing figures.

If model 1 with ADAA alone exceeds that, the cubic evaluation is being done twice
per sample somewhere. Cache `F(x[n])` and reuse it as `F(x[n-1])` next sample.

## 15. What would make this a failure

Worth stating so it can be recognised early:

- The element is audible only at settings that also make the resonator unstable.
- Tuning shifts when drive is raised on a model that is not Tension. That is DC
  leakage or the missing half-sample compensation, not a feature.
- Any of the forty existing presets changes by one sample.
- The friction budget holds in tests but not under live playing with the energy
  loop and cross routes engaged. Test that combination explicitly rather than
  testing the slot in isolation.
