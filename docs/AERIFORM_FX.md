# Aeriform FX — main-input effect conversion

Branch: `effect-version`. Product name: **Aeriform FX**. VST3 category `Fx`,
`IS_SYNTH=FALSE`, distinct plug-in code `Aefx`, `AERIFORM_FX=1` compile flag.

Aeriform FX is the Aeriform resonator engine turned into a conventional DAW
insert effect. Drop it on any audio track — drums, vocals, guitars, synths,
noise, field recordings, a full mix — press play, and the incoming audio is
driven through Aeriform's physical-model resonator network and its existing
effects chain. **No MIDI, no sidechain routing and no internal exciter are
required for audio to pass through and be processed.**

## Signal flow

```
DAW main input
  → Input Gain            (fx_input_gain, dB)
  → Resonator network     (Res A / B / C, routing, cross-feedback, energy loop,
                           reed junctions, body / formant filter) — shared maths
                           with the instrument voice, tuned to FX Root
  → Existing effects       (Chorus → Delay → Reverb → Output stage / limiter)
  → Dry / Wet mix          (fx_mix; linear crossfade against the untouched input)
  → Output Gain            (fx_output_gain, dB)
  → DAW output
```

The **dry** signal is the untouched main input. It never passes through the
resonators, the wavefolder, the effects chain or any non-linear stage — it is
captured once at the top of the block and crossfaded back in after everything
else. At 0 % wet the output is the input; at 100 % wet it is the processed
resonator/effects output.

## How the conversion works

### Note independence

The instrument derives every resonator's frequency from the MIDI note number
(`60 + (note - 60) * keytrack + coarse + fine + …`). Aeriform FX replaces the
note with the **FX Root** parameter (`fx_root_note`, MIDI-note units, default 60
= middle C). Every per-resonator Coarse / Fine / Ratio / Key Track control still
applies on top, unchanged, so existing tuning behaviour and presets carry over.
`buildNetworkParams()` — the function that maps parameters to the resonator
network — was factored out of `Voice` into `Source/DSP/NetworkParamsBuilder.h`
and is now called by both the voice (per note) and the FX path (per block, with
the FX Root note, a parameter-derived pressure and the global modulation
result). No resonator maths is duplicated.

### Continuous excitation, natural tails

`FxResonatorPath` (in `Source/DSP/SynthEngine.cpp`) owns **one**
`ResonatorNetwork` plus the body / formant filter — the same classes the voice
uses. Every block, the input-gained mono sum of the main input is fed sample by
sample into `ResonatorNetwork::next()`. The network is prepared once and reset
only on `prepareToPlay` / host reset — **never between blocks** — so an input
transient rings the resonators and the energy then decays on its own. Stop the
input and the DAW still receives the complete resonator tail
(`getTailLengthSeconds()` reports 8 s).

Excitation is a mono sum (L+R) driving the network's existing stereo output; the
dry path stays full stereo. This matches the engine's existing mono-excitation
design and avoids duplicating the whole network per channel.

### Where it joins the mix

The FX resonator output is added into the engine mix bus right after the voices
and before the global effects chain, so it flows through Chorus → Delay →
Reverb → Output stage exactly like synthesised voices, and is covered by the
existing NaN / runaway safety net (peak-bounded, `ResonatorNetwork` governor and
`isFinite()` flush). Dry/wet and FX Output Gain are applied after that, before
the peak/scope measurement, so the meters and safety check see the true output.

### Buses

| bus | index | default | purpose |
|-----|-------|---------|---------|
| Input     | in 0  | enabled  | main audio input — processed automatically |
| Output    | out 0 | enabled  | main audio output |
| Sidechain | in 1  | disabled | optional aux input for the existing sidechain features (Sidechain exciter model, `exc_ext_in`, per-slot freeze) |

`getPluginHasMainInput()` returns `true`. `isBusesLayoutSupported` accepts
mono/stereo main in and out and disabled/mono/stereo on the sidechain.
In-place hosts are safe: every input is copied to an internal buffer before any
output is written.

### MIDI and internal exciters are optional extras

MIDI input is still accepted (for MIDI-learn, the mod wheel and — if you want it
— the full polyphonic instrument on top of the effect). The internal exciter
system is untouched and still available. Neither is required: with no MIDI and a
silent input the plug-in is silent.

## New parameters

| ID | name | range | default |
|----|------|-------|---------|
| `fx_input_gain`  | FX Input  | −36…+12 dB | 0 dB |
| `fx_mix`         | FX Mix    | 0…100 %    | 100 % (fully wet, so the effect is audible on insert) |
| `fx_output_gain` | FX Output | −36…+12 dB | 0 dB |
| `fx_root_note`   | FX Root   | 12…108     | 60 (middle C) |

All four are appended after every existing parameter, so every earlier
parameter keeps its index and v1 / v2 states and presets load unchanged.
State-format version is bumped to **3**. `fx_input_gain` / `fx_output_gain` are
post-everything level trims and are *not* subject to the internal limiter.

## GUI

The MASTER panel's second row shows the FX I/O strip (Input / Mix / Output /
Root). `out_gain`, `out_hp`, unison spread and bend range remain fully
automatable but are not drawn on this compact panel in the FX build.

## Acceptance test

`Tests/SidechainTests.cpp`:

* `fx_main_input_is_processed_without_any_midi` — noise burst in, no MIDI:
  the resonators are driven, then ring and decay after the input stops; silence
  in gives silence out.
* `fx_dry_wet_input_and_output_gain` — 0 % wet passes the dry input at unity,
  100 % wet is the resonator output, output gain attenuates as expected.
* `fx_and_sidechain_bus_layouts_are_supported` — bus names/counts,
  `getPluginHasMainInput()`, supported layouts.
* `sidechain_input_excites_the_resonators` / `sidechain_exciter_model_…` — the
  existing sidechain features still work on the aux bus.

`Tests/HostCheck/HostCheckMain.cpp` loads the built VST3 as a DAW would and
checks it reports as an audio effect named "Aeriform FX", processes main input
with no MIDI at three sample rates / three block sizes, stays silent with no
input, and still renders optional MIDI voices.
