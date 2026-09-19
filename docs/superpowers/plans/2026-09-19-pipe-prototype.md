# PIPE P0/P1 Implementation Plan

> Execute inline with the executing-plans workflow. Preserve the dirty checkout.

**Goal:** Build the approved, unmerged one-slot listening prototype and evidence.
**Architecture:** Standalone C++ DSP/render executable reusing FractionalDelay.h;
Python/NumPy measurements and WAV assembly. No plugin target edits.
**Spec:** docs/BLOWN_EXCITATION_IMPLEMENTATION.md

## Global constraints

No Tests/ edits, acceptance threshold changes, plugin parameter/UI changes or merge.
The preceding interview and request to implement authorize P0/P1. Independent P1
listening remains required before production integration.

- [x] Reconcile source and write corrected scope/math/acceptance handoff.
- [x] Build prototypes/pipe/verify.cpp numeric diagnostic before DSP implementation;
      cover primitive derivative, allpass identities, HP peak and isolated gain.
- [x] Implement prototypes/pipe/PipePrototype.h using the existing delay header,
      two-pole exciter, normalized loop filters, corrected phase solve and ADAA.
- [x] Build prototypes/pipe/render.cpp for pressure/macro/control sweeps and impulses.
- [x] Run analysis.py to generate pitch/decay/control metrics and RMS-matched audio.
- [x] Review measured errors, verify deterministic rerenders and record limitations.
- [x] Deliver auditable P1 artifacts for listening; leave production steps pending.

Results: docs/BLOWN_EXCITATION_P1_REPORT.md. P1 musical acceptance remains open;
checked implementation tasks do not constitute a phase acceptance verdict.
