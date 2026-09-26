# Blown PIPE: Jet and Reed mouthpieces

The PIPE resonator model (index 9) now has a **Blow** control per slot:

| Blow | What it models | Sounds like |
|---|---|---|
| **Jet** (default) | An air jet crossing a mouth and splitting on an edge (flue pipe) | Flute, recorder, organ flue pipe, pan pipes and whistles (Cylinder bore = stopped pipe) |
| **Reed** | A pressure-controlled valve that closes as the pressure difference grows | Clarinet (Cylinder bore), sax or oboe (Cone bore) |
| **Linear** | The original PIPE: pressure and noise are added into the loop | A resonant tube that rings from noise; it cannot sustain a tone from steady pressure |

Before this change, PIPE's loop gain was always below one and the excitation was
simply added, so steady pressure (DC) was blocked by the loop highpass and the
pipe could only colour noise. Measured on C4 it produced a harmonic-to-noise
ratio of about 7 dB, sat 38 cents flat, and the DC-only setting was silent
(-77 dBFS). With Jet it sustains a clean tone (HNR about 26 dB) within a few
cents of pitch, from steady pressure alone.

## How the Jet works

- **Pressure sets the jet speed**, U = √P. Speed scales how hard the jet drives
  the pipe, and it sets how long the jet takes to cross the mouth (τ). Harder
  blowing gives a faster jet, a brighter tone and a slightly higher pitch, and
  near the top of the range it **overblows** to the octave, as a real flute does.
- **Only the returning sound wave (and turbulence) deflects the jet.** After τ
  the jet reaches the edge, and a saturating profile splits it into and out of
  the pipe. **Drive** is the jet gain, **Hardness** is the jet profile (soft,
  tanh-like, hard-edged), and **Asymmetry** is how far the jet sits off the
  edge. An off-centre jet adds even harmonics.
- **Jet** (the new knob) is the lip-to-edge distance. Short is pure and dark,
  the middle is flute-like, and long is rich and breathy until it overblows at
  the far end (at gentle pressures from about 70 %).
- **Turbulence** (DC/Noise) deflects the jet and is radiated from the mouth, so
  the breath noise moves with the tone instead of sitting on top of it.
- The **output is the radiated sound**: the bore's open end plus the mouth,
  through a gentle radiation highpass. It is normalised by the round-trip
  loss, so Nominal Decay colours the tone without setting its loudness.
- Too much loss (a very short decay or strong damping) stops the pipe speaking,
  as with a leaky real pipe.

## How the Reed works

The reed's reflection coefficient is `offset - slope × (bore wave - mouth pressure)`,
clamped to [-1, 1]. **Jet** sets the reed opening (the offset) and **Drive** its
stiffness (the slope). **Pressure** is scaled against the pressure that shuts
the reed for the current settings. 0 % is 45 % of that, near the speaking
threshold; 100 % is 95 %, just short of choking. This means stiff or open reed
settings never leave the knob in a dead zone.

## Tuning

The loop length is compensated for the loop filters (as before) plus:

- **Jet**: the steady-state phase of the jet path. At steady state the jet gain
  settles where the loop gain is exactly one, which fixes the phase the jet
  adds. A small correction toward the first overtone accounts for harmonic
  pulling.
- **Reed**: a harmonic-weighted phase delay. The reed locks its rich overtones
  to the fundamental, and the loop filters leave the upper resonances flat, so
  the uncorrected tone is pulled 3 to 6 cents flat.
- In both modes the loop highpass is capped at f0 / 8. A highpass that leads
  the fundamental far more than the overtones made low reed notes multiphonic.

Measured with `AeriformTests --filter=pipe_mouthpieces_are_in_tune` (default
settings, DC/Noise 5 %):

| Mode | C2 to C7 | Notes |
|---|---|---|
| Jet, cone and cylinder | within ±4 cents | Measured ±1 to ±2.5 cents over Jet 20 to 60 % and every pressure and noise setting. Near the overblow edge the tone goes a few cents flat, as a real flute does. |
| Reed, cylinder and cone | within ±3 cents from C2 to C5 | Above C6 (altissimo) the harmonic pulling reaches about ±10 to 20 cents. This is reported, not asserted. |

## Presets

The appended factory presets use PIPE with the legacy exciters off, so every
sound comes from steady pressure: **Steam Flute**, **Pan Pipes**, **Steam
Calliope**, **Flue Organ 8'**, **Steam Whistle**, **Shakuhachi Air**, **Reed
Clarinet** and **Reed Sax**. Aftertouch and the mod wheel are routed to Pressure.
The performance Pressure destination now also drives PIPE pressure, so a breath
controller blows the pipe.

## Compatibility

`res_blow`/`rb_blow`/`rc_blow` and `res_jet`/`rb_jet`/`rc_jet` are appended
after all existing parameters. A session that already used PIPE loads with
Blow = Jet, which changes its sound on purpose. Set Blow to Linear for the
original behaviour; the element-level Linear contract tests in
`Tests/PipeTests.cpp` are unchanged.

## Related fixes outside PIPE

- **Breath exciter spectrum.** The breath noise ran only through three
  resonant vowel formants (about 0.77, 1.26 and 2.6 kHz). That left nothing
  below about 500 Hz, which starved the fundamental of every note under about
  C5, so tubes sounded thin and nasal. A broadband turbulence core, gently
  lowpassed, now sits under the formants. At C4 the fundamental of the wind
  presets rose 5 to 9 dB, and overall level stayed within about 1 dB.
- **Legacy reed junction (Open Pipe, Closed Pipe, Reed).** The reflection
  coefficient multiplied the mouth pressure instead of the bore wave, so at
  rest the reed reflected only about 30 % of the returning wave. That was too
  lossy to self-oscillate. It now uses the standard form,
  `x = mouth + (bore - mouth) × r`. Reed Song went from noise (HNR 1.6 dB) to a
  clarinet tone (HNR 23 dB, odd harmonics), and Warm Wooden Pipe's pitch
  jitter fell from about 500 to 14 cents.

## Known limitations

- **Brass Horn** puts a clarinet-style reed on an open, non-inverting bore at
  very high pressure. That is not a physical combination, and the preset
  remains weak. A lip-reed (brass) mouthpiece would be the proper fix.
- Legacy Open Pipe presets without Reed are still noise-driven resonators.
  They benefit from the breath fix but do not self-oscillate. Use PIPE with Jet
  for sustained blown tones.

## Listening and measuring

`AeriformTests --render=<dir>` writes a fixed phrase through the wind presets
and PIPE settings. `python scripts/audition_analyze.py <dir> [<baseline dir>]`
reports level, attack, HNR, pitch, centroid and harmonic balance, optionally
against a baseline render.
