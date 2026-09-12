# UI overhaul

The editor keeps its 1180 x 820 logical size and existing scale controls.

| Workspace | Contents |
| --- | --- |
| Main | Exciters, breath envelope, Res A/B/C tabs, three LFOs, modulation envelope, network essentials, master, shared fixed effects, macro placeholder |
| Exciters | Full exciter, interaction, pre-shaper, articulation and wavefolder controls |
| Network | Resonators and routing; Advanced / Physical Stereo sub-tab |
| Mod Matrix | All 16 existing routing slots plus the full modulation controls |
| Effects / Effects | Fixed Chorus/Delay/Reverb, existing filters, resonant delay, shimmer, spectral freeze and saturation in one scrolling surface |
| Effects / Acoustic | Contact, Sympathetic Bank and Coupled Room side by side |
| Advanced | Existing snapshot, morph and randomization tools |

## Main-page details

- Pipe, Scope and Spectrum share the previous visualizer footprint.
- Res A/B/C use the same control arrangement. B/C expose their existing Ratio parameter in the position of A's Length control; no new audio parameter is introduced.
- Body/formant controls remain in Network and are absent from Main.
- Main and the dedicated effects view instantiate the same SpacePanel component, attached to the same existing parameters. Compact effect tabs affect visibility only.
- Chorus, Delay and Reverb remain fixed parts of the chain. No wet amount, default or preset is changed.
- Macros is a labeled placeholder; no macro engine is implemented.

## Interaction and preservation

The MIDI Control panel and separate signal-flow legend are removed. The interactive A/B/C resonator network diagrams remain on Main and Network, including routing connections, route adjustment gestures and double-click node toggles. MIDI Learn, per-control mapping removal, and Clear all MIDI mappings remain in the knob context menu. Normal MIDI performance and modulation behavior are preserved.

Parameter IDs, ranges, defaults, audio state version and factory patches are unchanged. A UI layout version migrates older saved page locations to the new Effects grouping. Numbers appear while editing rather than when automation or preset loading updates an idle knob. Existing tooltips carry control descriptions; paragraph-based explanations have been removed from the reorganized pages. In the scrolling Effects surface, the wheel scrolls; Shift+wheel edits a knob.

## Visualization

The spectrum uses a 2048-sample Hann-windowed FFT, logarithmic frequency spacing, and an -84 to 0 dB display range. Full-rate samples are copied through an atomic ring buffer after the final processor output, including Deep Morph. FFT work runs on the GUI thread only for the visible Spectrum view. The scope and pipe waveform also observe final output. These changes affect telemetry, not audio samples.

Output meter response uses an 8 ms attack and 300 ms release, with a 700 ms peak hold and an 18 dB/s peak fall. Resonator energy bars use a 10 ms attack and 300 ms release. Timing is based on elapsed time rather than a fixed decay per repaint.

The palette uses charcoal/slate surfaces, blue primary accents, warm resonator A controls, and restrained sea-glass and violet secondary accents. Repeated interactions remain immediate.

## Deferred

Per-resonator dry/wet, new Coupled Room processing, an interchangeable effects rack and the macro engine remain unimplemented.

## Validation

- Standalone and VST3 Release targets built successfully with GCC 14.2 / JUCE 7.0.12.
- Overhaul baseline complete test suite: 146 tests, 17,156,293 checks, zero failures.
- After restoring the network diagrams: all 8 editor tests passed (3,257 checks), including diagram visibility, bounds and double-click resonator toggles in both views. Standalone and VST3 were rebuilt and the synth VST3 host check passed again.
- Includes control binding and layout bounds, all 16 modulation slots, Main A/B/C selection, full-rate final-output telemetry, Deep Morph, bypass silence, and legacy UI location migration.
- The synth VST3 host check passed: scanning, repeated instantiation, MIDI audio at 44.1/48/96 kHz and 32/256/1024-sample blocks, and state restoration.
- Reviewed rendered Main, Network, Acoustic, Effects, Exciters and Mod Matrix pages. Images are in `artifacts/ui-overhaul`.
- No parameter-definition or factory-preset edits.

The generated Ninja VST3 post-build command uses the account's short path (`THENER~1`) to avoid JUCE's nested Windows command quoting problem with the caret in the long workspace path. This adjustment is local to the build output; a fresh build in a path without a caret does not need it.
