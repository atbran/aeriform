# User guide

## Pages

Six tabs: **MAIN, EXCITERS, NETWORK, MOTION, SPACE, ADVANCED**.

- **MAIN** — the playing page: both exciters in compact form (model, level,
  tone and the three most characteristic controls of the chosen model),
  interaction, folder essentials, breath envelope, the airflow visualiser,
  Resonator A, the network overview with the Repipe macro, motion (matrix
  slots 1-8) and master.
- **EXCITERS** — both slots in full, with waveform scopes, the interaction
  stage (with a one-line hint per mode), pre-shaper, envelope / articulation,
  and the wavefolder with its transfer curve.
- **NETWORK** — the interactive resonator-network diagram (drag an arrow or
  scroll to change a route, double-click a node to enable / disable a
  resonator), the network controls, the energy loop, all three resonators,
  contact/collision routing, the true stereo network, and the shared
  sympathetic bank and coupled room. See [RESONATOR_NETWORK.md](RESONATOR_NETWORK.md).
- **MOTION** — the three LFOs, the mod envelope and all 16 matrix slots.
- **SPACE** — chorus/delay/reverb, movable filters, and the new resonant
  delay / shimmer / spectral freeze / multiband saturation effects, plus a
  live signal-flow legend and the MIDI mapping list. See [EFFECTS.md](EFFECTS.md)
  and [FILTER_ROUTING.md](FILTER_ROUTING.md).
- **ADVANCED** — A/B patch morphing and the seeded randomizer. See
  [PRESET_MORPH.md](PRESET_MORPH.md) and [RANDOMIZER.md](RANDOMIZER.md).

## Sidechain / external audio

The plug-in declares a stereo *Sidechain* input bus (a VST3 aux input, so DAWs
list it in their sidechain routing). Either pick the **Sidechain** model in an
exciter slot (with its own filters, envelope follower, transient extraction
and Freeze) or use the Breath model's *Sidechain* knob (`exc_ext_in`). The
audio is summed to mono, run through the chain and injected into the
resonators of every note you hold, so it is forced to resonate at the notes
you play; the resonators are note-gated — with no note held nothing passes
through. The envelope follower is also a matrix source (*Sidechain Env*).

- Ableton Live: add AERIFORM on a MIDI track, open the device's sidechain
  section and pick the audio track. Bitwig / Reaper / Cubase / Studio One:
  route an audio track's send or output into the instrument's sidechain input.
- Standalone: un-mute the audio input in the audio settings and blow into a mic.

## Repipe

Start from any single-tube sound and turn **REPIPE** up: Resonator B and C are
brought in as a serial chain fed from A, then the cross-feedback routes
B -> A, C -> A, C -> B and A -> C open progressively, the network feedback
scale and drive rise and the output mix rebalances. At 100 % you have a
cross-fed three-resonator network; modulate Repipe from the matrix for
morphing timbres. Repipe never changes your own routing settings; it only
imposes minimum values while it is above zero.

## Energy loop

Off by default. When on, resonator energy (mix or one slot) is filtered,
delayed, saturated and injected back into the pre-shaper input, the folder
input or the network input. Loop gain is always bounded by the tanh return
path and the governor; expect drones, growls and self-playing textures rather
than runaway.

## MPE

Enable **MPE** in MASTER. The lower zone uses channels 2-16 with a
48-semitone per-note bend range; per-note pressure maps to *Aftertouch*, slide
(CC74) to *MPE Slide*, per-note pitch bend is applied directly.

## MIDI learn

Right-click any knob -> *MIDI Learn*, move a controller. Mappings are saved
with the session / preset state and listed on the SPACE page.

## Fine control

Shift-drag = fine; Ctrl-drag = velocity mode; double-click = default; mouse
wheel works on every knob. Right-click a knob for **Assign modulation** to add
a matrix route to an open slot, then drag the knob's teal ring (or Alt-drag
the knob) for a signed depth.

## Known limitations

- The exciter chain's oversampling makes the default patch noticeably more
  expensive than the original v0.1 instrument in Normal quality; use Eco for
  the lowest cost when the wavefolder is off.
- 16 voices with two oversampled exciters, the wavefolder and a full network
  in High quality approach one CPU core on the development machine.
- Waveguide notes above roughly 5 kHz with maximum dispersion cannot be tuned
  exactly (clamped safely); modal models are exact by construction.
- Autocorrelation-measured pitch of waveguides reads a few cents flat below
  C2; the fundamental itself is exact.
- Sidechain audio only sounds while notes are held; Freeze holds at most
  250 ms and is never stored.
- Cross-feedback between resonators tuned far apart mostly adds colour rather
  than pitched resonance (by design of the coupling normalisation).
- Spectral freeze has no dedicated GUI page yet; use its host parameters or a
  MIDI-learned knob.
- The MinGW build renders text with GDI (no DirectWrite); an MSVC build looks
  marginally crisper.
- macOS / Linux / AU builds are configured but untested on the development
  machine.

See [ARCHITECTURE.md](ARCHITECTURE.md) for how the signal path works and
[PARAMETERS.md](PARAMETERS.md) for the complete parameter reference.

---
Back to [README](../README.md).
