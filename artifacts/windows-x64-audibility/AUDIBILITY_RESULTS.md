# Sympathetic Bank and Coupled Room audibility — 2026-09-06

This is the first implemented audibility revision of EXP_Aeriform. It follows AUDIBILITY_IMPLEMENTATION_PLAN.md. It does not complete the larger synth feature list. Human listening sign-off remains pending; the numbers below are measurements, not a claim that somebody listened.

## What changed

- The bank receives stereo excitation. A calibrated sustained component and source-derived onset pulses excite its modes independently of the requested decay. There is no added oscillator or autonomous noise source, and no input attenuation based on active voice count. Distinct and coincident modes have separate normalization treatment. A smooth total-state budget and radial backstop bound its energy. Damping now has a substantial effect at lower pitches, as well as higher modes. Bypass clears hidden sound after fading out.
- The room has an audible layer of early reflections plus a recalibrated late return. Its output layer stays outside physical feedback. Size, shape, diffusion, wall damping and air absorption materially change level-matched audio. Shimmer keeps the explicit legacy room configuration.
- Room feedback is much stronger. A pre-resonator budget limits each voice's added injection to energy funded by its actual exciters. The return cannot fund itself through the resonator/energy-loop audio path. The minimum return delay remains 32 samples. Output level remains independent of network return.
- Both NETWORK module pages have AUDITION RETURN buttons and actual input/output meters. Audition is temporary monitoring state, with no preset or automation parameter added. It retains the normal graph and uses a 20 ms fade; preset/session load, reset/prepare and editor close end audition.

No extra pre-resonator audio feed was needed for the bank on these probes. All existing IDs, parameter ranges, experimental identity and original factory definitions remain unchanged. Enabled EXP patches intentionally sound different.

## Fixed musical probes

48 kHz, 256-sample blocks; factory Init, Plucked Tube and Metallic Steam; MIDI C4 velocity 102; unrelated global effects and the master limiter disabled. The isolated-return test allows 0.5 seconds to settle, then measures three seconds including a note release after about 1.5 seconds. It uses the actual monitor path and writes stereo WAVs to build/audibility/returns.

| Patch | Bank return / dry RMS | Room return / dry RMS |
|---|---:|---:|
| Init | -13.94 dB | -16.62 dB |
| Plucked Tube | -8.99 dB | -16.92 dB |
| Metallic Steam | -18.96 dB | -17.37 dB |

These are default enabled-module levels, with the room's network return set to zero for isolation. The initial -18 to -6 dB calibration target is broadly met; the noisy Metallic Steam bank result is about 1 dB below its lower edge. This is not a universal loudness specification. The upper Return/Output level ranges permit stronger contributions.

Before-change full-output difference measurements at d40798c were -52.20/-45.38/-71.18 dB for the bank, and -33.40/-32.55/-33.16 dB for the room, in Init/Plucked Tube/Metallic Steam order. Those earlier renders use the saved four-second harness configs; do not directly equate their analysis window to the three-second isolated-return test above. The exact same-config comparison lives in build/audibility/after/comparison.json once final validation is run.

With audible room output zero, Network return at 0.5 produced differences of -22.12, -2.75 and -24.92 dB relative to the uncoupled output. After RMS matching, these remained -22.18, -3.86 and -25.06 dB: the effect is not explained by volume alone. Maximum feedback on Plucked Tube is much more aggressive (about +13 dB output RMS change); 0.2–0.5 gives a more moderate starting range. Return strength depends on the source and resonator tuning.

## Verification

Nine focused audibility tests passed, 8,185,272 checks:

- Real stereo return levels on the three named patches.
- Exact upstream tap equality with audition toggled, including Deep morph; zero intercepted C++ allocations/frees during those warmed processing blocks.
- Antiphase input, 1/3/6/12 coincident bank modes, 44.1/48/96 kHz, bookkeeping changes, transient response at short/long decay, and bounded state with no radial backstop activation.
- Hostile room return continuing seven seconds after fresh excitation stops; emitted energy stays within credited energy and eventually becomes silent.
- Output level independence from room state/physical return; audible return-only host comparisons.
- Limiter-disabled full-instrument stress involving Repipe, Energy Loop, stereo, contact and both modules at three sample rates.
- Level-matched bank brightness/damping/damper/tuning/count and room size/shape/diffusion/wall/air comparisons. The initial damping test failed (only 0.050 normalized residual); the DSP was corrected, giving 0.453. The acceptance threshold was retained.
- Mono and oversized host blocks, Deep morph, preset/session/editor lifecycle, and hidden-energy clearing on bank bypass.
- CPU measurements at all three quality modes and 1/8/16 voices. Results vary with machine load; they are offline render percentages of real time, not Ableton's CPU meter. Both modules add measurable cost. Deep morph runs two engines. The complete MPE/controller allocation-and-lock audit remains unfinished.

Final verification passed: **139 tests, 17,142,923 checks, zero failures**; all **40 original preset renders have bit-identical float sample data** against the SHA-verified frozen baseline; VST3 host check PASSED at 44.1/48/96 kHz and 32/256/1024 samples; pluginval strictness 10 SUCCESS. Logs and exact-sample comparisons are retained in build/audibility and copied with the test package. Both module pages were rendered and visually inspected; their audition controls fit without overlap. The independent staged validation work is preserved separately; none of its fixtures or findings were changed to pass these tests.

## Listening and continuation

Start with one note and one main resonator. Enable the desired module, click AUDITION RETURN to hear its actual contribution, and click again for the full synth. Bank tuning modes respond to transients/broadband material; an unrelated steady sine still cannot supply arbitrary new scale pitches through a purely sustained linear path. Hold/Freeze retains already stored sound; it does not create energy in an empty module.

Listen to the labelled dry/module and room-coupling examples before accepting the final voicing. Expand the long-duration combined-feature and automation matrix as part of the remaining all-feature audit. Dedicated spectral-freeze UI, multiband saturation, the normal-input FX target and other original work remain outstanding; the separate upstream effect-version branch should be inspected before duplicating its FX implementation.

## Latest CPU sample

Init patch, 48 kHz / 256 samples; percent of real time, one-second steady renders after warm-up. This measures the test process on this machine, not Ableton.

| Voices | Quality | Modules off | Bank + room |
|---:|---|---:|---:|
| 1 | Eco | 1.76% | 5.39% |
| 1 | Normal | 3.52% | 6.37% |
| 1 | High | 3.44% | 5.24% |
| 8 | Eco | 7.98% | 12.70% |
| 8 | Normal | 13.48% | 15.13% |
| 8 | High | 15.27% | 18.96% |
| 16 | Eco | 15.02% | 21.74% |
| 16 | Normal | 21.00% | 25.92% |
| 16 | High | 28.62% | 34.16% |
