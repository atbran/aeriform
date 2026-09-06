# EXP_Aeriform v3 EXP — feature testing build

Version 3.0.0. Instrument VST3 and standalone synth for Windows x64. The separate normal-input FX product is outside this package.

## What changed

- Contact/collision: an oversampled nonlinear pickup stop now makes the contact response directly audible. The original conservative physical force still couples the two resonators. Friction adds corrugation, damping changes the stop reflection, and the audible gap has finer control of quiet signals. Activity and inactive-route guidance are shown on the Contact page.
- Economy mode disables the Physical stereo controls in the interface. Select Physical stereo to use them.
- SPACE > SPECTRAL FREEZE: Capture, Release, Hold, Blur, Shift, Random phase, Decay and Mix, with a display of the captured spectrum. Capture once after playing; Capture again replaces it. Release returns to live audio. Zero Decay holds indefinitely. Audio captures are temporary, not saved samples. Capture/Release host parameters retain their documented trigger-on-each-change behavior; one GUI click sends one change.
- SPACE > SATURATION: low/mid/high bands with adjustable LR4 crossovers, per-band drive, character, mix and output, global mix and 1x/2x/4x oversampling. Soft, Warm, Clip and Fold are independent per band. Zero drive is undistorted. Start with 12–18 dB drive and adjust Output to balance levels.

All 492 existing parameter positions/IDs remain intact; 17 saturation parameters are appended (509 total). New saturation defaults disabled. EXP_Aeriform retains its experimental plugin identity; no global plugin installation is performed by this package.

## Run / test

Run EXP_Aeriform.exe, or use the complete EXP_Aeriform.vst3 directory in your VST3 test location. Preserve its Contents subfolders. Close the host before replacing an older experimental build, then rescan if needed. It is the same experimental plugin identity, so install one EXP_Aeriform build at a time.

For contact, select Parallel (or Serial/Hybrid), enable both source and destination resonators, and play a note. Start with Gap 0.05, Amount 0.3–0.7; raise Friction for buzz and adjust Damping. The page explains a stopped route or a signal below the gap. Single mode does not run B/C unless Repipe activates them.

Saturation's neutral active signal has the crossovers' allpass phase and oversampling phase; its magnitude response is flat. Wet/dry mixes are aligned to this neutral signal. Disable the module for sample-exact raw bypass after the fade. Quality changes briefly fade to raw bypass while switching the oversampling filters. When crossover controls overlap, the lower split stays at least a 1.25 ratio below the upper split.

## Verification scope

This is a feature-complete experimental handoff for the user's separate testing agent. Compilation and focused smoke checks are recorded in CHECKS.md. Full regression, performance, aliasing, stress testing and human listening acceptance of the new features are still pending. Earlier bank/room validation applies to that earlier source, not automatically to this build. Contact's instantaneous pickup map is contractive before filtering; no complete-network mechanical passivity claim is made.

See V3_TESTING_HANDOFF.md for changed-path test instructions. SHA256SUMS.txt identifies the shipped binaries and documents. SOURCE.md identifies their exact source tree and publication lineage.
