# Resonator nonlinearity

Each resonator slot on NETWORK has a **Nonlinearity** tab. Enable its element,
choose a model and position, then adjust Amount. The original resonator
Saturation control remains separate. Existing patches load with the new element
off; Amount at zero also leaves samples unchanged.

## Models

**Saturate** softens peaks. Drive increases the gain into a cubic soft clip, with
compensation after it. Bias changes the asymmetry; the static offset is removed.
The global ADAA switch reduces aliasing in waveform paths. In waveguide loops,
tuning compensation accounts for its phase delay and the wet/dry blend. Modal
pre/post uses radial attenuation of the mode states, without ADAA delay. The
waveform saturation branch removes DC in loop positions and biased pickup use.

**Hysteresis** adds memory: the response depends on recent input as well as the
current sample. Drive widens the memory bands and Amount increases their effect.
Its four weighted play operators cannot exceed the running input peak. Modal
pre/post converts that response to radial loss, capped at unity. Bias and ADAA do
not affect Hysteresis.

**Tension** raises pitch as loop energy grows and relaxes toward nominal tuning
as energy falls. Amount sets depth, capped at a frequency ratio of 1.03 (about
51 cents). Waveguides shorten the fractional delay with limited movement; modal
resonators smoothly raise their mode frequencies. Pre and post both affect tuning.
Pickup does not act on Tension: select pre or post. Drive, Bias and ADAA do not
affect this model. The panel shows the applied pitch offset rather than an X/Y
trace; individual high modes can reach their frequency ceiling sooner.

**Friction** responds to changes between successive samples. Drive sets force
range and Amount sets contact strength. Signed force in waveguides and pickup
paths can add local energy; an immediate limit holds output squared energy to
at most 1.02 times input squared energy, with a separate adaptive force budget.
Injection tapers away at larger signals and cannot begin from silence. Modal
pre/post uses only radial loss to preserve its recurrence structure. Bias and
ADAA do not affect Friction. These are sound-design models, not calibrated
simulations of particular materials.

## Position and controls

| Position | Waveguide / comb | Modal resonators |
|---|---|---|
| pre | Returning delay signal before loop losses | Radial gain before the mode recurrence |
| post | Signal written back into the delay | Radial gain after the mode recurrence |
| pickup | Output taps; does not change internal feedback | Output taps after the mode sum |

Drive and Amount have separate modulation destinations for slots A, B and C.
Drive is keytracked downward above A3 to temper high-note excitation. Controls
smooth over roughly 5 ms; model, position and enable changes fade through the
unchanged signal. Bias is available only for Saturate. The ADAA switch on slot A
is shared across all slots and applies only to Saturate's waveform paths.
See [parameter reference](PARAMETERS.md#nonlinear-loop-controls-state-version-4)
for IDs, ranges and defaults.

The live X/Y view plots actual input/output pairs from the newest voice's left
slot. Modal pre/post shows the first mode's radial response. It is not a static
transfer curve or a whole-mix scope. The Tension meter occupies the same area.

## Demonstrator presets

Four presets in the **Nonlinear** category follow all 42 pre-existing factory
entries. Each uses slot A alone with the original chorus, delay and reverb dry,
so toggling slot A's nonlinear On switch gives a direct comparison. Start at A3
(MIDI 57), play at a firm velocity, and compare both held notes and releases.

| Preset | Setup | What to compare |
|---|---|---|
| Loop Saturate | Bright wave into Open Pipe, Drive 85 | Peak shape and sustained tone |
| Loop Hysteresis | Triangle-like wave into String, Drive 100 | Memory-dependent shape and decay |
| Loop Tension | Decaying excitation into String, Amount 100 | Pitch at the attack and as energy falls |
| Loop Friction | Sine into Open Pipe, Drive 100 | Velocity-dependent response while held |

Short on/off renders are supplied for audition; numerical signal differences do
not establish a subjective listening result. Full sonic, tuning, aliasing, CPU
and maximum-feedback acceptance remains with the separately assigned testing
agent. Local element bounds alone do not prove whole-network stability.

## Implementation and verification

No host latency is added. Loop processing, histories, energy probes and scope
queues use fixed storage. Original parameter indices are preserved; nineteen
parameters and six modulation destinations were appended, and state version 4
provides disabled defaults when loading old states.

See [implementation handoff](NONLINEAR_LOOP_IMPLEMENTATION.md) for the concrete
DSP formulas and phase checks, [acceptance specification](NONLINEAR_LOOP_TESTING.md)
for the pending measurements, and [CPU notes](PERFORMANCE.md) for profiling status.
