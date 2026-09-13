# Changelog

## v3.2.0

A major audio processing and sound design release introducing authentic Roland Juno BBD chorus emulation, expanded algorithmic reverbs, dedicated Motion vibrato controls, pure waveguide bore radiation without dry exciter bleed, and critical DSP repairs across resonant delays, shimmer reverb, FM/sync interaction, and vocal breath formants.

### Authentic Roland Juno BBD Chorus Emulation
- **5 Chorus Algorithms**:
  - `Ensemble`: Classic multi-voice rich stereo ensemble chorus.
  - `Juno I`: Slow, deep lush swirl (0.513 Hz LFO, 1.66 ms to 4.21 ms hyperbolic delay sweep).
  - `Juno II`: Faster, shimmering vibrato chorus (0.863 Hz LFO, 1.66 ms to 3.51 ms hyperbolic delay sweep).
  - `Juno I+II`: Dual-LFO non-linear interaction mode (0.513 Hz + 1.00 Hz compound modulation).
  - `Dimension`: Static spatial widening delay with subtle anti-phase pitch movement.
- **Circuit-Accurate BBD Modeling**: Dual bucket-brigade delay lines with exact anti-phase right-channel modulation, hyperbolic clock sweeping ($\tau(t) = 1 / f_{\text{clock}}(t)$), pre-de-emphasis (7.2 kHz 2-pole lowpass, 2.3 kHz pre-emphasis shelf), post-BBD reconstruction filtering, and soft analog overdrive saturation.
- **SpacePanel UI Strip**: Integrated 5-button segmented algorithm selector with immediate visual feedback and automatable parameter attachment.

### Expanded Algorithmic Reverbs
- **3 Spatial Engines**:
  - `Hall`: 8-delay-line Feedback Delay Network (FDN) with orthogonal Householder feedback matrix, prime delay lengths, Schroeder allpass diffusers, and per-line high-frequency absorption.
  - `Room`: Densely coupled dual-room chamber with distinct early reflection taps and lively diffuse decay.
  - `Plate`: Classic continuous Dattorro plate tank with input diffusers, cross-coupled delay loops, inner allpasses, and damping decay.
- **Click-Free Transitions**: Equal-power 20 ms crossfader during algorithm switches.

### Dedicated Vibrato Controls (Motion Panel)
- **Vibrato Speed (`vib_rate`)**: Continuous rate from 0.5 Hz to 15.0 Hz (default 5.0 Hz).
- **Vibrato Depth (`vib_depth`)**: 0.0% to 100.0% depth (up to $\pm 50$ cents peak excursion).
- **Per-Voice Sinusoidal Phase**: Smooth per-voice accumulator starting at zero on note attack and flowing seamlessly through legato phrasing. Modulates note pitch across both exciter models and physical resonator waveguide lengths simultaneously.
- **UI Integration**: Positioned directly beside the Modulation Envelope in the Motion panel with teal accents.

### Pure Waveguide Resonators (Zero Dry Bleed)
- **Acoustic Bore Radiation Output**: Fixed waveguide pipe and tube modes (`OpenPipe`, `ClosedPipe`, `DispersiveTube`, and `Comb`) to tap the radiated acoustic delay wave ($d$) rather than the mouthpiece pressure junction ($x = \text{reflected} + \text{in}$).
- At 100% wetness, raw dry exciter impulses and waveforms are completely removed at $t=0$, leaving only the pure resonant acoustic response.

### DSP Defect Repairs & Calibrations
- **Resonant Delay**: Removed input starvation factor `(1.0 - feedback)`, added `1.2 * fastTanh` feedback saturation, and widened modal filter bandwidths for audibly sustained, rich repeats.
- **Shimmer Reverb**: Calibrated feedback reinjection loop gain compensating for room attenuation, fixed feedback highpass filtering, and eliminated wet volume drop. Pitch-shifted octave-up tails cascade continuously.
- **Interaction Modes**: Replaced exponential FM with textbook Phase Modulation FM ($I = 0$ to $8.5$ radians) keeping the fundamental in-tune with full Bessel harmonic sidebands; added unison pitch sweep to Hard Sync; implemented peak/trough full-wave envelope selection for Min/Max.
- **Breath Articulation**: Implemented 3-formant resonant filter bank tracking natural vowel formant spaces ("oo" -> "ah" -> "ee") plus adjustable high-frequency embouchure air edge.

### Verification & Compatibility
- Preserves all existing factory presets and parameter layouts (all new parameters default off or neutral).
- Full automated test suite: 162 test suites, 30,802,472 assertions, 0 failures.

## v3.1.0

This release introduces an overhauled, streamlined user interface, four dedicated macro controls with full modulation routing, right-click macro mapping, and visual polish across the synth.

### 4 Modulatable Macro Controls
- **Dedicated Macro Panel**: 4 macro knobs arranged in a 2x2 grid on the Main page in copper accent styling.
- **Right-Click Assignment**: Right-clicking any parameter now provides an "Assign Macro" menu (displaying Macro 1–4 with live custom names), mapping the macro immediately as an additive modulation source.
- **Modulatable Macros**: Macros 1–4 are both modulation sources (`ModSource::Macro1..4`) and modulation destinations (`ModDest::Macro1..4`), evaluated in a two-pass resolution pass with additive offset $\text{effective} = \text{clamp01}(\text{baseKnob} + \text{modSum})$. LFOs, envelopes, and other macros can modulate macro knobs.
- **Editable Macro Names**: Double-clicking a macro label allows in-place renaming; custom macro names are serialized into the plugin state XML and restored across sessions and presets.
- **32-Slot Modulation Matrix**: Expanded the modulation matrix capacity from 16 to 32 slots (`ids::numModSlots = 32`) to accommodate macro assignments without consuming voice modulation slots.

### UI Overhaul & Visual Polish
- **Effects Tab Styling**: Replaced the previous button styling on the Effects / Space panel with clean, low-profile tab buttons featuring subtle dark backgrounds and teal active underlines.
- **Network Diagram Bypass Path Alignment**: Repositioned the "DIRECT EXCITER PATH" bypass indicator to the bottom-left of the canvas, eliminating visual overlap with Resonator Node C.
- **Streamlined Main Surface**: Clean arrangement uniting exciters, breath envelope, resonator tabs, visualizer modes (Pipe, Scope, Spectrum), LFOs, modulation envelope, network essentials, macros, and master section.

### Verification & Compatibility
- All 561 parameters preserved with backwards-compatible defaults, IDs, and normalized ranges.
- Full test suite: 141 tests, 17,113,467 checks, 0 failures.
- Complete VST3 host validation passing across 44.1/48/96 kHz and 32/256/1024-sample block sizes.

## v3.0.0

A large expansion of the exciter/resonator instrument introduced in v2.1, developed and independently validated in parallel, then merged into the main release. Every original parameter ID, factory preset and default sound is unchanged; all new DSP defaults off. 40 original factory presets measured bit-identical against the v2.1 baseline (see [docs/AUDIBILITY_RESULTS.md](docs/AUDIBILITY_RESULTS.md)).

The interface is now six tabs: **MAIN, EXCITERS, NETWORK, MOTION, SPACE, ADVANCED**.

### A/B morph and patch tools (ADVANCED)
- Capture two full patch snapshots (A/B); a single host-automatable **Morph** knob crossfades between them.
  - **Parameter mode** interpolates continuous values only (frequencies and time constants logarithmically, pitch in semitones/cents) while structural choices (model selections, routing) stay at whichever endpoint is closer — cheap, always available.
  - **Deep mode** runs two complete synthesis engines side by side and mixes their audio, so structurally different patches (e.g. different resonator models on each side) actually morph rather than snap.
- **Seeded randomizer**: an editable 32-bit seed, Randomize, Mutate (nudges the current patch instead of replacing it), a Wild mode for wider ranges, and per-control locks (right-click any knob/choice/toggle) so parts of a patch can be protected while the rest is randomized.
- **Undo/redo** (Ctrl+Z / Ctrl+Shift+Z) for edits, randomization and mutation as discrete, reversible actions.
- **Favorites**: persistent, stable across preset renames and factory-list reordering.
- See [docs/PRESET_MORPH.md](docs/PRESET_MORPH.md) and [docs/RANDOMIZER.md](docs/RANDOMIZER.md).

### Movable filters (SPACE, FILTERS)
- Three independent filter blocks, each with a choice of low-pass, high-pass, band-pass, notch, morphing SVF, driven SVF, a ladder-style low-pass, formant, comb, modal-bank, or tilt response.
- Each block can be inserted at any of 20 positions in the signal path, including **inside a resonator's own feedback loop** (A, B or C) — the waveguide compensates the filter's phase delay so the resonator stays in tune.
- Smooth, click-free transitions when changing a filter's model or position while playing.

### Contact / collision routing (NETWORK, CONTACT)
- A bounded, oversampled (1x/2x/4x) contact route between two resonators: energy only transfers once one resonator's displacement crosses a gap relative to the other, shaped by stiffness, hardness, friction, damping and asymmetry. Off by default; disabled routes are bit-identical to no contact at all.

### True stereo resonator network (NETWORK, PHYSICAL)
- Switches the resonator network from one shared engine to independent left/right instances with detuning (cents), interchannel coupling, offset excitation/pickup/damping, mid-side rotation, width, and low-frequency mono convergence.
- The default "Economy" (mono) mode is bit-identical to the v2.1 network.

### Shared sympathetic bank (NETWORK, SYMPATHETIC)
- A 12-mode resonant bank that every voice excites, for piano-style sympathetic ringing. Tunable to chromatic/major/minor/pentatonic/whole-tone scales, a harmonic series, twelve custom intervals, currently-held notes (with sustain pedal), or a captured MIDI chord.

### Shared coupled room (NETWORK, ROOM)
- An 8-line shared room (early reflections plus a late, bounded feedback core) that the whole instrument plays into, with a filtered, delayed, energy-budgeted return back into the resonators. A demonstration preset, "Reed in a Small Room," is included.

### New effects (SPACE)
- **Resonant delay**: a stereo delay whose feedback passes through six contracting resonant modes (harmonic, metallic-bar, membrane or vowel-like colour families), with tempo sync, stereo offsets, dispersion and saturation.
- **Shimmer reverb**: an independent pitch-shifted feedback path (±24 semitones, 0.01-semitone resolution, with octave/fifth/fourth shortcuts) layered onto the reverb tail.
- **Spectral freeze**: captures and holds the spectrum of whatever is playing, with blur, spectral shift, randomized phase and decay controls. (DSP and host parameters are complete; a dedicated GUI page is still pending — control it from the host's parameter list or MIDI-learned knobs in the meantime.)
- **Multiband saturation**: a 3-band crossover with an independent drive/character (soft, warm, clip, fold)/mix/output per band.

### Other fixes from user testing
- Explicit resonator bypass (and turning off every resonator) now routes excitation straight through, bypassing the network cleanly.
- Right-click **Assign modulation** on any knob adds a route to an open matrix slot; drag the knob's teal ring (or Alt-drag the knob) to set a signed depth without opening the matrix page.
- Contact routing's audible strength was increased based on listening feedback.

### Compatibility
- State format bumped to version 3; version-1 and version-2 sessions and presets load unchanged, morph disabled by default.
- Parameter table grew from 353 IDs (v2.1) to 492 IDs; every v2.1 ID keeps its original meaning.

### Known limitations
- Spectral freeze has no dedicated GUI page yet (host parameters only).
- A full realtime-safety audit of the newly added DSP (allocation/locking under worst-case load) is still outstanding; JUCE's internal `MPEInstrument` lock was already a pre-existing note from v2.1.
- Filter tuning inside a resonator loop is phase-delay (not group-delay) compensated; formant and modal-bank filter positions have no phase compensation yet. See [docs/FILTER_ROUTING.md](docs/FILTER_ROUTING.md) for exact tuning tolerances.
- The room and sympathetic-bank return levels are deliberately conservative; see [docs/AUDIBILITY_RESULTS.md](docs/AUDIBILITY_RESULTS.md) for measured levels.

---

## v2.1.0

Dual exciters (band-limited waveforms, an original chaotic "orbit" oscillator, a 12-model noise laboratory, physical exciters, sidechain input), 13 interaction modes, a pre-shaper, an oversampled wavefolder, and a three-resonator network (9 models, 4 routing modes, 6 governed cross-feedback routes, the Repipe macro, an optional energy loop). Five-page GUI (MAIN, EXCITERS, NETWORK, MOTION, SPACE), 353 parameters, 40 factory presets, MPE.

| Area | v0.1 | v2.1 |
|---|---|---|
| Exciter | one breath / pluck / sidechain exciter | two slots, 25 models each, interaction stage, pre-shaper, oversampled wavefolder |
| Resonator | one waveguide (3 topologies) + body | three slots, 9 models, routing, cross-feedback, Repipe, energy loop, governor |
| Matrix | 8 slots, 15 sources, 23 destinations | 16 slots, 29 sources, 52 destinations |
| Parameters | 120 | 353 (all 120 old IDs unchanged) |
| Presets | 20 | 40 |
| GUI | one page | five pages, scopes, transfer curve, network diagram |
| Tests | 33 unit + 5 smoke | 64 unit + 8 smoke (incl. multi-minute fuzz, CPU profile, editor) |

A v0.1 session or preset loads with Exciter A = Breath, B = Off, the folder off, Single routing and the loop off, which is exactly the v0.1 signal path; the default chord renders at the same level as before.

## v0.1.0

Initial oscillator-free, breath-driven physical-modelling synth: one waveguide resonator (open pipe / closed pipe / string), a breath/pluck exciter with a pressure-driven reed junction, chorus/delay/reverb, an 8-slot modulation matrix, MPE, sidechain excitation. 120 parameters, 20 factory presets.
