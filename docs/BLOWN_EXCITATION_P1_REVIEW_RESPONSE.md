# PIPE P1 acceptance-feedback response

This records implementation changes after the supplied September 19 acceptance
report. Independent acceptance verdicts remain with the acceptance owner. No
Tests/, plugin, GUI or generated-parameter files were changed for this revision.

## T5: revised tuning, same measurement grid

The phase-only solution reproduced the reported +24.598141-cent maximum. The fix
solves for a local maximum of the full small-signal output transfer, including
filter magnitude slopes and the numerator. The phase solution seeds a bounded
delay-length search; it is no longer the final tuning objective.

Rebuilt with GCC 14.2/C++20 Release. On the same 72 impulse configurations:

| Observation | Before | After |
| --- | --- | --- |
| Maximum absolute measured spectral-peak displacement | 24.598141 cents | 0.035423 cents |
| Missing local peaks | 0 | 0 |
| Delay-clamped configurations | 0 | 0 |
| Nonfinite renders / state-guard activations | 0 / 0 | 0 / 0 |

The independent Python transfer calculation agrees with the new rendered peaks.
Its largest peak displacement from the measurement is 0.035423 cents. Analytic
frequency derivatives agree with numerical differences with maximum scaled error
1.86e-9. The reported 3-cent criterion was not changed. These are engineering
measurements for independent T5 rerun, not a self-issued suite verdict.

### Remaining tuning scope

An additional analytical grid covers 972 combinations: three sample rates, notes
33/60/93, both bores, ADAA on/off, LP 20/800/20000 Hz, HP 20/300/2000 Hz, and nominal
RT 0.001/0.5/30 s, with filter/RT key tracking disabled. A suitable local maximum
was found in 396; 576 searches report unavailable and retain the phase fallback.
This grid intentionally includes nearly extinguished loops and severe attenuation.
It does not establish impossibility outside the bounded search neighborhood.

Thus the original 72-case failure is addressed, but an unconditional promise of
3-cent tuning at every filter/loss setting is still unsupported. Driven/nonlinear
pitch and production control-rate cost also remain unverified. The code exposes
failure instead of silently calling those configurations tuned.

## Other report findings

- Physical pressure versus performance macro remains a musical decision. The
  separate macro fixture is retained; no new host macro parameter was invented.
- Five filter controls becoming two saves exactly **nine** across three slots:
  **57 -> 48 additions; 842 -> 833 total**, before any additional macro parameter.
  An extra macro per slot would add three. Final controls are not frozen by the
  evidence-only T8/T19 tables.
- Missing production parameters/UI/network suites are expected pre-integration
  dependencies, not evidence of a prototype implementation defect. Production
  integration still follows the agreed listening/control-selection gate.
- Pointwise `|f(u)| <= |u|` does not prove strict contraction: this curve has an
  identity region. Retain the narrower stability discussion, not the report's
  phrase "strict contraction holds."
- A live decay readout is a new optional UI proposal; it is not part of the
  implemented DSP revision or previously agreed v1 requirement.
- HEAD `6cc13ef...` is the checkout base, not a committed prototype revision.
  The source files are uncommitted; use `source-sha256.json` for reproduction.

## Artifacts and review

Original evidence remains in `prototypes/pipe/output/`. New evidence is in
`prototypes/pipe/output-retuned/`: raw/WAV renders, `tuning-before-after.csv`,
`math-verification.csv`, `extreme-grid.csv`, render/source hashes and summary JSON.
See the prototype README for reproduction commands.

Independent code review found no concrete defect in the transfer derivative,
bounded search, curvature check or cache dependencies. It independently confirmed
negative peak curvature after float delay rounding across the 72 impulse fixtures.
That code review does not replace acceptance or musical approval.
