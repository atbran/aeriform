# Focused implementation checks

This build passed compilation and the following short checks. Broad acceptance is delegated to the user's testing agent.

- Release build: AeriformTests, Aeriform_VST3 and Aeriform_Standalone completed successfully.
- `AeriformTests --filter=collision_`: 7 tests, 496792 checks, zero failures.
- `AeriformTests --filter=v3_features_`: 1 test, 18 checks, zero failures. Covers effect processing, all saturation qualities, spectral capture/release, new parameter state restore, raw disabled saturation bypass and rendering both new effect pages.
- `AeriformTests --filter=spectral_`: 4 tests, 1339488 checks, zero failures. Existing focused DSP checks, including capture, held pitch, release, blur/shift/decay and intercepted C++ allocation checks for this spectral path.
- Total: 12 focused tests, 1836298 checks, zero failures. This is not the complete regression suite.
- Instrument VST3 host check: PASSED, zero failures (44.1/48/96 kHz, blocks 32/256/1024, notes and state restore).
- Existing 492 parameter IDs and positions unchanged; 17 saturation IDs appended, 509 total.
- Contact, spectral and saturation screenshots were visually checked for page fit and readable controls.

Contact on the existing host fixture now differs from bypass by -0.79 dB RMS relative to the reference. On the matched-resonator mono fixture, RMS-matched differences were -18.93 dB at Amount 0.3, -4.10 dB at Amount 1, and -7.46 dB for the friction variant. The pickup-path alias probe measured 107.91 dB suppression at its selected folded harmonic from 1x to 4x. These are specific numerical fixtures, not universal perceptual or aliasing guarantees. Human listening approval of this contact revision remains pending.

No fresh whole-plugin regression, performance campaign or pluginval certification is claimed for v3 EXP. The next testing pass should prioritize the changed feature paths described in V3_TESTING_HANDOFF.md.
