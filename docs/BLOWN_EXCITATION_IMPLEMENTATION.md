# PIPE / blown excitation: agreed implementation scope

Status: P0/P1 prototype plus generated PIPE parameter contract. Not a shipping feature.
This document consolidates the September 18–19 design interview and supersedes the
two supplied implementation drafts where they disagree with these decisions.

## Boundaries

PIPE is a proposed tenth resonator model, appended at index 9. No existing DSP,
parameter, preset, UI, allocator or automation mapping changes during P1. The
prototype is not linked into the plugin and is not merged. Existing checkout
changes belong to other work and must be preserved.

Implementation may measure its work. Independent acceptance owns musical approval
and release thresholds. Do not edit Tests/ for this work. P1 must be auditioned
before production integration or freezing the parameter set.

## Ratified design

- Continuous DC/noise excitation, pressure and shared voice-envelope timing;
  independent pressure per resonator. No claim of a physical speaking threshold.
- Fixed two-pole exciter lowpass with Q, velocity and key tracking.
- Existing four-point Lagrange delay implementation; cone/non-inverting and
  cylinder/inverting feedback with full- and half-period lengths respectively.
- Following the P1 tuning failure, refine the phase-based starting length against
  the complete small-signal output-response peak. Report unavailable local peaks;
  do not claim an every-setting pitch guarantee from the impulse grid.
- Asymmetric cubic-knee saturation, drive compensation, first-order ADAA,
  normalized one-pole HP then LP; output before nominal loop gain.
- Nominal RT gain: g = 10^(-3 T_round / RT_eff), RT_eff clamped to 1 ms–30 s.
  Shorter round trips require g closer to one. Filters/interpolation/nonlinearity
  introduce additional loss; actual decay is reported separately.
- Retain shared release/allocation and the existing steal fade. Long nominal RT
  tails may be truncated by the shared output fader. No new silence termination.
- No global pitch drift or one-pole exciter option in v1. Optional fractional
  allpass is deferred unless the existing interpolator proves inadequate.
- Pressure alone is auditioned first, with RMS-matched comparisons. A separately
  identified performance macro varying pressure/noise/cutoff is a fallback.
- Short/long RT comparisons characterize the loop, not proof of pressure-driven
  expression. Sweep every provisional control in P1 to inform retention.

## Parameters and compatibility

The user subsequently authorized finalizing the parameter contract for UI work.
The **48-addition set** is now generated (833 total), with direct LP/HP cutoffs and
PIPE appended at model index 9. See PIPE_PARAMETER_HANDOFF.md for exact IDs, values
and integration limitations. The provisional counts below preserve the decision
history; the selected set is 48, not 57.

Observed baseline: 785 generated parameters. Removing blow_on and the agreed four
cuts leaves 57 provisional additions (842 total). Replacing four filter endpoints
plus morph with two cutoffs saves nine more: 48 additions / 833 total.
Shared parameter storage does not authorize reinterpretation of existing controls.

Append-only model choices are a narrow accepted compatibility exception: preserve
indices/defaults and verify every old model's state restoration. Extending choice
ranges can change normalized host automation; measure and document this before
release. Do not call existing automation preserved without evidence.

Inactive-control navigation/hiding is a proposed release prerequisite pending
real-host feasibility verification. Plugin UI hiding and host automation-list
hiding are separate requirements. Neither is necessary to compile a P1 probe.

## Source reconciliation

The current voice has shared amp/mod envelopes, two exciters and three resonators;
there are no three independent key-scaled ADSRs. FractionalDelay already uses
cubic Lagrange. The waveguide's blocker is 1.5 Hz and nonlinear Saturate additionally
has a 5 Hz wet blocker. PIPE's proposed dedicated chain must not inherit duplicate
filters accidentally. Coupling currently reads nominal feedback; production PIPE
needs explicit effective-loss integration. The allocator frees through its output
fader. Model switching fades output/taps, not circulating state.

At 96 kHz the current allocation is 8192 samples (8186 readable), minimum frequency
16 Hz: nominal cone length 6000, cylinder 3000. Phase compensation can lengthen a
delay and must still be checked against capacity. These are source observations,
not a new supported-range promise.

## Delivery sequence

1. P0: record mathematical corrections and direct numerical verification.
2. P1: compile isolated one-slot prototype, fixed render scenarios, no host
   parameters/UI; produce pressure audition, fallback macro and control sweeps.
3. Independent P1 listening verdict selects the architecture and controls.
4. Only then integrate PIPE, parameter generation/state, network interaction and
   shared lifecycle; independently verify compatibility, stability and CPU.

See BLOWN_EXCITATION_MATH_REVIEW.md and BLOWN_EXCITATION_UI_AND_TESTING.md.
