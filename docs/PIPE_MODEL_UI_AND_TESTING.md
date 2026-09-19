# PIPE resonator model: UI implementation and acceptance testing

Intended location: `docs/PIPE_MODEL_UI_AND_TESTING.md`
Owner: UI and acceptance agent (Gemini)
DSP reference: `docs/PIPE_MODEL_IMPLEMENTATION.md`
Supersedes: all previous versions of this document, including every version
titled "Blown excitation".

**Status of the DSP reference.** This document is written against the decisions
agreed in review. The implementation document has not yet been revised to match
and still describes the superseded design. Where the two disagree, this document
is current. Do not build UI or tests against the stale implementation doc.

## 0. What changed, and why suites were deleted

The design changed substantially in review. Nine errors were found in the previous
specifications, two of them by a spot check that took seconds. Suites testing
things that no longer exist are deleted rather than left in place, because a suite
that tests an absent feature produces a confident pass that means nothing.

| Change | Test consequence |
| --- | --- |
| PIPE is a tenth resonator model at index 9, not a mode flag on existing models | `_blow_on` is gone. New suite T2 covers model index compatibility |
| Loop gain derived from decay time, always below one | Push interlock and combined-runaway suites deleted |
| `_rt` is nominal decay before additional losses | Split into T3 (isolated law) and T4 (characterisation, no threshold) |
| Existing cubic Lagrange interpolator retained | Hermite verification deleted; T5 uses Lagrange phase delay |
| Existing allocator, release and steal fade retained | Lifecycle suite rewritten; truncated tails are expected, not defects |
| Stability claim narrowed to the isolated fixed-coefficient loop | Time-varying and coupled-network behaviour is empirical, per T18 |
| Pressure-ratio claim withdrawn as false | New T8 exists to settle whether pressure alone carries expression |
| Parameter baseline is 785, not 509 | T17 revised, and parameter hiding is now a prerequisite |

## 1. Two jobs, and the conflict between them

You build the PIPE page, and you accept the DSP.

DSP acceptance is genuinely independent. You did not write that code, you cannot
modify it, and your verdict is the only one that counts.

UI acceptance is not independent, because you are grading work you did. Keep it to
mechanically checkable facts: does every control bind to a real parameter, does a
change here appear on NETWORK, does a display match the DSP's actual behaviour.
Anything requiring taste is an observation for the human, not a pass you award
yourself.

**Boundaries.** You may write GUI code, anything under `Tests/`, and the report.
You may not write DSP under `Source/` or touch parameter definitions. If a DSP
test fails, report it. Do not fix it and do not hide it in the UI. A UI that
conceals a DSP defect is worse than the defect.

Astra also measures. That overlap is deliberate. Astra reports numbers, you
re-derive them independently, and divergence between the two is a finding in its
own right. Given the error rate in review so far, that redundancy is earning its
cost.

**What neither of you can decide.** Two suites in this document, T8 and T19,
gather evidence for design decisions rather than pass or fail a contract. Whether
a sound is musically compelling is not measurable, and neither agent should claim
it. Your job there is to produce loudness-matched renders and per-band
measurements and present them. The human decides.

Report a suite as **blocked** rather than **passed** if you could not run it.
Never widen a threshold to obtain a pass. If a threshold here is wrong, say so and
leave the test failing.

Do not begin UI work until the parameters exist in the generated table. Build
against real IDs, never placeholders.

## 2. Environment

Build under `D:\dev\build\aeriform`. The directory must not contain a `^`.

GCC 14.2 MinGW-w64, Release. MinGW has no DirectWrite so JUCE falls back to GDI
text rendering. Minor screenshot text differences are expected and are not
findings.

Audio tests at 44100, 48000, 96000 Hz, blocks 64 and 512, unless a suite names one
configuration.

Logs `build/pipe-<suite>.log`, artifacts `artifacts/windows-x64-pipe/`.

---

# Part 1: the PIPE page

## 1.1 Entry point

PIPE is a resonator model, not a mode. A slot runs it by having its model selector
set to PIPE at index 9.

This has a UI consequence that needs a deliberate answer: what the PIPE page shows
when the selected slot is running one of the other nine models. Twenty greyed
controls with no explanation is the wrong answer and is the most likely default.

Show the model selector for the selected slot prominently at the top of the page,
always, with a single line stating that these controls apply when the slot's model
is PIPE. A user who lands on the page with a plucked string selected should be
able to understand and act without leaving.

## 1.2 Hard requirement: no page-local state

Every control binds directly to an APVTS parameter. No shadow values, no caches,
no apply button. Changing a control on PIPE moves the same control on NETWORK and
the reverse. If you find yourself writing synchronisation code between the pages,
the binding is wrong.

The slot selector is UI state and may be page-local, since it selects rather than
sets.

## 1.3 Panels

**STEAM**: Pressure, DC/Noise, Cutoff, Resonance, Key track, Velocity track.
Envelope timing is shared across slots and lives where it already lives. Pressure
is per-slot and belongs here.

**PIPE**: Model selector, Decay time (see 1.4), Decay key track, Damp, filter
controls (see 1.5), Drive, Knee, Asymmetry, Bore.

**SPACE**: existing room and reverb, with a voiced default rather than a neutral
one. Nobody hears an instrument dry, and a dry physical model will always sound
harsher than one with a space baked in.

## 1.4 Labelling decay honestly

`_rt` is nominal decay before the loop filters, saturator and interpolator add
their own losses. Measured decay will always come in under the nominal figure, by
an amount that varies with filter settings.

If the control is labelled "Decay time" in seconds with no qualification, users
will measure it, find it short, and report a bug. Label it as nominal, and if
space allows show measured decay alongside it as a separate readout.

This is a case where the honest label costs a word and saves a support thread.

## 1.5 Filter controls are provisional

The current parameter set gives each filter two morph endpoints plus a shared
morph position, five parameters where two plain cutoffs would do. Whether the
morph earns its extra three is a P1 question, covered by T19.

Build the UI so that collapsing to single cutoffs later is a layout change rather
than a rewrite. Do not design a panel that only makes sense with the morph
present.

## 1.6 Displays

- Effective loop gain per slot, in dB below unity. With `g` derived from decay
  time this is not visible anywhere else, and it is what explains why a note rings
  the way it does.
- Total phase compensation in samples, per slot. This is a diagnostic display as
  much as a user one, and it is how a tuning complaint gets resolved in the field
  rather than by bisecting source.
- Measured decay alongside nominal, per 1.4, if space allows.
- Asymmetry deserves visual prominence. It is the largest timbral control in the
  loop and burying it in a row of identical knobs wastes it.

Every display reads from a lock-free single-producer buffer written on the audio
thread. None may allocate. T14 covers this and it covers your code, not Astra's.

## 1.7 UI acceptance criteria

The only UI items you may pass or fail yourself:

1. Every PIPE control maps to a parameter ID in the generated table. Enumerate and
   assert.
2. Bidirectional binding: host API to PIPE, PIPE to host and to NETWORK.
3. No page-local storage of parameter values. Audit by inspection and state that
   you did.
4. The page is comprehensible when the selected slot is not running PIPE, per 1.1.
5. Decay is labelled as nominal, per 1.4.
6. Host automation of every PIPE parameter is reflected live.
7. Renders at every supported window scale without clipping.
8. No allocation on the audio thread from any display.

Everything else about the page is an observation in the report.

---

# Part 2: acceptance suites

## Contracts

| ID | Contract |
| --- | --- |
| C1 | With no slot set to PIPE, audio is bit-identical to the pre-feature build |
| C2 | Existing model indices 0 to 8 load unchanged from saved sessions |
| C3 | The gain law, in isolation, produces the requested T60 |
| C4 | Measured pitch is within 3 cents of nominal at every filter setting |
| C5 | The loop highpass magnitude never exceeds unity at any frequency |
| C6 | No DC in the loop or the output at any DC/Noise setting |
| C7 | Bore switching changes register by exactly one octave |
| C8 | No allocation, lock or file I/O on the audio thread |

C3 is scoped to the isolated law deliberately. Full-engine decay is characterised,
not contracted. See T3 and T4.

## T1. Disabled equality (P0, blocking)

All forty factory presets, three sample rates, no slot set to model index 9.
Compare against the frozen golden-audio baseline.

Pass: exact sample equality, zero tolerance, 120 renders.

If T1 fails, stop and report. Nothing else is meaningful.

## T2. Model index compatibility (P0, blocking)

New suite. Appending PIPE at index 9 preserves existing indices but extends the
selector range, and the exposure is normalized host automation.

Why it matters concretely: VST3 passes choice parameters as 0 to 1. With nine
choices, index 4 sits at 0.5. With ten choices, 0.5 falls between indices 4 and 5
and rounding decides. Any automation lane written against the nine-choice range
can land on a different model.

Tests:

1. **Saved sessions, index by index.** Build nine sessions, one per existing
   model, saved with the pre-feature build. Load each in the new build and assert
   the model selector reads the same model. Assert rendered audio is identical.
2. **Normalized round trip.** For each of the ten indices, set via the host as a
   normalized value, read back, assert the same index. Report the normalized
   value that now maps to each index, for both nine and ten choices, as a table.
3. **Automation shift, documented not fixed.** Build an automation lane against
   the nine-choice range, load it in the ten-choice build, and record which
   indices change. This is expected behaviour under the agreed compatibility
   exception. The finding is whether it is documented in the changelog, not
   whether it happens.
4. Assert index 9 is PIPE and that no index 0 to 8 changed meaning.

## T3. Gain law in isolation (P0, blocking)

The previous version of this suite could not do its job, because "filters wide
open" still attenuates, and over hundreds of round trips at long nominal decay a
per-trip magnitude of 0.999 is a large cumulative error. Interpolation and ADAA
contribute their own losses.

Configuration: integer delay length only, loop filters bypassed, saturator
bypassed, ADAA off. `g` is then the only gain element in the loop, so the
measurement tests the formula and nothing else.

Measure T60 by fitting the decay envelope in dB over -10 to -50 dBFS. Pitches A1
through A6, nominal `_rt` at 0.1 s, 1 s and 5 s, decay key tracking at zero.

Pass: measured T60 within 5 percent of nominal at every pitch. The threshold is
tighter than the previous 10 percent because the setup is now genuinely isolated
and there is nothing left to explain a discrepancy.

Report all eighteen values.

**Failure signature to watch for.** The central claim is that decay is
pitch-independent. If measured T60 falls as pitch rises, the implementation has a
fixed `g` rather than one derived from round-trip time. Worth stating the correct
direction explicitly, because the previous specification had it backwards in prose
twice: with the derived law, a high note has a shorter round trip so `g` is closer
to one, and the note still rings for the same duration. With a fixed `g`, a high
note makes more round trips per second and decays faster.

Then set decay key tracking to 100 percent and confirm T60 halves per octave
upward, within 10 percent.

## T4. Full-engine decay characterisation (P1, no threshold)

Report only. No pass or fail.

Full PIPE engine, everything active. Measure T60 across a grid: nominal `_rt` at
0.1 s, 1 s, 5 s and 20 s; loop lowpass at 20 kHz, 4 kHz and 800 Hz; drive at
minimum and at 8.

Report the ratio of measured to nominal in every cell, as a table.

The purpose is to characterise how far nominal diverges from actual across the
usable range, so the UI readout in 1.4 can be calibrated and so the divergence is
documented rather than discovered. If the ratio falls below roughly 0.5 anywhere
in normal use, that is worth flagging as an observation, since a control reading
5 s that delivers 2 s will be perceived as broken regardless of how it is
labelled.

## T5. Phase compensation and tuning (P0, blocking)

The highest-value suite here. A missing or wrong compensation produces a tuning
error that grows as filters close, easily misdiagnosed as anything else.

Note that the compensation sum must include the existing cubic Lagrange
interpolator's phase delay, re-derived against what is actually in the code rather
than against any interpolator named in an earlier draft.

Measure the fundamental by autocorrelation at A1 through A6. At each pitch sweep
the loop lowpass from 20 kHz down to 500 Hz and the highpass from 20 Hz up to
1 kHz, five points on each.

Pass: within 3 cents of nominal at every combination.

Three failure signatures to distinguish:

- Error grows as the lowpass closes: the lowpass phase term is missing or has the
  wrong sign.
- Error grows with pitch at fixed filter settings: the ADAA half-sample term is
  missing.
- Constant offset at all settings: the interpolator's phase delay is missing.

Also measure with ADAA toggled at otherwise identical settings. The difference is
the half-sample term and must be under 2 cents.

## T6. Highpass magnitude (P0, blocking)

Verifies the `(1+a)/2` normalisation directly, because dropping it as a redundant
scale factor is the most plausible implementation slip in the spec and it is a
gain path above unity inside a feedback loop.

Drive the loop highpass in isolation with a logarithmic sweep. Measure magnitude
across the spectrum with cutoff at 20 Hz, 200 Hz and 2 kHz.

Pass: magnitude never exceeds 1.0 at any frequency at any cutoff. Report the
measured maximum, which should sit very close to 1.0 and never above.

A maximum near 2.0 at high cutoffs means the normalisation is absent.

Repeat for the lowpass, where the maximum should be 1.0 at DC.

## T7. Sustained tone equilibrium (P0)

One slot on PIPE, pressure 70, DC/Noise 30, nominal `_rt` 10 s, saturator active.

Hold for 20 seconds.

Pass:

- Output RMS in one-second windows varies by no more than 2 dB after the first two
  seconds. Continued growth means no equilibrium; decay means the saturator is
  limiting below the intended level.
- No sample-level discontinuity above 0.25 at note-off.

Note that ring-down after note-off is governed by the shared allocator fader, not
by the loop, so do not test for a natural tail here. See T13.

Sweep pressure at 20, 50, 80 and 100 and report settled RMS at each. Settled
amplitude should rise with pressure and compress as the saturator engages. A
pressure control that does not change settled amplitude means the DC term is not
reaching the saturator.

## T8. Pressure expression audition (P1, evidence only)

No pass or fail. This suite settles a design question and the verdict is the
human's.

**Why it exists.** A previous draft claimed that raising pressure grows the tonal
component against a noise floor that does not. That claim is false. The exciter is
`pressure · env · ((1−m) + m·noise)`, which is linear in pressure, so both terms
scale identically and the ratio is fixed at `(1−m)/m` regardless of pressure. In
the linear regime, pressure is a volume control.

Two mechanisms could still produce timbral change, and both are weaker than
claimed. Saturation compresses peaks more than the mean, which pushes the ratio
the wrong way. The loop is narrowband and amplifies noise near its resonances,
which does produce tone emerging from noise, but that is a property of the loop
and is not pressure-dependent.

**Method.** Astra produces renders. You loudness-match and measure.

1. Pressure sweep at 20, 40, 60, 80, 100, everything else fixed. Loudness-match
   all five to equal integrated loudness, so what is compared is timbre rather
   than level. Compute per-band differences across 24 gammatone bands between
   adjacent steps, and the harmonic-to-noise ratio at each step.
2. Repeat at nominal `_rt` of 0.2 s and 10 s. This is diagnostic only. It
   characterises where emergence comes from and does not establish that pressure
   drives expression, which is the actual open question.
3. If the loudness-matched sweep shows little differentiation, repeat with a
   performance macro driving pressure, DC/Noise and exciter cutoff together, and
   measure the same way.

**Report.** The per-band and harmonic-to-noise figures, the loudness-matched
renders themselves for listening, and a plain statement of whether pressure alone
produced measurable timbral change. Do not characterise any result as compelling
or not compelling. That word belongs to the human.

**Consequence.** If pressure alone is flat, the performance macro becomes part of
the design, and it must stay a distinct parameter from physical pressure. One is a
modeled quantity and the other is a playing-interface mapping, and conflating them
is how a parameter set becomes incoherent.

## T9. DC audit (P0)

DC/Noise at 0 (pure DC), 50 and 100. Held note, 10 seconds.

Pass: mean of the output over any one-second window below -80 dBFS.

Instrument the loop signal directly if you can and assert the same there. If not,
report that the loop-internal check was not possible and why.

A nonzero mean scaling with the DC setting means the loop highpass is not removing
DC across round trips, and it will also show as pitch error in T5.

## T10. Saturator symmetry and contractivity (P1)

Element level, calling the saturator directly.

**Contractivity.** Sweep input over -4 to 4 in steps of 1e-3 across a grid of
drive, knee and asymmetry. Assert `|f(u)| <= |u|` at every point, zero tolerance.
This is the stability argument and it should hold exactly.

**Symmetry.** Sustained tone, spectrum analysis, second-to-third harmonic ratio.
Sweep asymmetry from -100 through 0 to +100 in eleven steps.

Pass: at asymmetry 0, even harmonics at least 40 dB below odd. Even content rises
monotonically with the magnitude of asymmetry in both directions.

Report the ratio in dB at all eleven points. This tells you whether the warmth
mechanism works, as distinct from the control merely doing something.

## T11. Aliasing (P1)

48 kHz, C6 at 1046.50 Hz, maximum drive, 4 second render, 65536-point Hann FFT of
the steady portion. Measure energy in bins below the fundamental, excluding bins
within 25 cents of a harmonic.

Pass, both required: ADAA on is at least 6 dB below ADAA off, and ADAA on is at or
below -55 dBFS relative to the fundamental.

Report both absolute values. If ADAA on is not measurably better than off, the
path is inactive regardless of whether both clear the absolute threshold.

## T12. Bore and register (P1)

Hold a note and switch `_bore` between cone and cylinder.

Pass:

- Cylinder sounds exactly one octave below cone for the same nominal note, within
  5 cents. Same pitch in both means the factor of two has been applied twice or
  not at all.
- Odd harmonics dominate in cylinder mode: even at least 20 dB below odd, with
  asymmetry at zero.
- The switch produces no discontinuity above 0.25.

Separately confirm buffer sizing. Cone requires `fs/f0` and cylinder requires
`fs/(2·f0)`, so cone is the worst case. Verify at the lowest supported note at
96 kHz in cone mode.

## T13. Voice lifecycle under the existing allocator (P0, blocking)

The allocator, release behaviour and steal fade are unchanged for v1. This suite
verifies that PIPE behaves acceptably under them, not that they were modified.

1. **Truncated tails are expected.** With nominal `_rt` at 20 s, confirm the note
   is cut by the shared output fader rather than ringing naturally. This is a
   documented v1 limitation, not a defect. The finding is whether it appears in
   the release notes. Report the audible character of the truncation, since a
   fader that cuts a full-amplitude sustained tone may click even though it does
   not for a decaying one.
2. **Stealing sustained voices.** Force stealing with 24 notes on an 8-voice
   configuration, all slots on PIPE with long nominal decay. The existing fade is
   exponential rather than fixed-duration and has never been exercised against a
   voice at full amplitude. Assert no discontinuity above 0.25 at any steal point.
   If this exposes an audible defect, report exactly what it sounds like and at
   what settings. A change to the fade is authorized only by this finding.
3. **No leaks.** 500 notes over five minutes at long nominal decay. Active voice
   count returns to zero.
4. **Mixed voices.** A patch with one slot on PIPE and another on a plucked model.
   Confirm release behaves consistently for both and that the PIPE slot does not
   hold the voice open.

## T14. Realtime safety (P0, blocking)

Extend the existing allocation interception harness.

Scenario: 60 seconds of continuous playing, model switched to and from PIPE while
notes sound, pressure and filter controls automated continuously, bore switched,
quality changed mid-playback, PIPE page open with all displays live.

Pass: zero intercepted allocations or frees on the audio thread, zero lock
acquisitions.

This covers your display code as well as Astra's DSP. State the probe's scope
honestly: C++ `new` and `delete` only, or also raw `malloc` in third-party code.

## T15. Audibility (P1)

Loudness-matched per-band comparison across 24 gammatone bands, for each of:

- Loop filter key tracking on versus off, across three octaves.
- Exciter velocity tracking on versus off, at three velocities.
- Saturator asymmetry at 0 versus 60.
- DC/Noise at 10 versus 90.

Pass: at least 8 bands differ by 1.5 dB or more, for each comparison.

Also report, without a verdict: the per-band difference between a PIPE patch and
the nearest equivalent patch on an existing model. If that difference is small,
the feature has not delivered its premise and the human needs to know regardless
of how many suites passed.

## T16. CPU (P1)

PIPE voices sustain while held, so decay-based culling will not free them early.
This is a different load profile from the existing models.

Measure percent of real time at 4, 8 and 16 voices, one slot on PIPE and all
three, at Eco, Normal and High, 48 kHz, block 256.

Fail if 8 voices with all three slots on PIPE at Normal exceeds 80 percent of real
time. Report every figure in the format used by `docs/PERFORMANCE.md`.

Recommend a default polyphony for PIPE presets. The recommendation goes in the
report, not into the code.

## T17. Host, state and parameters (P0, blocking)

- `AeriformHostCheck`: PASSED.
- pluginval strictness 10, `--validate-in-process`: SUCCESS.
- State round trip: save, reload, compare every parameter exactly.
- Migration: load a session saved at the current released state version. No slot
  on index 9, every new parameter at its documented default, every pre-existing
  parameter unchanged.
- Parameter count. The baseline is 785, not the 509 quoted in earlier drafts.
  Fifty-seven new parameters are proposed, giving 842. Confirm the actual count
  and attach a diff of the generated table against the previous version.
- Confirm no existing continuous parameter changed range, default or meaning. The
  append-only exception applies to model choice parameters only.

**Automation list inspection.** Load the current build in a real DAW and inspect
the automation list at the 785 baseline, before the feature lands. Report whether
finding a specific parameter is practical. Parameter hiding is a prerequisite for
this release rather than a co-requisite, and this inspection is the evidence for
or against that judgment. If the list is already unworkable at 785, say so plainly
in the findings.

## T18. Interaction and empirical stability (P1)

The contraction argument in the DSP spec covers an isolated loop with fixed
coefficients. It does not cover time-varying coefficients, delay length
modulation, or the coupled network. Those are empirical and this suite is where
they are exercised.

1. **Time-varying coefficients.** Sweep both loop filters continuously and rapidly
   across their full range while notes sound, at maximum drive and long nominal
   decay. A one-pole whose coefficient is changing is not guaranteed passive
   during the change. Assert bounded output and no NaN.
2. **Delay modulation.** Continuous pitch bend and MPE glide on a sustained PIPE
   voice. Assert no discontinuity and that phase compensation tracks the bend.
3. **Coupled network.** All three slots on PIPE, all six cross routes at maximum,
   energy loop at maximum, 16 voices, five minutes. Cross routes add gain outside
   the contraction argument. Assert bounded output, no NaN, return to silence
   within two seconds of release.
4. **Mixed coupling.** One slot on PIPE, two on existing models, cross-coupled.
   This is the configuration the feature exists for. Confirm stability and that it
   is audibly distinct from either alone.
5. **Economy mode**, which stops the second network. Confirm consistent behaviour
   and that any restriction on PIPE is documented rather than silent.
6. **Deep morph** between a PIPE snapshot and a non-PIPE one with notes held. No
   discontinuity, no unbounded output.
7. **Repipe** with PIPE active on all three slots and notes held. There is a prior
   unreproduced report of pitch resetting under Repipe, and a model that computes
   effective delay length from phase compensation is where it would become real.

Report governor and state guard activation counts per configuration.

## T19. Provisional control sweeps (P1, evidence only)

No pass or fail. This suite decides which controls ship.

Most of the fifty-seven new parameters are provisional pending P1. Rather than
tuning the prototype to one good sound and inheriting the parameter set wholesale,
each provisional control is swept individually and its effect measured.

**Method.** Astra produces renders sweeping each provisional control across its
range with everything else fixed, at three reference patches. You loudness-match
and compute per-band differences across 24 gammatone bands between the extremes of
each sweep.

**Report** a table: control, bands differing by 1.5 dB or more, maximum per-band
difference, and whether the effect is monotonic across the sweep.

**The specific question to answer first.** The filter morph endpoints
(`_lp_0`, `_lp_1`, `_hp_0`, `_hp_1`) plus the morph position are five parameters
where two plain cutoffs would do, twelve parameters across three slots. Compare a
morph sweep against a plain cutoff sweep reaching the same endpoints. If they are
not meaningfully different, the morph does not earn its extra three parameters and
cutting it takes the count from 57 new to 45, and the total from 842 to 830.

That is still a wall. The parameter problem is not solvable by trimming this
feature, and the report should say so rather than presenting the cut as a
solution.

## Report

One file, `artifacts/windows-x64-pipe/REPORT.md`.

```
Phase under test:
Commit hash:
Toolchain:
Date:
Suites run:
Suites blocked (with reason):
Astra-reported numbers re-derived, with any divergence:
Evidence-only suites (T8, T19): measurements presented, no verdict claimed
UI items self-assessed (listed separately, flagged as self-assessed):
```

One section per suite: verdict where one applies, measured values including for
passes, exact configuration and smallest reproduction for failures, log path.

Close with a prioritised findings list, P0 first, then P1, P2, then observations
that are not defects. Keep self-assessed UI items visually separate from
independent DSP verdicts, and keep evidence-only results separate from both, so
the human can weight the three differently.

## Reporting conventions

Report check counts if your harness produces them, but do not lead with them. A
suite reporting millions of checks is usually one assertion inside a sample loop.

The figures that matter here are the measured ones: T60 in isolation and in the
full engine, cents deviation across filter settings, the highpass maximum
magnitude, the second-to-third harmonic ratio, the alias floor, the band counts in
T15 and T19, the CPU percentages, and the automation list judgment in T17.

If a suite is ambiguous, unimplementable as written, or measures the wrong thing,
say so and propose the correction. Do not silently substitute your own version.
Nine errors were found in the previous specifications by exactly that behaviour.
