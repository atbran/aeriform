# PIPE P0/P1 engineering handoff

Built September 19, 2026 with MinGW GCC 14.2, C++20 Release, through the standalone
CMake project at `prototypes/pipe`. No plugin targets, Tests/, existing parameters,
UI, presets or allocator code were modified. All new work remains uncommitted.
This is an implementation/measurement report, not independent musical acceptance.

## Delivered

- Consolidated implementation and independent listening/verification documents.
- Mathematical review with the additional DC, hardness, ADAA-history and delay
  indexing corrections.
- Standalone DSP reusing the actual existing FractionalDelay header.
- Reproducible numeric verifier, renderer, analysis, raw float data and WAVs.
- 118 renders: six fixed-pressure notes, four continuous sweeps, 18 single-control
  sweeps, 16 tracking contexts, two bore comparisons and 72 impulse probes.

## Evidence

| Measurement | Observed result |
| --- | --- |
| CMake Release build | Exit 0; no compiler warnings emitted |
| Raw render SHA-256 comparison across two runs | 118 identical; zero mismatches |
| Nonfinite renders / guard activations / clamped tuning fixtures | 0 / 0 / 0 |
| Primitive numerical derivative max error | 3.51e-10 |
| Corrected allpass phase versus complex transfer | Max error 3.06e-14 rad |
| Normalized HP sampled peak magnitude | 1 |
| LP / HP phase equation max error | 2.22e-16 / 1.51e-14 rad |
| Isolated nominal RT numerical error | Max absolute approximately 1.28e-13 s |
| ADAA linear phase delay | 0.5 samples |
| ADAA linear magnitude error | -2.11e-15 |
| ADAA changing-input/curve versus Simpson quadrature | Max error 1.013e-8 |
| Pure DC excitation, final second of ten-second probe | RMS 0 |
| Largest nearby full-engine spectral-peak displacement | +24.598 cents |
| Measured spectral peak versus independent analytic response | Max difference 0.0343 cents |
| Impulse spectrum versus analytic transfer, peak-normalized magnitude error | Max 0.000287 |

These are observed numerical results, not newly invented acceptance thresholds.
The mathematical grids are finite samples, not proofs over all configurations.

## Findings that prevent an acceptance claim

### Phase alignment does not guarantee an aligned spectral peak

The closed-filter cylinder at A1/44.1 kHz peaks at 55.787 Hz rather than 55 Hz.
An independently constructed full closed-loop transfer predicts 55.7869 Hz.
The implemented phase compensation is consistent with its equations; damping and
frequency-dependent loop magnitude shift the spectral maximum. Do not hide this
by reporting only the phase residual. Further tuning design or a deliberately
accepted perceptual tradeoff is needed before calling the model in tune.

### Pressure can reduce output at high settings

At nominal RT 3 s, pressure 0.08 / 0.30 / 0.80 produces steady RMS approximately
0.0224 / 0.0728 / 0.0466 in the selected fixture. Spectral centroids are approximately
829 / 897 / 1539 Hz. This shows a changing spectrum and non-monotonic output, not
proof of an expressive blown instrument. The pressure and macro auditions must be
judged by ear. Pure DC converges to silence in the separate ten-second fixture.

## Review

An independent code reviewer found no concrete DSP math defect. Two evidence gaps
were corrected: tracking sweeps now use notes 48/72 and velocities 0.2/0.9; ADAA
diagnostics now cover changing-input quotient evaluation and linear frequency
response. The reviewer confirmed both corrections. Musical and production
acceptance remain unassessed.

## Listening artifacts

Generated locally under `prototypes/pipe/output/` (ignored, regenerable):

- `pressure-audition-matched.wav`: six notes, equal steady RMS; short then long RT,
  each at pressure 0.08, 0.30, 0.80. Starts: 0, 3.5, 7, 10.5, 14, 17.5 s.
- `sweep-audition-raw.wav`: pressure-only then macro at short RT, then that pair at
  long RT. Starts: 0, 8.5, 17, 25.5 s. The macro changes noise and cutoff too.
- `control-*.wav`, `tracking-*.wav`, `bore-*.wav`: isolated audition fixtures.
- `math-verification.csv`, `renders.csv`, `tuning-decay.csv`,
  `pressure-metrics.csv`, `summary.json`, `render-sha256.json`: measurements.

See `prototypes/pipe/README.md` for build and regeneration commands. Source-controlled
production integration, final parameter selection, host compatibility/navigation,
network stability, alias assessment and full-plugin CPU remain future phases,
conditional on the previously agreed P1 decision.
