# AERIFORM

**An oscillator-free physical-modelling synthesizer with a complex exciter
generator and a resonator network.**
VST3 + Standalone, 8-voice polyphonic (up to 16), MPE, sidechain excitation.
C++20 / JUCE 7.0.12 / CMake. Version 3.0.

[**🎧 Listen to demos**](https://atbran.github.io/aeriform/) · [**⬇️ Download latest release**](https://github.com/atbran/aeriform/releases/latest)

![AERIFORM interface (MAIN page)](docs/screenshot.png)

AERIFORM has no oscillators in the traditional sense. Two exciter slots —
band-limited waveforms, an original chaotic "orbit" oscillator, a 12-model
noise laboratory, physical exciters (reed, bow, mallet, pluck...), or
sidechain audio — combine through 13 interaction modes, a pre-shaper and an
oversampled wavefolder, then excite a network of up to three tuned resonators
(9 models: pipes, string, comb, dispersive tube, modal bank, metallic bar,
membrane, formant body) with flexible routing, six governed cross-feedback
routes, a **Repipe** macro and an optional energy loop. On top of that:
movable filters (including inside a resonator's own feedback loop), collision
routing between resonators, a true stereo network, a shared sympathetic bank
and room, four extra effects, and full A/B patch morphing with a seeded
randomizer.

It is an original design inspired only by the general concept of
exciter -> resonator -> space instruments, and contains no code, algorithms,
presets, panel layouts, artwork or branding from any other product.

## Quick start

1. [Download the latest release](https://github.com/atbran/aeriform/releases/latest) (Windows x64).
2. **VST3**: copy the `VST3\AERIFORM.vst3` folder to `C:\Program Files\Common Files\VST3\` and rescan in your DAW.
   **Standalone**: run `AERIFORM.exe` directly.
3. Open the factory preset browser and start with **Airy Flute** or **Repipe Morph**.

## Documentation

| | |
|---|---|
| 📖 [User guide](docs/GUIDE.md) | Pages, sidechain, MPE, MIDI learn, Repipe, energy loop, known limitations |
| 🏗️ [Architecture & DSP](docs/ARCHITECTURE.md) | Signal flow, every exciter/resonator model, stability |
| 🎛️ [Parameter reference](docs/PARAMETERS.md) | All parameters: ID, range, default, description |
| 🎹 [Presets](docs/PRESETS.md) | The full factory bank |
| 🧪 [Testing & validation](docs/TESTING.md) | How to run the test suite, last recorded results |
| ⚡ [Performance](docs/PERFORMANCE.md) | CPU profile by configuration and quality mode |
| 🛠️ [Building from source](docs/BUILDING.md) | Prerequisites, exact build commands, all platforms |
| 📝 [Changelog](CHANGELOG.md) | What's new in each version |
| ⚖️ [Third-party licenses](THIRD_PARTY_LICENSES.md) | JUCE / VST3 SDK distribution terms |

New-module deep dives: [effects](docs/EFFECTS.md) · [filters](docs/FILTER_ROUTING.md) ·
[resonator network](docs/RESONATOR_NETWORK.md) · [patch morphing](docs/PRESET_MORPH.md) ·
[randomizer](docs/RANDOMIZER.md)

## Licensing

AERIFORM's code is MIT licensed. Binaries include JUCE (GPLv3 / commercial) and
the Steinberg VST3 SDK (GPLv3 / proprietary), so **distributed binaries are
GPLv3 unless you hold a JUCE commercial licence and the Steinberg VST3
licence**. See [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md) for full
details.
