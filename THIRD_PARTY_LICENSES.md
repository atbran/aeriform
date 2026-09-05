# Third-party licences and distribution obligations

AERIFORM's own code is MIT licensed (see `LICENSE`). A built plug-in or
standalone binary additionally contains the components below. Read this before
distributing binaries.

## JUCE 7.0.12 (`external/JUCE`)

- Licence: dual **GPLv3 / commercial** for the `juce_*` modules used here
  (`juce_audio_basics`, `juce_audio_devices`, `juce_audio_formats`,
  `juce_audio_plugin_client`, `juce_audio_processors`, `juce_audio_utils`,
  `juce_core`, `juce_data_structures`, `juce_dsp`, `juce_events`,
  `juce_graphics`, `juce_gui_basics`, `juce_gui_extra`). Full text:
  `external/JUCE/LICENSE.md`.
- **What this means for AERIFORM builds:**
  - Building and using the plug-in privately, or distributing it as **open
    source under GPLv3**, needs no JUCE licence purchase. When distributing under
    the GPL, the combined work (AERIFORM + JUCE) is GPLv3; the MIT licence on
    AERIFORM's own files is compatible with that.
  - Distributing **closed-source** binaries requires a JUCE commercial licence
    (JUCE Personal is free for revenue under 50 K USD but requires the JUCE
    splash screen; Indie / Pro tiers remove it). The build sets
    `JUCE_DISPLAY_SPLASH_SCREEN=0`, which is only permitted under the GPL or a
    paid licence. If you build closed-source under JUCE Personal, set it to `1`.
- JUCE bundles several permissively licensed libraries that end up in the
  binary: zlib (zlib licence), pnglib (zlib), jpeglib (IJG), Oboe (Apache 2.0,
  Android only), FLAC and Ogg Vorbis (BSD, disabled in this build via
  `JUCE_USE_FLAC=0` / `JUCE_USE_OGGVORBIS=0`).

## Steinberg VST3 SDK (bundled inside JUCE, `modules/juce_audio_processors/format_types/VST3_SDK`)

- Licence: dual **GPLv3 / Proprietary Steinberg VST3 Licence**. Full text in
  `external/JUCE/modules/juce_audio_processors/format_types/VST3_SDK/LICENSE.txt`.
- Distributing a VST3 plug-in under the proprietary licence requires accepting
  the Steinberg VST3 Plug-In SDK Licensing Agreement (free of charge, via
  Steinberg's developer portal). Under GPLv3 no agreement is needed.
- "VST" is a trademark of Steinberg Media Technologies GmbH.

## MinGW-w64 / GCC runtime (Windows builds with the portable toolchain)

- The Windows binaries are linked with `-static -static-libgcc -static-libstdc++`,
  so `libstdc++`, `libgcc` and `libwinpthread` are embedded. These are licensed
  under the GPL **with the GCC Runtime Library Exception** (libstdc++/libgcc) and
  the MIT/BSD-style mingw-w64 licences (winpthread, CRT glue), which permit
  static linking into programs of any licence. No source-distribution obligation
  arises from the runtime itself.
- The toolchain (WinLibs GCC 14.2, Ninja, CMake) is a build tool only and is not
  part of the shipped binaries.

## pluginval (validation tool only)

- Tracktion pluginval is GPLv3 and is used only as a development-time validator;
  it is not linked into or distributed with AERIFORM.

## Trademarks and originality

- "Steam Pipe" is a product of Erica Synths. AERIFORM is an original design
  inspired only by the general concept of an oscillator-free exciter / resonator
  architecture. It contains no Erica Synths code, presets, artwork, panel layout
  or branding.
