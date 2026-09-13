# AERIFORM — hands-on testing checklist

The separate AI completed technical rack/breath tests and inspected minimum-size UI renders. The main remaining checks are your listening judgment, actual DAW/VST3 behavior, Windows scaling, and performance on your setup.

## Builds

- Standalone: `D:\dev\build\aeriform-dsp-rack\Aeriform_artefacts\Release\Standalone\AERIFORM.exe`
- VST3 bundle: `D:\dev\build\aeriform-dsp-rack\Aeriform_artefacts\Release\VST3\AERIFORM.vst3`

Use copies of test patches/projects when saving. Old optional-effect settings are intentionally not migrated into the rack.

## First pass — about 15 minutes

### 1. Launch and layout

- [ ] Launch the standalone, select your audio/MIDI device, and play notes.
- [ ] Load the VST3 in your usual DAW and confirm MIDI input and stereo output work.
- [ ] Open Effects at the minimum size (1280 × 900). Confirm all four slots and their full controls fit with no scrolling, clipped labels, or overlapping controls.
- [ ] Put Multiband Saturation in all four slots; it has the most controls. Check every knob, menu, and text value is usable.
- [ ] Resize the window and try your normal Windows/DAW display scaling. Close and reopen the editor.
- [ ] Confirm Chorus/Delay/Reverb are accessible on Main, with no duplicate controls on Effects. Check Main and Exciters still fit and work.

### 2. Rack behavior

- [ ] Try Resonant Delay, Shimmer, Spectral Freeze, and Multiband Saturation individually. Adjust every control enough to confirm it responds.
- [ ] Use two copies of one effect with very different settings. Editing one should leave the other unchanged.
- [ ] Change a slot to another type and back: its previous control settings should return. Its old audio tail is not expected to return.
- [ ] Combine Resonant Delay and strong Saturation, then swap their order with the arrows. The sound should change; each labeled instance should keep its settings.
- [ ] While playing, toggle bypass, change types, and reorder. Listen for loud clicks, bursts, stuck sound, or sudden unwanted level jumps. Brief fades through dry are intentional.
- [ ] For Spectral Freeze, play a sustained note, press Capture, then Release. Repeat; also bypass/re-enable and change types away/back. Check Hold behaves consistently and Release returns to live audio.
- [ ] Try four copies of your heaviest preferred effect. All combinations should remain available. Check for dropouts at your usual buffer size; increase the buffer to distinguish CPU overload from a functional fault.

### 3. Breath sound — most important listening check

- [ ] Start from an initialized patch at ordinary volume and velocity. Select Soft Exhale on the Exciters page. Is it clearly audible and convincingly like a soft human exhalation, rather than static, hiss, or a harsh attack?
- [ ] Select Flute Air. Does it evoke air blown across a flute mouthpiece? Note whether it needs more/less edge, body, or movement.
- [ ] Try Focused Jet, Whisper, and Rough Air. Are the five characters usefully distinct?
- [ ] Test low/middle/high notes and soft/medium/hard velocities. Include short notes, repeated notes, long holds, and releases.
- [ ] Adjust Air, Mouth, Turbulence, Swell, Settle, Contour, and Edge individually. Check each has a useful audible effect without unstable ringing or harsh spikes.
- [ ] Try pressure/breath-controller input if you use one. Check smooth transitions from gentle to strong playing.
- [ ] Select a character, edit one breath control, then undo/redo. The modified label should update; selecting a character should undo as one action. Main envelope and resonator settings should remain unchanged.

## Second pass — DAW and persistence

- [ ] Save a preset with four different slots, reordered, and a modified breath character. Reload it and check types, order, bypass states, and settings.
- [ ] Save and reopen a DAW project. Check the same settings, including a held Spectral Freeze. A fresh capture may be needed: frozen audio itself is not saved.
- [ ] Automate a slot control, reorder that slot, and play the automation. It should still control the same labeled instance.
- [ ] Try macro/modulation assignments to a new rack control and a new breath control. Confirm the sound responds and assignments survive save/reload.
- [ ] Exercise Undo/Redo and A/B with rack order, effect types, breath characters, and Spectral Capture/Release. Watch for unexpected captures or failure to release.
- [ ] Confirm permanent Chorus/Delay/Reverb settings survive saves and rack edits.
- [ ] Try familiar factory patches and non-breath patches. Note unexpected changes unrelated to the breath overhaul or intentionally replaced optional-effect chain. No need to build your new preset bank yet.
- [ ] If relevant, try 44.1/48/96 kHz, your normal and small buffers, multiple plugin instances, and offline export. Check DAW CPU use, dropouts, hanging notes, and crashes.

## Optional before/after listening

Files are under the repository's `artifacts` folder:

- New soft breath: `advanced-dsp/Soft-Exhale-60-60-synth-matched.wav`
- New flute air: `advanced-dsp/Flute-Air-60-60-synth-matched.wav`
- Previous full synth: `dsp-baseline/baseline-synth-60-76-matched.wav`

Compare matched files for tonal character; use files without `-matched` to judge actual output level. Isolated source versions are also available in `advanced-dsp`. Matching is RMS-based with a peak ceiling, not a guarantee of identical perceived loudness.

## What was already checked

The AI verifier passed all ten new advanced tests, including processing order, duplicate instances, state/undo, modulation, extreme settings, and tested callback allocations. Minimum-size UI snapshots were inspected. Three obsolete legacy host tests were adapted to the new rack and passed targeted reruns. Actual DAW hosting, subjective realism, and reliable performance on your hardware still need your checks. See `DSP-VERIFICATION.md` for the full results and limits.

## If something fails

Record the build, DAW/version (or standalone), sample rate/buffer, Windows scaling, patch, slot order/settings, and exact steps. Include a screenshot for layout problems or a short audio example for sound problems. For breath feedback, say which character, note/velocity, and whether the issue is attack, sustain, release, brightness, roughness, or level.
