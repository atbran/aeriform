# Additional feature ideas for EXP_Aeriform

Proposals only: separate user approval is required before implementing any of these additions. Required takeover features are tracked separately in TASKS.md.

| Feature | Musical purpose | DSP approach | CPU | Difficulty | Existing architecture interaction | Main risk | Priority |
|---|---|---|---|---|---|---|---|
| Scala/MTS microtuning | Alternate temperaments | Immutable per-note frequency table; off-thread scale parser | Negligible | Medium | Voice and sympathetic tuning | Held-note retuning discontinuities | High |
| Topology sequencer | Rhythmic physical transformations | Tempo-scheduled routing scenes through transition engine | Low, higher during fades | High | Network and movable filters | Overlapping transitions exceed CPU budget | Medium |
| Excitation phrase looper | Replay a breath or scrape | Preallocated stereo ring and windowed loop boundaries | Low | Medium | External exciter input | Memory and sample-state persistence | High |
| Per-note spatial coupling | Move individual notes with expression | MPE-driven azimuth and normalized room sends | Low | Medium | Stereo network and room | Voice-dependent feedback gain | High |
| Preset breeding | Explore related sound families | Seeded constrained parameter crossover | Offline | Medium | Randomizer and snapshots | Incompatible structural combinations | Medium |
| Macro scenes | Recall expressive variations | Sparse smoothed offsets over effective parameters | Low | Medium | Morph and modulation | Priority conflicts with automation | High |
| Gesture recorder | Repeat hand-played motion | Fixed control-event ring with tempo-relative playback | Low | Medium | Macros and morph | Transport jumps and buffer overflow | Medium |
| Feedback stability map | Understand energy circulation | Background linearized gain analysis plus energy telemetry | Low audio, medium background | High | Network and room | False confidence for nonlinear paths | Medium |
| Opt-in adaptive quality | Match quality to voice budget | Displayed hysteretic quality scheduler | Low overhead | High | Oversampling and deep morph | Audible changes and offline determinism | Low |
| Harmonic/chaos zones | Move between tuned and unruly sounds | Pitch attractors blended with bounded detuning | Low | Medium | Resonator tuning | Discontinuous pitch attraction | High |
| Spectral imprint | Transfer external timbre to the instrument | Analysis filterbank envelopes drive modal weights | Medium | High | Sidechain and sympathetic modes | Gain compensation and unstable envelopes | Medium |
| Half-pedal gestures | Expressive piano-style damping | Smoothed pedal curve changes per-mode losses | Low | Medium | Sympathetic bank | Pedal/freeze restoration ambiguity | High |
| Material designer | Explore wood, glass and metal families | Constrained modal-ratio and decay interpolation | Low | High | Modal resonators | Labels implying unmeasured physical accuracy | Medium |
