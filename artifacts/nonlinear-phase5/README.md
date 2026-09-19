# Nonlinear demonstrator comparisons

Each `Loop <Model>-off.wav` / `-on.wav` pair uses the corresponding new factory
preset, A3 (MIDI 57), velocity 110, 0.8 seconds held plus 0.2 seconds released.
The render is rounded to 256-sample blocks at 48 kHz. Files are 24-bit mono
fold-downs, without loudness normalization. Only slot A's nonlinear On parameter
changes between each pair; separate fresh processor instances are used.

| Preset | Relative RMS difference |
|---|---|
| Loop Saturate | 0.981733 |
| Loop Hysteresis | 0.067549 |
| Loop Tension | 0.989075 |
| Loop Friction | 0.063643 |

These ratios compare difference RMS with disabled RMS. They establish a signal
change, not sonic quality or subjective audibility. Listening acceptance belongs
to the testing agent. Loop Friction's resonator input is 0.08, intentionally lower
than the other demos to keep excitation in the useful force range.

The four `nonlinear-phaseN-network.png` images show the model controls and live
visualizations in a common Init-derived UI fixture, not the demonstrator presets.
The filenames correspond to Saturate (1), Hysteresis (2), Tension (3), Friction (4).
Final focused test log: `D:/dev/Aeriform-ORCH/Build/nl-phase5-tests.log`.

See ../../docs/NONLINEAR_LOOP_IMPLEMENTATION.md for the implementation handoff.
