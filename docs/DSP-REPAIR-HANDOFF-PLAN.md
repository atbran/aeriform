# AERIFORM DSP repair plan — external agent handoff

Date: 2026-09-12
Status: Planning only. This document authorizes no implementation by its author. The user will give another agent the first repair attempt; if that attempt fails, the user intends to return to the original agent after usage resets.

## Workspace and reproduction build

Repository: `C:\Users\The Nerd^2\.ao\data\worktrees\aeriform-orch\orchestrator\aeriform-orc-orchestrator`

Reported build: VST3 from `D:\dev\build\aeriform-dsp-rack\Aeriform_artefacts\Release\VST3\AERIFORM.vst3`.

The previous implementation remains uncommitted. Preserve existing work and inspect the working-tree diff before editing. Baseline before that implementation: `7e6e3bfd20f2f877be1318bef866778a90071f4d`. Do not reset the checkout to the baseline.

Related documents: `DSP-IMPLEMENTATION-DECISIONS.md`, `DSP-VERIFICATION.md`, `DSP-USER-TEST-CHECKLIST.md`, and `FEATURE-REQUESTS-ADVANCED-DSP.md`. The decisions in this plan and the user's subsequent instructions take precedence over older acceptance assumptions.

## User-reported failures

1. Resonant Delay was tested by itself in VST3. It was generally inaudible; increasing dry/wet caused the output to become inaudible.
2. Shimmer was tested separately and behaved similarly: little or no identifiable effect, with volume dropping as wet mix increased.
3. FM, Sync, and Min/Max interactions were inaudible or barely changed the sound. **Both exciter sources were sine waves.** Do not explain this report away using Breath or an inactive second source.
4. The exposed Breath source, with no resonators, still sounded like vanilla white noise with little filtering. The user explicitly wants stronger vowel coloration and suggested resonant bandpass shaping.

Exact DAW/version, loaded patch, parameter values, note durations, and loaded binary identity have not been captured. Establish these if necessary for exact reproduction; first use the available code and a controlled initialized setup rather than making the user rediscover local facts.

## Confirmed code findings versus hypotheses

A read-only review established the following facts. These are leads, not a confirmed reproduction of the reported VST3 failures:

- Interaction processing passes through the active source when either source is Off. This does not explain the user's two-sine test.
- FM/PM strength is multiplied by B→A, which defaults to zero. Wave/Complex sources consume FM/PM/Sync; Breath does not. Sync needs a source that emits wrap pulses.
- Min/Max at its default Interaction value of 0.5 is mathematically the same as averaging the sources: `(min(a,b) + max(a,b)) / 2 = (a+b) / 2`.
- Resonant Delay defaults include 375 ms delay, 0.5 feedback, and 0.3 mix. Its wet branch contains delayed audio, so full wet legitimately removes the immediate dry signal. Fresh delay input is multiplied by `1-feedback`; at feedback 0.98 this is only 2%. Feedback filtering/coloration can further reduce return energy.
- Shimmer uses the older unvoiced CoupledRoom core via `prepare(sr,false)`. Pitch shifting occurs in the feedback return; the initial wet output is unshifted room output. Feedback-related attenuation affects fresh excitation in the stages.
- The previous Breath changes add broad filtering, onset/settling, and turbulence to white/pink noise. Their existence does not prove sufficient perceptual change.

The previous tests checked wet/dry difference but did not establish sufficient wet-signal audibility. Attenuating output toward silence can pass that assertion. Passing those tests is not evidence against the user's report.

## Agreed repair scope

### A. Resonant Delay and Shimmer

Reproduce the failures with one occupied rack slot at a time, then trace the signal through the wet path. Inspect gain staging, excitation attenuation, feedback behavior, and rack integration before choosing a repair.

Restore clearly audible delay repeats and a distinct pitch-shifted shimmer tail at useful ordinary settings. High wet mix should yield usable processed output after the expected delay/build-up, rather than unexplained silence. High feedback should remain musically useful without simply starving the effect of fresh signal.

Do not blindly remove attenuation or compensate with extreme gain: preserve stability, headroom, and output protection. Any legitimate silence before a delay arrives must be distinguished from a persistently inaudible wet path. Verify effects individually before combined rack cases.

### B. Interaction modes

Use two sine sources as the primary reference. The user explicitly approved changing scaling and defaults so selecting FM, Sync, or Min/Max with active sources produces an immediately noticeable, useful result. Preserve existing controls and layout.

Investigate B→A/depth dependencies, source frequency relationships, Sync triggering, and Min/Max's ineffective midpoint. Choose defaults and ranges with distinct results through ordinary settings, while allowing aggressive results toward the extremes. Make required control dependencies understandable rather than silently leaving the mode ineffective.

Do not rely exclusively on identical, phase-aligned sine inputs: Min/Max cannot distinguish identical signals, and Sync can be inconspicuous at particular ratios. Test both equal and deliberately different pitches/ratios and explain those mathematical limits. Do not invent a promise that every parameter position must alter every possible input.

### C. Breath sound

Keep the same UI. The user prefers stronger vowel coloration over subtle filtering. Use the existing Mouth control for a continuous sweep through rounded/open/bright vowel-like spectral shapes; no separate vowel selector is planned.

Explore multiple resonant bandpass peaks with time variation, a shaped onset, and settling during sustain. Rounded oo-like, open ah-like, and brighter ee-like regions are design targets, not prescribed physical formant frequencies. Tune frequencies, bandwidths, resonance, and movement through listening. Avoid merely increasing level or treble and calling it a new breath character.

Soft Exhale should have clearly audible body and vowel coloration. Flute Air should combine that body with stronger upper air/edge evocative of blowing across a flute mouthpiece. Preserve the five existing character choices and visible editable controls. Character selection must still change only breath-engine controls, remain one undo action, and indicate subsequent modifications.

Judge the exposed source first, then through normal resonator routing. Preserve independent Air amount, roughness, contour, and pressure behavior. Avoid accidental tonal whistles, unstable resonances, or huge level jumps unless deliberately available at extreme settings.

### D. Preserve the accepted product constraints

- Four independently configurable serial rack slots, with unrestricted duplicate effects. Do not prohibit combinations merely to save CPU.
- Preserve per-type control settings and stable slot identities when reordered.
- Full selected-effect controls for all four slots on one page without scrolling.
- Preserve existing UI continuity and the 1280 × 900 minimum editor size.
- Permanent Chorus/Delay/Reverb controls remain on Main only.
- Acoustic processors retain their existing graph roles.
- No requirement to migrate the old optional-effects chain or preserve the previous breath timbre. Preserve successful state loading and unrelated behavior.
- Do not author a new preset bank; the user will do that after the main features are settled.

## Independent verification: Gemini 3.8 Flash

The user specifically chose **Gemini 3.8 Flash** for testing to save usage. Do not substitute Astra or silently claim another model performed the review. The original agent's tool roster did not provide Gemini; arrange an external handoff if needed. Record which model actually runs the verification.

The implementation agent should supply a reviewable patch/commit, build instructions, reproduction settings, and recordings. Gemini should independently evaluate the targeted repair and report pass/fail/unverified for each item. If it cannot listen to audio, explicitly mark perceptual judgments unverified and leave them to the user.

### Required demonstrations

1. Dry versus Resonant Delay: identifiable repeats at ordinary settings, plus high wet/high feedback cases with sufficient duration to hear the delayed output.
2. Dry versus Shimmer: audible wet output and a distinct shifted tail, with enough duration for the feedback pitch-shift path to develop.
3. Two-sine interaction examples: ordinary mix, FM, Sync, and Min/Max with fully recorded source frequencies, routing/depth, and mode settings.
4. Exposed Soft Exhale and Flute Air, plus the same characters through the normal resonator. Compare against the current failed implementation, not only the older pre-overhaul baseline.
5. Short/repeated notes, sustained notes, and releases at representative pitches and velocities. Include ordinary and extreme shaping settings.

Provide raw recordings and gain-matched comparisons. Record peak/RMS and normalization method. Raw levels must establish audibility and headroom; gain matching must not conceal a near-silent wet signal. Define a reasonable signal-level acceptance criterion for the chosen reference settings, rather than testing only `wet != dry`.

### Focused technical checks

- Reproduce the original fault before the repair where possible, and demonstrate the same case afterward.
- Verify actual VST3 host loading and processing, with the final binary path/version identified; internal processor tests alone did not establish the reported host behavior.
- Check finite output, feedback stability, silence/tails, and rapid mix/type/bypass/reorder changes.
- Check representative sample rates and buffer sizes, and report CPU/dropouts without enforcing arbitrary combination restrictions.
- Confirm duplicate independence, automation/modulation, save/reload, undo/redo, and A/B for changed behavior.
- Check Spectral Capture/Release and permanent FX have not regressed if shared routing/state code changes.
- Capture and inspect screenshots after any UI change. Include all four dense effect panels at minimum size and actual host/DPI behavior if available.
- Run relevant existing regressions. Do not present the earlier suite as sufficient perceptual verification or expand into unrelated features.

## Delivery and acceptance

Deliver the repaired source, exact standalone/VST3 paths, a short explanation of each root cause and repair, reproducible settings, representative comparison WAVs, and Gemini's verification report. Clearly separate measured results from subjective judgments and untested areas.

Final listening approval belongs to the user: effects must actually be audible, interactions meaningfully distinct, and the exposed breath noticeably more colored and breath-like than the current white-noise impression.

If the next agent fails, preserve its changes, logs, reproduction details, and remaining failures for the original agent's later attempt. No automatic retry or scheduled work has been arranged.
