# In-loop resonator nonlinearity: testing and acceptance

Intended location: `docs/NONLINEAR_LOOP_TESTING.md`
Owner: testing agent (GLM 5.3 or Gemini 3.8 Flash)
Implementation reference: `docs/NONLINEAR_LOOP_IMPLEMENTATION.md`

## 0. Your role

You are the acceptance agent for this feature. You verify, measure and report.

Do:

- Build the delivered source, run the suites below, record measured numbers.
- Report every failure with the exact configuration that produced it and the
  smallest reproduction you can find.
- Report measured values even when a test passes. A pass at 1.02x the threshold
  is information the implementer needs.

Do not:

- Modify anything under `Source/`. If a test cannot build against the delivered
  code, that is a finding, not something to patch.
- Push to any branch. Produce logs and a report.
- Re-run the full historical regression campaign. Run the focused suites here plus
  the golden-audio and host checks named in T1 and T9.
- Widen a threshold to make a test pass. If you believe a threshold is wrong, say
  so in the report and leave the test failing.

Report a test as **blocked** rather than **passed** if you could not run it.
Blocked is a legitimate outcome. A fabricated pass is the worst possible one.

## 1. Environment

Build under `D:\dev\build\aeriform`. The build directory must not contain a `^`
character because of JUCE's post-build steps under Ninja.

Reference toolchain: GCC 14.2 MinGW-w64, Release. Note that MinGW has no
DirectWrite and JUCE falls back to GDI text rendering, so minor text differences
in rendered screenshots are expected and are not findings.

Sample rates for every audio test: 44100, 48000, 96000. Block sizes: 64 and 512.
Where a test names one rate only, that is deliberate.

Log naming: `build/nl-<suite>-tests.log`, for example `build/nl-t3-bounds.log`.
Package artifacts under `artifacts/windows-x64-nl/`.

## 2. Contracts under test

Restated here so this document stands alone.

| ID | Contract |
| --- | --- |
| C1 | With every `*_nl_on` false, audio is bit-identical to the pre-feature build |
| C2 | Saturate is contractive: output magnitude never exceeds input magnitude, within a stated bias allowance |
| C3 | Hysteresis is peak non-expansive: the running peak of the output never exceeds the running peak of the input |
| C4 | Tension pitch shift is bounded at 51 cents and returns to nominal as energy decays |
| C5 | Friction never adds more than 2 percent of block input energy after the budget settles |
| C6 | No allocation, lock or file I/O on the audio thread |
| C7 | ADAA compensates its own half-sample delay so tuning does not shift when it is toggled |
| C8 | All new parameters are appended; current-version state loads with the feature off |

## 3. Suites

### T1. Disabled equality (P0, blocking)

Render all forty factory presets with every `*_nl_on` false, at all three sample
rates, and compare against the frozen baseline used by the existing golden-audio
harness.

Pass: exact sample equality, zero tolerance, all 120 renders.
Fail: any nonzero difference. Report the preset, rate and first differing sample
index.

This is the single most important test in the document. If it fails, stop and
report. Nothing else in this suite is meaningful until it passes.

### T2. Element-level bounds (P0, blocking)

Unit-level, calling the element directly rather than through the synth.

**T2a. Saturate contractivity.** Sweep input over -4.0 to 4.0 in steps of 1e-3.
Parameter grid: drive at 0, 25, 50, 75, 100; amount at 0, 50, 100; bias at 0.

Pass: `|y| <= |x|` for every point.

Repeat with bias at -100, -50, 50, 100. Pass: `|y| <= |x| + 0.02`. Report the
largest observed `delta` across the whole bias grid as a number.

**T2b. Hysteresis peak non-expansion.** Drive the element with 10 seconds of
white noise, swept sine and a decaying pluck envelope. Track the running peak of
input and output.

Pass: running output peak never exceeds running input peak by more than 1e-6.
Do not test pointwise `|y[n]| <= |x[n]|`. That is false by construction for a play
operator and asserting it is a test bug, not a code bug.

### T3. Full-system boundedness (P0, blocking)

Matrix: 4 models x 9 resonator models x 3 sample rates x 3 quality settings.
For each cell, drive at 100, amount at 100, in-loop position `post`, with the
energy loop enabled, all six cross routes active at maximum, and the governor in
its default state.

Excitation: 60 seconds of alternating full-scale noise bursts and held notes
across the keyboard, plus sustained 16-voice chords.

Pass:
- No NaN or Inf anywhere in the output.
- Pre-limiter bus magnitude stays below 8.0.
- The instrument recovers to normal output within 500 ms after excitation stops.

Report, do not fail on: the count of state-guard activations per cell. A guard
that fires and recovers is working. A guard that fires continuously means the
element is being driven past its design range and the drive mapping needs
rescaling, which is a finding.

Also run the friction model specifically against C5: log block input and output
energy for 60 seconds and report the maximum ratio observed after the first
200 ms, plus how long the budget took to settle from a cold start.

### T4. ADAA correctness (P1)

48 kHz. Model 1, drive 100, amount 100, pipe and string resonator models only
(the dispersive and modal models have legitimate inharmonic content that confounds
this measurement).

Play C6 at 1046.50 Hz, render 4 seconds, take a 65536-point Hann-windowed FFT of
the steady portion. Measure total energy in bins below the fundamental, excluding
bins within 25 cents of a known harmonic.

Pass, both required:
- ADAA on is at least 6 dB below ADAA off.
- ADAA on is at or below -55 dBFS relative to the fundamental.

Report both absolute numbers, not just the delta. If ADAA on is not measurably
better than ADAA off, the ADAA path is inactive or wrong, even if both numbers are
under the absolute threshold.

### T5. Tuning (P0 for T5a, P1 for T5b)

**T5a. ADAA delay compensation.** Model 1, drive 100, in-loop position `post`,
waveguide-based models. Measure fundamental by autocorrelation at A2, A3, A4, A5
with `nl_adaa` on and then off.

Pass: the measured fundamental differs by less than 2 cents between the two
settings at every pitch.

Failure mode to expect: a shift that grows with pitch, roughly doubling each
octave. That is the missing half-sample compensation described in section 8 of the
implementation doc. Report it as such.

**T5b. Tension behaviour.** Model 3, amount 100. Excite the same note at -12, -6
and 0 dBFS and measure the fundamental during the first 100 ms.

Pass, all required:
- Pitch shift is monotonically increasing with excitation level.
- Maximum shift does not exceed 55 cents.
- After decay to -40 dBFS, pitch returns to within 3 cents of the nominal value.
- No sample-level discontinuity greater than 0.25 in the output during the pitch
  glide, which would indicate the delay-length rate limit is not working.

Confirm that the existing exact-tuning tests carry an explicit exemption when
model 3 is active rather than a loosened tolerance. A loosened global tolerance is
a finding.

### T6. Audibility (P1)

For each of the four models, render a reference patch with the element on at
default amount 50 and drive 50, and the same patch with `*_nl_on` false.

Loudness-match the two renders, then compute mean absolute per-band difference
across 24 gammatone bands.

Pass: at least 8 bands differ by 1.5 dB or more.

Report the per-band figures and the count of qualifying bands for every model.
This gate exists because the sympathetic bank, coupled room and contact routing
each shipped and were documented before anyone noticed they were inaudible. A
model that clears every stability test and fails this one is still a failure.

Additionally, render the same comparison for the `pickup` position against the
`post` position. If they are indistinguishable, the in-loop insertion is not doing
anything and the element is effectively a post-effect. Report the result whether or
not it is conclusive.

### T7. Realtime safety (P0, blocking)

Extend the existing allocation interception harness, the one used for spectral
freeze, to cover this feature.

Scenario: 60 seconds of continuous playing with note-on and note-off across the
keyboard, all four models cycled, `_nl_pos` changed while notes sound, drive and
amount automated continuously, quality setting changed mid-playback.

Pass: zero intercepted allocations or frees on the audio thread, zero lock
acquisitions.

State the scope of the probe honestly in your report. The existing interceptor
covers C++ `new` and `delete` on the test thread. It does not cover raw `malloc`
in third-party code. Say which one you ran.

### T8. CPU (P1)

Default patch, 8 voices, 48 kHz, block 256. Measure percent of real time with the
feature off, then with one element active per slot, for each of the four models,
at Eco, Normal and High.

Pass: Normal quality increase is at or below 10 percent relative.
Report all figures regardless of pass or fail, in the format used by
`docs/PERFORMANCE.md`.

Also measure 16 voices at High quality and report. That configuration is not
gated but it is where a regression will show first.

### T9. Host and state (P0, blocking)

- `AeriformHostCheck` against the built VST3: PASSED.
- pluginval strictness 10 with `--validate-in-process`: SUCCESS.
- State round trip: save, reload, compare every parameter value exactly.
- Migration: load a session saved at the current released state version. Every
  `*_nl_on` must be false, every other new parameter at its documented default,
  and every pre-existing parameter unchanged.
- Parameter count: confirm 528 and confirm no existing ID changed range, default
  or meaning. Diff the generated parameter table against the previous version and
  attach the diff.

### T10. Integration and interaction (P1)

Cases that isolated tests miss:

1. **Repipe.** Engage the Repipe macro with model 3 active on all three slots and
   sustained notes held. There is a prior report of pitch resetting under Repipe
   that was not reproduced at parameter level. Tension modulation changes effective
   delay length, so this is the most likely place for that report to become real.
   Render, measure pitch continuity, report.
2. **Economy mode.** Economy uses the original single network and stops the second
   network. Confirm the element behaves consistently and that disabling it in
   Economy, if that is what the implementation does, is documented rather than
   silent.
3. **Movable filters in the same loop.** A filter and a nonlinear element in the
   same feedback path at adjacent positions. Run T3 bounds on this configuration
   specifically.
4. **Deep morph.** Morph between two snapshots where one has the element on and
   the other off, with notes held. Confirm no discontinuity and no unbounded
   output during the blend.
5. **MPE and glide.** Model 3 with continuous pitch bend. Confirm the tension
   shift composes with bend rather than fighting it.

### T11. GUI (P2)

Render the NETWORK page screenshots with each model selected. Confirm the live X-Y
plot updates and is not a static curve for models 2 and 4, and that model 3 shows a
cents readout rather than an uninformative plot. Confirm the ring buffer feeding
the plot does not appear in the T7 allocation results.

## 4. Report format

One markdown file, `artifacts/windows-x64-nl/REPORT.md`.

Header block:

```
Phase under test:
Commit hash:
Toolchain:
Date:
Suites run:
Suites blocked (with reason):
```

Then one section per suite, each containing:

- Verdict: pass, fail or blocked.
- Measured values, as numbers, including for passes.
- For failures: exact configuration, smallest reproduction, log path.
- Anything you observed that is not covered by a test in this document.

Close with a prioritised findings list: P0 blocking items first, then P1, then P2,
then observations that are not defects.

## 5. Reporting conventions

Report check counts if your harness produces them, but do not use them as the
headline quality figure. A suite reporting several million checks is usually one
assertion inside a sample loop. The meaningful figures in this document are the
measured values: the contractivity delta, the cents shift, the dB of alias
suppression, the band count in T6, the CPU percentages.

If a test in this document is ambiguous, unimplementable as written, or measures
the wrong thing, say so in the report and propose the correction. Do not silently
substitute your own version.
