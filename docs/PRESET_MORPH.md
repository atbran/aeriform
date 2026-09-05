# A/B snapshot morphing

Use PLAY to capture or load both endpoints. EDIT A and EDIT B explicitly choose which endpoint the ordinary controls edit. Capture copies the current controls to a snapshot; selecting another endpoint stores the outgoing live values and loads the selected endpoint for editing. Snapshot values, names and selected endpoint are serialized with sessions and exported user presets. Version 1/2 sessions default to morph disabled.

Parameter mode interpolates continuous DSP values using an internal fixed-size effective layer. Frequencies, positive time constants and ratios interpolate logarithmically; pitch interpolates in semitones/cents. Mix and pan remain coordinates consumed by existing constant-power DSP. All discrete structural controls stay at the selected endpoint in this economy mode. Output, MIDI/voice administration, quality and morph/randomizer controls remain live controls outside the snapshots.

Deep mode runs two independent complete synthesis engines. Continuous values share the interpolated controls while each engine retains its endpoint's structural choices. A sample ramp applies equal-power audio mixing before a shared output limiter. Both engines run while Deep is active, including at the endpoints, so their notes and tails evolve. External input is copied before either engine writes in place. Newly enabling Deep primes its second engine with currently held notes; it does not reconstruct the historical resonator energy of notes played before activation.

The Morph parameter is host-automatable and smoothed over roughly 35 ms. It never writes hundreds of interpolated parameters into APVTS. GUI undo records explicit commands; automation playback is not recorded. A snapshot update uses atomic publication with a bounded retry count in the audio reader.

COMMIT is available for Parameter mode and the endpoints of Deep mode. An interior Deep blend contains two incompatible structures and cannot be flattened into a single conventional patch; COMMIT is disabled there. Save the full preset/session to preserve that blend. Four-corner XY is deferred as agreed.

The new tests cover serialization, frequency/time/pitch interpolation, distinct Deep structural arrays, finite automated rendering, host-value preservation and undo behavior. More extreme transition and CPU measurements remain part of final validation.
