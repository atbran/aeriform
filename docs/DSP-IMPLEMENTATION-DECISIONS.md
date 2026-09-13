# Rack and breath implementation decisions

User-confirmed scope, September 12, 2026:

- Four serial slots with independent instances; unrestricted duplicate combinations. CPU cost alone must not restrict choices.
- Existing eligible algorithms: Resonant Delay, Shimmer, Spectral Freeze, Multiband Saturation. Acoustic processors retain their existing graph positions.
- Rack follows permanent Chorus/Delay/Reverb and precedes post-effects filters and output protection.
- Full controls for all four slots visible together without scrolling. Preserve UI continuity, establish minimum size, and verify screenshots. If a usable layout cannot fit, reconvene with the user.
- Permanent Chorus/Delay/Reverb controls live on Main only. Filters remain accessible separately.
- Slot identity is independent of order. Keep per-type parameter settings when straightforward; this is low priority.
- Preserve state on reorder. Prefer natural bypass tails; controlled short fades are an accepted fallback for implementation complexity.
- Existing patches may change. No legacy optional-effect chain migration required for the small test-patch collection.
- Breath characters: Soft Exhale (initial default), Focused Jet, Whisper, Rough Air, Flute Air. Character selection changes only breath-engine controls and is one undo transaction.
- Technical checks and screenshots required. Subjective breath acceptance awaits user listening.

## Implementation

Baseline: `7e6e3bfd20f2f877be1318bef866778a90071f4d`. Work is left in the user-selected checkout for review; no plugin installation or publishing is performed.

The fixed processing order is pre-effects global filters, Chorus, permanent Delay, permanent Reverb, four-slot rack, post-effects global filters, then the existing master/output protection. Contact, sympathetic resonance, and room/acoustic processing retain their original graph positions. They are not post-sum rack algorithms.

Each rack slot owns four prepared algorithm objects and retains a separate parameter bank for each type. Only its selected algorithm processes audio. Empty is transparent. Repeated algorithms are unrestricted, including four Shimmers or four Spectral Freezes. There is no CPU admission policy. Preparation allocates delay/algorithm storage outside the callback; processing uses fixed 32-frame scratch arrays. The cost is more persistent memory than allocating only the selected type.

A type change fades that slot to dry over approximately 20 ms, resets the incoming algorithm's runtime state, and fades it in. Bypass also fades to dry and clears runtime state once silent. Stored controls survive these operations; old audio tails do not. Reorder fades the whole rack through dry and moves the processing order while retaining instance objects and their runtime state. Switching occurs at an internal chunk boundary. No additional dry-path latency compensation is introduced; effect-specific delays remain part of their sound. Spectral Freeze retains its 2048-sample analysis window and 512-sample hop.

## Host parameters and state

| Parameter family | Meaning and compatibility |
| --- | --- |
| `rack_order` | Integer 0�23: lexicographic permutations of A, B, C, D; 0 means ABCD. |
| `rack1_type` � `rack4_type` | Empty, Resonant Delay, Shimmer, Spectral Freeze, Multiband Saturation. |
| `rack1_enabled` � `rack4_enabled` | Independent bypass; defaults enabled, with Empty selected. |
| `rackN_rd_*`, `rackN_sh_*`, `rackN_sf_*`, `rackN_sat_*` | 46 controls per persistent slot. Ranges and choice semantics mirror the original algorithms. |
| `breath_mouth`, `breath_edge`, `breath_contour` | Continuous 0�1 breath shaping. |
| `breath_swell`, `breath_settle` | 5�500 ms onset and 20�2000 ms settling. |
| Original optional-effect parameters | IDs remain for host/state compatibility, but no longer operate the old optional chain. Old optional settings are not migrated, as requested. |
| Existing breath parameters | Original IDs retained; Noise is labeled Air. Default turbulence/attack/release amounts become 0.12/0.02/0.025. |

The layout has 762 parameters: 198 appended to the previous 564. Existing parameter indices are preserved. There are 149 appended continuous modulation destinations (144 rack controls plus five breath controls). Rack modulation is additive over each physical range and clamped to that range. Inactive banks remain stored and do not affect the current algorithm.

Slot identity stays fixed when its panel moves, so automation continues addressing the same instance. Capture/Release are event buttons that also coordinate Hold; Capture enables the slot. Their edge parity is administrative state: undo, A/B, preset loading, session restore, and randomization preserve the current command parity instead of replaying stale commands. Restored Hold can initiate a fresh capture; the frozen audio buffer itself is not serialized.

`scripts/gen_params.py` is the source of generated parameter tables and invokes `scripts/advanced_bindings.py` to regenerate the rack/modulation maps. `Source/Params/ParamTable.inc` records exact host ranges/defaults; `RackParameters.h` records slot-bank mapping.

## Breath implementation

Soft Exhale, Focused Jet, Whisper, Rough Air, and Flute Air are explicit sets of 11 visible breath controls, defined in `Source/Params/BreathCharacters.h`. Selecting one is one undo transaction and leaves the main envelope, resonator, and other engine settings alone. Editing afterward displays a modified character. The existing shared breath architecture remains shared between Breath exciter slots.

White/pink noise feeds broad low-Q mouth and upper-air bands. Mouth moves the broad spectral emphasis; Flute Air emphasizes the additional upper air band. These are sound-design approximations, not a physical mouth or flute simulation. Turbulence adds bounded correlated amplitude variation (gain clamped to 0.1�2). Pressure adds mouth opening and roughness while existing LP/HP pressure brightness remains delegated to the exciter pre-shaper, avoiding duplicate LP/HP application.

The breath source has its own exponential onset and settling emphasis, separate from the main amplitude envelope. Air gain, Mouth, Edge, and contour amount smooth over 20 ms. Filter coefficients refresh every 16 source samples; timing coefficients derive from the sample rate. Existing deterministic seeds are retained: noise uses the supplied seed, with turbulence/drift streams derived by fixed multipliers. Reset clears filter and contour state; it does not reseed a running stream. Equal seeds and event sequences support reproducible comparisons; different sample rates need not produce identical samples.

Air retains the existing 0.5 source gain factor with velocity and per-note variation. Broad mouth/edge gains are 1.8 and 1.1 respectively; this is not a loudness-normalized source. Existing high-pass filtering and final output protection remain in place. Listening-file normalization targets RMS 0.1 with a 0.95 peak ceiling, using the lower gain when the peak ceiling prevents exact RMS matching. Raw files remain available for assessing actual level. Measurements and subjective approval must not be conflated.

## UI

The Effects section presents four complete panels in a 2�2 grid, ordered left-to-right then top-to-bottom. Arrow buttons change the actual processing order. Full selected-type controls are displayed in each panel; there is no rack viewport or scrolling. Minimum editor dimensions are 1280�900 logical pixels; supported scale choices start at 100 percent. Existing styling and shared controls are reused.

Permanent Chorus/Delay/Reverb controls live only on Main. Main's compact Breath controls are Air, Mouth, and Turbulence; the dedicated Exciters page retains Pressure, Reed, and all other existing breath controls alongside the new detailed shaping controls. Exciter panels are taller to fit their complete controls.

## Verification handoff

At the user's request, a separate AI owns builds, tests, screenshots, and their report in `DSP-VERIFICATION.md`. The user owns final listening and hands-on acceptance. Do not treat implementation notes as passing test results. The verifier will record failures and unverified items explicitly. No preset bank is authored; the user will create it after the main features are settled.

Build environment: Windows, MinGW GCC/Ninja, JUCE from `D:/dev/build/gpt-aeriform-test/external/JUCE`. The JUCE checkout avoids a Windows linker path issue involving the caret in the user profile name.

```powershell
$env:PATH = 'D:\dev\tools\mingw64\bin;' + $env:PATH
cmake -S '<repository>' -B D:/dev/build/aeriform-dsp-rack -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DFETCHCONTENT_SOURCE_DIR_JUCE=D:/dev/build/gpt-aeriform-test/external/JUCE
cmake --build D:/dev/build/aeriform-dsp-rack --parallel 6
$env:AERIFORM_DSP_ARTIFACTS = '<repository>/artifacts/advanced-dsp'
& D:/dev/build/aeriform-dsp-rack/AeriformTests.exe --filter=advanced_
```

The original source is retained in `artifacts/dsp-baseline` and the isolated baseline helper in `Tests/BaselineBreath.h`. A separate baseline checkout/build under `D:/dev/build/aeriform-dsp-before-source` and `D:/dev/build/aeriform-dsp-before` supports full-synth before renders. Test instrumentation and timing coverage are limited to the cases run; they do not establish universal real-time performance or convincing human breath sound.

