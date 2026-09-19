# PIPE prototype: listening and independent acceptance handoff

P1 is a standalone DSP experiment, not plugin acceptance. No UI or host parameters
are added. The implementing agent reports measurements; the listener/independent
acceptance owner decides whether the sound justifies integration.

## Listening sequence

1. Compare low/mid/high fixed-pressure notes, raw and RMS-matched, at short and long
   nominal RT. Compare timbre, not the louder sample's apparent quality.
2. Listen to continuous pressure sweeps. No hard onset is promised by this passive
   architecture. Record whether soft emergence is musically useful.
3. Compare the separately labeled macro that changes pressure, DC/noise and cutoff.
   Do not attribute macro expression to pressure alone.
4. Listen to isolated control sweeps. RT/key tracking/filter endpoints and morph,
   exciter cutoff/Q/tracking, drive/hardness/asymmetry, pressure/noise and bore are
   provisional. Tracking sweeps include different note/velocity contexts.
5. Compare filter morph with coordinated HP/LP motion: their reachable static
   responses are equivalent; assess whether the extra endpoints aid performance.

## Measurement split

- Gain-law isolation: integer delay, no filters, interpolation, ADAA or saturation;
  report measured decay for multiple periods and nominal RT values.
- Full engine: report pitch near the intended fundamental and actual decay; do not
  reuse the isolated-law tolerance. A phase root alone is not a measured spectral
  peak, and a noise peak is not a deterministic oscillator frequency.
- Small-signal tuning: impulse excitation, low amplitude, both polarities, A1–A6,
  open/closed filters, 44.1/48/96 kHz. Report errors and clamped configurations.
- Report direct checks of transfer phases, HP magnitude, saturation primitive,
  ADAA small-signal delay/magnitude and automated-curve history handling.
- Deterministic PRNG supports exact rerender comparisons. Do not normalize away
  clipping or nonfinite samples: preserve raw renders and state-guard counts.

Production acceptance remains outstanding: mixed-slot routing/coupling, automation
and state migration, release/stealing, alias assessment, UI/host navigation and CPU
at 8/16 voices across quality modes. No claim about these follows from P1.
