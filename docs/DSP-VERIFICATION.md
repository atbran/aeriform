# Independent DSP verification — September 12, 2026

Verifier: separate AI reviewer/test agent. Implementation reviewed against starting commit `7e6e3bfd20f2f877be1318bef866778a90071f4d` and user-approved overrides. Work remains uncommitted. No production source was modified by this verifier; three legacy test cases were adapted to the accepted rack architecture.

## Build and deliverables

Windows x64, MinGW GCC 14.2, CMake/Ninja, Release. JUCE source supplied by existing local build configuration. Final production Standalone and VST3 linked successfully. Warnings remain in existing and newly compactly formatted source; builds are not warning-clean.

- Standalone: `D:/dev/build/aeriform-dsp-rack/Aeriform_artefacts/Release/Standalone/AERIFORM.exe`
- VST3 bundle: `D:/dev/build/aeriform-dsp-rack/Aeriform_artefacts/Release/VST3/AERIFORM.vst3`
- Test executable: `D:/dev/build/aeriform-dsp-rack/AeriformTests.exe`
- Build logs: `D:/dev/build/aeriform-dsp-rack/build-final.log`, `build-deliverables.log`, `build-test-final.log`.

## Results

Full `--all` run: **164 tests, 27,404,897 checks, three stale-test failures**. All ten new advanced tests passed. The three old host tests enabled removed global optional effects and/or checked obsolete global spectral telemetry. Tests now target independent rack slots and slot telemetry; final targeted rerun recorded below. Full-run log: `D:/dev/build/aeriform-dsp-rack/test-all.log`.

The full run passed existing factory preset loading/audibility and classified level/DC/headroom sweeps, randomized parameter fuzz, state migration, DSP algorithms, morph/undo, and output protections. Existing optional effects no longer using old IDs is explicitly accepted by the user; no legacy-chain migration is promised.

Advanced coverage passed: actual noncommuting rack order; manual chain comparison; independent duplicate settings; stable slot identities; empty/bypassed transparency; macro modulation at audio output; parameter/session round-trip and undo; grouped breath character changes that preserve envelope/resonator; 44.1/48/96 kHz extreme settings with 17/64/256/1024-frame blocks; structural type/order/bypass transitions; deterministic breath seeds, long holds, and finite bounded output. C++ allocation/deallocation probe reported zero in tested rack callbacks. This does not instrument every C allocation, OS lock, host callback, or scheduling behavior.

Independent review found Spectral Capture/Release UI/state inconsistencies. Implementer repaired coordinated action buttons, reset-time event priming, and live session/preset/undo event-parity preservation. New runtime tests passed the independently proposed live-release-parity restoration failure case.

## UI

Minimum-size snapshot test passed **697 checks** at 1280 × 900. Visually inspected `artifacts/advanced-dsp/rack-mixed.png`, `rack-four-type-4.png` (densest combination), and `exciters-breath.png`. All four complete effect panels fit simultaneously without scrolling, clipping, or overlap. Text is readable and existing visual styling is preserved. Permanent Chorus/Delay/Reverb controls are absent from the rack page. Snapshots are JUCE component renders, not screenshots of a DAW window; host framing and operating-system DPI remain user checks.

## Audio and performance

Baseline source was retained from the starting implementation; baseline full synth was separately built from its exported source. Baseline render: **one test, 36 checks, zero failures**. Outputs: `artifacts/dsp-baseline` (18 WAVs); `artifacts/advanced-dsp` (182 WAVs). Current matrix covers five characters × three MIDI notes (45/60/81) × three velocities, both isolated source and initialized synth, raw and matched versions. Medium synth velocity is MIDI 76; sample rate is 48 kHz; held 1.5 seconds followed by 0.5 seconds release.

Independent `audio-analysis.csv` contains peak/RMS/DC and power-spectrum centroid for 200 WAVs; `breath-contours.png` plots 10 ms source RMS. Matched versions target RMS 0.1 with peak capped at 0.95; this is gain-only normalization. Soft Exhale medium C4 initialized synth: peak 0.154959, RMS 0.0374475. Flute Air: peak 0.166528, RMS 0.0335341. Neither requires an extreme gain boost in these technical renders.

`rack-timing.csv` contains maximum observed call durations and block budgets. Timing ran concurrently with compilation and is not an isolated benchmark: worst captured call was 19.3124 ms for four Spectral Freezes at 48 kHz/17 frames (0.354167 ms block budget). Do not present these measurements as guaranteed deadline compliance. Combinations remain unrestricted as requested. No universal realtime-safety or subjective sound-quality claim is made.

## Remaining limits

Subjective human-exhalation/flute-air quality and final preset-bank tuning await user listening. Representative comparisons: `Soft-Exhale-60-60-synth-matched.wav`, `Flute-Air-60-60-synth-matched.wav`, and baseline `baseline-synth-60-76-matched.wav`. DAW/VST3 host loading was not executed in this run. Full all-character short/repeated-note perceptual comparisons and exhaustive parameter-interaction proofs were not performed. Technical tests cannot establish realism.

## Final targeted rerun

Final test relink succeeded. Corrected Resonant Delay host test: 5 checks, zero failures; Shimmer: 8 checks, zero failures; Spectral/Saturation: 15 checks, zero failures. Final UI capture: 697 checks, zero failures. Logs: `D:/dev/build/aeriform-dsp-rack/targeted-*.log`. All three original full-run failures are resolved by adapting obsolete tests; no production edits were required. Full suite was not repeated. Final Main snapshot includes Air/Mouth/Turbulence compact controls and was visually inspected.

