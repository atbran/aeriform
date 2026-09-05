# AERIFORM v2.1 CPU profile

Measured by `AeriformTests --filter=smoke_cpu_profile` (`AERIFORM_PROFILE_SECONDS=4`)
on an Intel i5-11600K, Release build, GCC 14.2 MinGW-w64, 48 kHz, 256-sample blocks,
one core. Numbers are the fraction of real time needed to render the configuration
with the given number of held notes (breath envelope release 4 s, so every voice
is sounding for the whole measurement).

| Configuration | Eco | Normal | High |
|---|---|---|---|
| Default patch (Breath -> single resonator), 8 voices | 5.6 % | 7.9 % | 10.7 % |
| 2 exciters (Wave FM Complex) + 1 resonator, 8 voices | 9.8 % | 17.1 % | 28.2 % |
| 2 exciters + 3 resonators parallel, 8 voices | 13.7 % | 22.9 % | 32.5 % |
| 3 resonators serial (Breath), 8 voices | 9.1 % | 12.0 % | 14.7 % |
| Maximum cross-feedback (all six routes, feedback 100 %), 8 voices | 15.5 % | 22.3 % | 33.0 % |
| Energy loop on (serial network, loop -> folder in), 8 voices | 12.8 % | 14.3 % | 19.5 % |
| Wavefolder on (2 exciters, single resonator), 8 voices | 20.8 % | 22.9 % | 38.2 % |
| Everything: 2 exciters, folder, 3 resonators hybrid, cross-feedback, loop, effects, 8 voices | 21.7 % | 23.0 % | 37.4 % |
| Everything, 16 voices | 43.3 % | 46.2 % | 73.0 % |

The v0.1 measurement (8 voices + effects, default patch) was 3.7-4.0 %; the v2.1
default patch costs 5.6 % in Eco and 7.9 % in Normal because the exciter chain
now runs through the interaction / pre-shaper stages and (Normal) 2x oversampling
with a polyphase halfband decimator even when the folder is off.

## Quality modes

| Mode | Exciter-chain oversampling | Control rate | Intended use |
|---|---|---|---|
| Eco | 1x, or 2x only while the wavefolder is on | 64 samples | live playing, big polyphony |
| Normal (default) | 2x | 32 samples | general use |
| High | 4x | 32 samples | mix-down, heavy FM / PM / folding |

Oversampling covers everything from the exciters to the wavefolder (oscillators,
interaction modes, pre-shaper drive, folder). The resonator network, body EQ and
effects always run at the host rate. The alias measurement in the test suite
(`band_limited_wave_oscillator_has_low_aliasing`) puts the folded 25th harmonic
of a saw at 70 dB below the fundamental at 2x.

## Where the time goes

- Wave / Complex exciters at 2x-4x: PolyBLEP evaluation, the phase-coupled
  orbit operators and the per-sample chaotic map are the biggest single cost.
- The wavefolder itself is cheap; its cost is the oversampling it forces in Eco.
- Each additional resonator slot costs roughly what a v0.1 tube cost (waveguide)
  or slightly less (modal bank, 10-12 two-pole sections).
- Cross-feedback adds three delay-line reads, three one-pole filters and three
  tanh evaluations per sample per voice.

## Worst-case block time

The 90-second randomised fuzz (`smoke_randomized_parameter_fuzz`) reports the
worst single block over 25 random configurations (any sample rate 44.1-96 kHz,
any block 32-1024, 1-16 voices, High quality included, sidechain on): 8.6 ms,
which is 6.5x the budget of a 32-sample block at 96 kHz. That spike happens when
a fuzz step re-prepares the processor (new sample rate / block size) in the same
measured interval; steady-state blocks stay within budget for every profiled
configuration on this machine. Use Eco or Normal with 16 voices of the
"Everything" configuration on slower machines.
