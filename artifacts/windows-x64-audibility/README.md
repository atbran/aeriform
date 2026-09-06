# EXP_Aeriform — audible bank and room test build

Source commit: `55aa00926e216dd8ae60b1942479d8cc19bb7766` on `codex/experimental-aeriform`.

Run EXP_Aeriform.exe for standalone testing. For Ableton, use the entire EXP_Aeriform.vst3 folder, retaining Contents and Resources. The package does not install anything automatically. It has the same identity as previous EXP_Aeriform builds; the original AERIFORM retains its separate identity. Plugin version remains 2.1.0, with 492 parameters and 42 factory presets.

The Sympathetic Bank now responds strongly to attacks and has more useful damping. Coupled Room has stronger reflections and source-funded physical feedback. Each page has AUDITION RETURN: click to hear the actual return, click again for the full synth. Preset/session load or editor close ends audition. One note and one main resonator are sufficient. Hold/Freeze needs previously excited energy.

Try default sends/returns first. For room physical feedback, 0.2–0.5 is a useful starting range; maximum Network return can heavily extend ringing on plucked patches. Enabled experimental presets intentionally sound different.

Validated: 139 tests, 17,142,923 checks, zero failures; 40 original preset renders have bit-identical samples against the verified baseline; VST3 host checker PASSED; pluginval strictness 10 SUCCESS. Full logs are in validation/. AUDIBILITY_RESULTS.md records measurements, CPU results and limitations. audio/ contains raw and RMS-matched listening comparisons. Human listening approval is still pending.

This is a local testing checkpoint. The separate validation task's no-push history has not been published. The previous published binaries remain in artifacts/windows-x64.

Remaining larger project work includes dedicated spectral-freeze UI, multiband saturation, normal-input FX target and the whole-plugin realtime/performance audit. The MPE/controller path is not claimed allocation-free or lock-free.
